#include "BaseLocation.h"
#include "Util.h"
#include "CCBot.h"
#include <sstream>
#include <iostream>

const int NearBaseLocationTileDistance = 20;

BaseLocation::BaseLocation(CCBot & bot, int baseID, const std::vector<Unit> & resources)
    : m_bot(bot)
    , m_baseID               (baseID)
    , m_isStartLocation      (false)
    , m_left                 (std::numeric_limits<CCPositionType>::max())
    , m_right                (std::numeric_limits<CCPositionType>::lowest())
    , m_top                  (std::numeric_limits<CCPositionType>::lowest())
    , m_bottom               (std::numeric_limits<CCPositionType>::max())
	, m_centerOfBase(0.0f, 0.0f)
{
    m_isPlayerStartLocation[0] = false;
    m_isPlayerStartLocation[1] = false;
    m_isPlayerOccupying[0] = false;
    m_isPlayerOccupying[1] = false;

    CCPositionType resourceCenterX = 0;
    CCPositionType resourceCenterY = 0;
	float mineralsCenterX = 0;
	float mineralsCenterY = 0;


    // add each of the resources to its corresponding container
    for (auto & resource : resources)
    {
        if (resource.getType().isMineral())
        {
            m_minerals.push_back(resource);
            m_mineralPositions.push_back(resource.getPosition());

            // add the position of the minerals to the center
            resourceCenterX += resource.getPosition().x;
            resourceCenterY += resource.getPosition().y;
			mineralsCenterX += resource.getPosition().x;
			mineralsCenterY += resource.getPosition().y;
        }
        else
        {
            m_geysers.push_back(resource);
            m_geyserPositions.push_back(resource.getPosition());

            // pull the resource center toward the geyser if it exists
            resourceCenterX += resource.getPosition().x;
            resourceCenterY += resource.getPosition().y;
        }

        // set the limits of the base location bounding box
        CCPositionType resWidth = Util::TileToPosition(1);
        CCPositionType resHeight = Util::TileToPosition(0.5);

        m_left   = std::min(m_left,   resource.getPosition().x - resWidth);
        m_right  = std::max(m_right,  resource.getPosition().x + resWidth);
        m_top    = std::max(m_top,    resource.getPosition().y + resHeight);
        m_bottom = std::min(m_bottom, resource.getPosition().y - resHeight);
    }

    // calculate the center of the resources
    size_t numResources = m_minerals.size() + m_geysers.size();

	const sc2::Point2D centerMinerals(mineralsCenterX / m_minerals.size(), mineralsCenterY / m_minerals.size());
	m_centerOfMinerals = centerMinerals;
    m_centerOfResources = CCPosition(m_left + (m_right-m_left)/2, m_top + (m_bottom-m_top)/2);

    // compute this BaseLocation's DistanceMap, which will compute the ground distance
    // from the center of its recourses to every other tile on the map
    m_distanceMap = m_bot.Map().getDistanceMap(m_centerOfResources);

    // check to see if this is a start location for the map
    for (auto & pos : m_bot.GetStartLocations())
    {
        if (containsPosition(pos))
        {
            m_isStartLocation = true;
            m_depotPosition = Util::GetTilePosition(pos);
			m_centerOfBase = CCPosition(m_depotPosition.x, m_depotPosition.y);
			
        }
    }
    
    // if this base location position is near our own resource depot, it's our start location
    for (auto & unit : m_bot.GetUnits())
    {
        if (unit.getPlayer() == Players::Self && unit.getType().isResourceDepot() && containsPosition(unit.getPosition()))
        {
            m_isPlayerStartLocation[Players::Self] = true;
            m_isStartLocation = true;
            m_isPlayerOccupying[Players::Self] = true;
			m_centerOfBase = unit.getPosition();
            break;
        }
    }

    
    // if it's not a start location, we need to calculate the depot position
    if (!isStartLocation())
    {
        UnitType depot = Util::GetTownHall(m_bot.GetPlayerRace(Players::Self), m_bot);
		
#ifdef SC2API
        int offsetX = 0;
        int offsetY = 0;
#else
        int offsetX = 1;
        int offsetY = 1;
#endif
        
        // the position of the depot will be the closest spot we can build one from the resource center
        for (auto & tile : getClosestTiles())
        {
            // the build position will be up-left of where this tile is
            // this means we are positioning the center of the resouce depot
            CCTilePosition buildTile(tile.x - offsetX, tile.y - offsetY);

            if (m_bot.Map().canBuildTypeAtPosition(buildTile.x, buildTile.y, depot))
            {
                m_depotPosition = buildTile;
				m_centerOfBase = CCPosition(buildTile.x, buildTile.y);
                break;
            }
        }
    }

	m_inlineTurretPosition = CCPosition((m_centerOfBase.x + (m_centerOfMinerals.x *2)) / 3, (m_centerOfBase.y + (m_centerOfMinerals.y)*2) / 3);

	float bestDist = 0;

	for (auto & mineral1 : m_minerals) {
		for (auto & mineral2 : m_minerals) {

			if (Util::Dist(mineral1, mineral2) > bestDist) {

				m_mineralEdge1 = mineral1.getPosition();
				m_mineralEdge2 = mineral2.getPosition();

				bestDist = Util::Dist(mineral1, mineral2);
			}

		}
	}
	m_mineralEdge1 = Util::prolongDirection(m_centerOfBase,m_mineralEdge1,0.25);
	m_mineralEdge2 = Util::prolongDirection(m_centerOfBase, m_mineralEdge2, 0.25);
	m_behindMineralLine = Util::prolongDirection(m_centerOfBase, m_centerOfMinerals, 0.5);

	/**
	if (isPlayerStartLocation(Players::Self)) {
		m_bunkerPosition = m_bot.Map().getBunkerPosition();
	}
	*/
}

