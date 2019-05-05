#pragma once

#include "Common.h"
#include "DistanceMap.h"
#include "Unit.h"
#include <map>
#include <vector>
#include <cmath>

class CCBot;

class BaseLocation
{
    CCBot &                     m_bot;
    DistanceMap                 m_distanceMap;


    CCTilePosition              m_depotPosition;
	CCPosition					m_centerOfMinerals;
	CCPosition					m_centerOfBase;
    CCPosition                  m_centerOfResources;

	

    std::vector<Unit>           m_geysers;
    std::vector<Unit>           m_minerals;

    std::vector<CCPosition>     m_mineralPositions;
    std::vector<CCPosition>     m_geyserPositions;

    std::map<CCPlayer, bool>    m_isPlayerOccupying;
    std::map<CCPlayer, bool>    m_isPlayerStartLocation;
        
    int                         m_baseID;
    CCPositionType              m_left;
    CCPositionType              m_right;
    CCPositionType              m_top;
    CCPositionType              m_bottom;
	
    bool                        m_isStartLocation;


	sc2::Point2D				m_bunkerPosition; //c
	std::vector<CCTilePosition>	m_backlineTurretPositions; //c
	CCPosition				m_inlineTurretPosition; //c
    
public:

	CCPosition					m_behindMineralLine;
	CCPosition					m_mineralEdge1;
	CCPosition					m_mineralEdge2;
	

    BaseLocation(CCBot & bot, int baseID, const std::vector<Unit> & resources);
    
    int getGroundDistance(const CCPosition & pos) const;
    int getGroundDistance(const CCTilePosition & pos) const;
    bool isStartLocation() const;
    bool isPlayerStartLocation(CCPlayer player) const;
    bool isMineralOnly() const;
    bool containsPosition(const CCPosition & pos) const;
    const CCTilePosition & getDepotPosition() const;
	const CCPosition & getInlineTurretPosition() const;
    const CCPosition & getPosition() const;
    const std::vector<Unit> & getGeysers() const;
    const std::vector<Unit> & getMinerals() const;
    bool isOccupiedByPlayer(CCPlayer player) const;
    bool isExplored() const;
    bool isInResourceBox(int x, int y) const;

	const int getBaseID() const; //custom
	const sc2::Point2D & getCenterOfRessources() const noexcept;
	const sc2::Point2D & getCenterOfMinerals() const noexcept;


	const int getNumberOfTurrets() const;	//custom
	const sc2::Point2D & getCenterOfBase() const noexcept;
    void setPlayerOccupying(CCPlayer player, bool occupying);
    const std::vector<CCTilePosition> & getClosestTiles() const;
    void draw();


	void mineralDepleted(int positionInArray);	//custom
	void calculateTurretPositions(); //custom
};