// TODO: calculate the actual depot position
const CCTilePosition & BaseLocation::getDepotPosition() const
{
    return m_depotPosition;
}

const CCPosition & BaseLocation::getInlineTurretPosition() const
{
	return m_inlineTurretPosition;
}

void BaseLocation::setPlayerOccupying(CCPlayer player, bool occupying)
{
    m_isPlayerOccupying[player] = occupying;

    // if this base is a start location that's occupied by the enemy, it's that enemy's start location
    if (occupying && player == Players::Enemy && isStartLocation() && m_isPlayerStartLocation[player] == false)
    {
        m_isPlayerStartLocation[player] = true;
    }
}

bool BaseLocation::isInResourceBox(int tileX, int tileY) const
{
    CCPositionType px = Util::TileToPosition((float)tileX);
    CCPositionType py = Util::TileToPosition((float)tileY);
    return px >= m_left && px < m_right && py < m_top && py >= m_bottom;
}

const int BaseLocation::getBaseID() const
{
	return m_baseID;
}

const sc2::Point2D & BaseLocation::getCenterOfRessources() const noexcept
{
	return m_centerOfResources;
}

const sc2::Point2D & BaseLocation::getCenterOfMinerals() const noexcept
{
	return m_centerOfMinerals;
}



const int BaseLocation::getNumberOfTurrets() const
{
	int turrets = 0;
	for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self)) {
		if (unit.getType().getName() == "TERRAN_MISSILETURRET") {

			float closestPosition = 10000;
			int baseID = 0;

			for (auto & base : m_bot.Bases().getOccupiedBaseLocations(Players::Self)) {
				if (Util::Dist(unit.getPosition(), base->getCenterOfBase()) < closestPosition) {

					closestPosition = Util::Dist(unit.getPosition(), base->getCenterOfBase());
					baseID = base->getBaseID();

				}
			}

			if (baseID == this->getBaseID()) {
				turrets++;
			}
		}
	}
	return turrets;
}

const sc2::Point2D & BaseLocation::getCenterOfBase() const noexcept
{
	// TODO: insert return statement here
	return m_centerOfBase;
}

bool BaseLocation::isOccupiedByPlayer(CCPlayer player) const
{
    return m_isPlayerOccupying.at(player);
}

bool BaseLocation::isExplored() const
{
    return m_bot.Map().isExplored(m_centerOfResources);
}

bool BaseLocation::isPlayerStartLocation(CCPlayer player) const
{
    return m_isPlayerStartLocation.at(player);
}

bool BaseLocation::containsPosition(const CCPosition & pos) const
{
    if (!m_bot.Map().isValidPosition(pos) || (pos.x == 0 && pos.y == 0))
    {
        return false;
    }

    return getGroundDistance(pos) < NearBaseLocationTileDistance;
}

const std::vector<Unit> & BaseLocation::getGeysers() const
{
    return m_geysers;
}

const std::vector<Unit> & BaseLocation::getMinerals() const
{
    return m_minerals;
}

const CCPosition & BaseLocation::getPosition() const
{
    return m_centerOfResources;
}

int BaseLocation::getGroundDistance(const CCPosition & pos) const
{
    return m_distanceMap.getDistance(pos);
}

int BaseLocation::getGroundDistance(const CCTilePosition & pos) const
{
    return m_distanceMap.getDistance(pos);
}

bool BaseLocation::isStartLocation() const
{
    return m_isStartLocation;
}

const std::vector<CCTilePosition> & BaseLocation::getClosestTiles() const
{

    return m_distanceMap.getSortedTiles();
}

void BaseLocation::draw()
{
    CCPositionType radius = Util::TileToPosition(1.0f);

    m_bot.Map().drawCircle(m_centerOfResources, radius, CCColor(255, 255, 0));

	m_bot.Map().drawCircle(m_centerOfMinerals, radius, CCColor(79, 255, 252));

	m_bot.Map().drawCircle(m_inlineTurretPosition, radius, CCColor(200, 100, 100));

    std::stringstream ss;
    ss << "BaseLocation: " << m_baseID << "\n";
    ss << "Start Loc:    " << (isStartLocation() ? "true" : "false") << "\n";
    ss << "Minerals:     " << m_mineralPositions.size() << "\n";
    ss << "Geysers:      " << m_geyserPositions.size() << "\n";
    ss << "Occupied By:  ";

    if (isOccupiedByPlayer(Players::Self))
    {
        ss << "Self ";
    }

    if (isOccupiedByPlayer(Players::Enemy))
    {
        ss << "Enemy ";
    }

    m_bot.Map().drawText(CCPosition(m_left, m_top+3), ss.str().c_str());
    m_bot.Map().drawText(CCPosition(m_left, m_bottom), ss.str().c_str());

    // draw the base bounding box
    m_bot.Map().drawBox(m_left, m_top, m_right, m_bottom);

    for (CCPositionType x=m_left; x < m_right; x += Util::TileToPosition(1.0f))
    {
        //m_bot.Map().drawLine(x, m_top, x, m_bottom, CCColor(160, 160, 160));
    }

    for (CCPositionType y=m_bottom; y<m_top; y += Util::TileToPosition(1.0f))
    {
        //m_bot.Map().drawLine(m_left, y, m_right, y, CCColor(160, 160, 160));
    }

    for (auto & mineralPos : m_mineralPositions)
    {
        m_bot.Map().drawCircle(mineralPos, radius, CCColor(0, 128, 128));
    }

    for (auto & geyserPos : m_geyserPositions)
    {
        m_bot.Map().drawCircle(geyserPos, radius, CCColor(0, 255, 0));
    }

    if (m_isStartLocation)
    {
        m_bot.Map().drawCircle(Util::GetPosition(m_depotPosition), radius, CCColor(255, 0, 0));

    }

	m_bot.Map().drawCircle(m_mineralEdge1, radius, CCColor(200, 100, 100));
	m_bot.Map().drawCircle(m_mineralEdge2, radius, CCColor(200, 100, 100));
	m_bot.Map().drawCircle(m_behindMineralLine, radius, CCColor(0, 200, 0));

    m_bot.Map().drawTile(m_depotPosition.x, m_depotPosition.y, CCColor(0, 0, 255)); 

	/**
	if (isPlayerStartLocation(Players::Self)) {
		m_bot.Map().drawCircle(m_bunkerPosition, radius, CCColor(200, 70, 250));
	}
	*/


    m_distanceMap.draw(m_bot);

}

void BaseLocation::mineralDepleted(int positionInArray)
{
	m_minerals.erase(m_minerals.begin() + positionInArray);
}

void BaseLocation::calculateTurretPositions()
{
	//calculate inline turret position
	
	
	//calculate behind line turret positions


}

bool BaseLocation::isMineralOnly() const
{
    return getGeysers().empty();
}