#include "Common.h"
#include "BuildingManager.h"
#include "CCBot.h"
#include "Util.h"

BuildingManager::BuildingManager(CCBot & bot)
    : m_bot(bot)
    , m_buildingPlacer(bot)
    , m_debugMode(false)
    , m_reservedMinerals(0)
    , m_reservedGas(0)
{

}

void BuildingManager::onStart()
{
    m_buildingPlacer.onStart();
}

// gets called every frame from GameCommander
void BuildingManager::onFrame()
{
    for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // filter out units which aren't buildings under construction
        if (m_bot.Data(unit).isBuilding)
        {
            std::stringstream ss;
            ss << unit.getID();
            m_bot.Map().drawText(unit.getPosition(), ss.str());
        }
    }

    validateWorkersAndBuildings();          // check to see if assigned workers have died en route or while constructing
    assignWorkersToUnassignedBuildings();   // assign workers to the unassigned buildings and label them 'planned'    
    constructAssignedBuildings();           // for each planned building, if the worker isn't constructing, send the command    
    checkForStartedConstruction();          // check to see if any buildings have started construction and update data structures    
	checkForDamagedBuildings();	
	repairDamagedBuildings();
	checkFinishedRepairs();
    checkForDeadTerranBuilders();           // if we are terran and a building is under construction without a worker, assign a new one    
    checkForCompletedBuildings();           // check to see if any buildings have completed and update data structures

    drawBuildingInformation();
}

bool BuildingManager::isBeingBuilt(UnitType type)
{
    for (auto & b : m_buildings)
    {
        if (b.type == type)
        {
            return true;
        }
    }

    return false;
}

// STEP 1: DO BOOK KEEPING ON WORKERS WHICH MAY HAVE DIED
void BuildingManager::validateWorkersAndBuildings()
{

    std::vector<Building> toRemove;

    // find any buildings which have become obsolete
    for (auto & b : m_buildings)
    {
        if (b.status != BuildingStatus::UnderConstruction)
        {
            continue;
        }

        auto buildingUnit = b.buildingUnit;

        // TODO: || !b.buildingUnit->getType().isBuilding()
        if (!buildingUnit.isValid())
        {
            toRemove.push_back(b);
        }
    }

    removeBuildings(toRemove);
}

// STEP 2: ASSIGN WORKERS TO BUILDINGS WITHOUT THEM
void BuildingManager::assignWorkersToUnassignedBuildings()
{
    // for each building that doesn't have a builder, assign one
    for (Building & b : m_buildings)
    {
        if (b.status != BuildingStatus::Unassigned)
        {
            continue;
        }

        BOT_ASSERT(!b.builderUnit.isValid(), "Error: Tried to assign a builder to a building that already had one ");

        if (m_debugMode) { printf("Assigning Worker To: %s", b.type.getName().c_str()); }

        // grab a worker unit from WorkerManager which is closest to this final position
        CCTilePosition testLocation = getBuildingLocation(b);
        if (!m_bot.Map().isValidTile(testLocation) || (testLocation.x == 0 && testLocation.y == 0))
        {
            continue;
        }

        b.finalPosition = testLocation;

        // grab the worker unit from WorkerManager which is closest to this final position
        Unit builderUnit = m_bot.Workers().getBuilder(b);
        b.builderUnit = builderUnit;
        if (!b.builderUnit.isValid())
        {
            continue;
        }

		std::cout << "Assigning new worker for " << b.type.getName() << "\n";


        // reserve this building's space
		//if its building with addon, calculate extra tileWidth

        m_buildingPlacer.reserveTiles((int)b.finalPosition.x, (int)b.finalPosition.y, b.type.tileWidth(), b.type.tileHeight());
		
		if (b.type.getName() == "TERRAN_BARRACKS" ||
			b.type.getName() == "TERRAN_FACTORY" ||
			b.type.getName() == "TERRAN_STARPORT") {
			m_buildingPlacer.reserveTiles((int)b.finalPosition.x +3, (int)b.finalPosition.y, 2, 2);
		}


        b.status = BuildingStatus::Assigned;
    }
}

// STEP 3: ISSUE CONSTRUCTION ORDERS TO ASSIGN BUILDINGS AS NEEDED
void BuildingManager::constructAssignedBuildings()
{

	if (!m_bot.isUnderAttack()) {

		for (auto & b : m_buildings)
		{
			if (b.status != BuildingStatus::Assigned)
			{
				continue;
			}

			if (!(b.buildingUnit.getType().isResourceDepot() && m_bot.isExpandingProhibited())) {

				// TODO: not sure if this is the correct way to tell if the building is constructing
				//sc2::AbilityID buildAbility = m_bot.Data(b.type).buildAbility;
				Unit builderUnit = b.builderUnit;

				bool isConstructing = false;


				BOT_ASSERT(builderUnit.isValid(), "null builder unit");
				isConstructing = builderUnit.isConstructing(b.type);


				//check mineral and gas bank
				if (canBuild(b.type)) {

					// if that worker is not currently constructing
					if (!isConstructing)
					{
						// if we haven't explored the build position, go there
						if (!isBuildingPositionExplored(b))
						{
							builderUnit.move(b.finalPosition);
						}
						// if this is not the first time we've sent this guy to build this
						// it must be the case that something was in the way of building
						else if (b.buildCommandGiven)
						{
							//if builder died
							if (!(b.builderUnit.isValid() && b.builderUnit.isAlive())) {

								std::cout << "Builder died, assigning new one\n";

								b.builderUnit = m_bot.Workers().getBuilder(b);
								b.buildCommandGiven = false;

							}
							else {
								//if someone is blocking
								if (b.currentDelay < b.maxDelay) {
									std::cout << "Something is blocking a building, steps before finding new position " << b.currentDelay << "/" << b.maxDelay << "\n";
									b.currentDelay++;
									b.builderUnit.build(b.type, b.finalPosition);
								}
								else {
									//if we ran out of time, find new position
									std::cout << "Finding a new position for " << b.type.getName() << "\n";

									

									//if we are finding a new position for CC
									if (b.type.isResourceDepot()) {
										//we should probably build the CC on third

										m_buildingPlacer.freeTiles((int)b.finalPosition.x, (int)b.finalPosition.y, b.type.tileWidth(), b.type.tileHeight());
										freeAddonTiles(b.finalPosition);

										if ((!b.hasSkipped)) {
											
											CCTilePosition nextExpansion = m_bot.Bases().getSkippedExpansion(Players::Self);
											
											
											b.hasSkipped = true;
											b.finalPosition = nextExpansion;
											b.currentDelay = 0;

											b.builderUnit.build(b.type, b.finalPosition);

										}
										else {
											//if we tried to build on third unsuccesfully, try to return to second
											
											CCTilePosition nextExpansion = m_bot.Bases().getNextExpansion(Players::Self);
											b.hasSkipped = false;
											b.finalPosition = nextExpansion;
											b.currentDelay = 0;

											b.builderUnit.build(b.type, b.finalPosition);
										}


									}
									else {
										//else just find a new valid position from the previous point 

										
										CCTilePosition newStartPosition = Util::GetTilePosition(CCPosition(b.builderUnit.getPosition().x, b.builderUnit.getPosition().y));
										CCTilePosition newBuildingPosition = getBuildingLocation(b);

										m_buildingPlacer.freeTiles((int)b.finalPosition.x, (int)b.finalPosition.y, b.type.tileWidth(), b.type.tileHeight());
										freeAddonTiles(b.finalPosition);


										b.finalPosition = newBuildingPosition;
										b.currentDelay = 0;

										m_buildingPlacer.reserveTiles((int)b.finalPosition.x, (int)b.finalPosition.y, b.type.tileWidth(), b.type.tileHeight());

										if (b.type.getName() == "TERRAN_BARRACKS" ||
											b.type.getName() == "TERRAN_FACTORY" ||
											b.type.getName() == "TERRAN_STARPORT") {
											m_buildingPlacer.reserveTiles((int)b.finalPosition.x + 3, (int)b.finalPosition.y, 2, 2);
										}


										b.builderUnit.build(b.type, b.finalPosition);
									}

								}
							}
						}
						else
						{
							// if it's a refinery, the build command has to be on the geyser unit tag
							if (b.type.isRefinery())
							{
								// first we find the geyser at the desired location
								Unit geyser;
								for (auto unit : m_bot.GetUnits())
								{
									if (unit.getType().isGeyser() && Util::Dist(Util::GetPosition(b.finalPosition), unit.getPosition()) < 3)
									{
										geyser = unit;
										break;
									}
								}

								if (geyser.isValid())
								{
									b.builderUnit.buildTarget(b.type, geyser);
								}
								else
								{
									std::cout << "WARNING: NO VALID GEYSER UNIT FOUND TO BUILD ON, SKIPPING REFINERY\n";
								}
							}
							// if it's not a refinery, we build right on the position
							else
							{
								b.builderUnit.build(b.type, b.finalPosition);
							}

							// set the flag to true
							b.buildCommandGiven = true;
						}
					}
				}
				else if (b.buildCommandGiven == false) {
					if (!b.type.isAddon()) {
						//if i cant build the first bulding from the q, send worker to desired position 
						b.builderUnit.move(b.finalPosition);
						break;
					}
				}

			}
		}
	}
}

// STEP 4: UPDATE DATA STRUCTURES FOR BUILDINGS STARTING CONSTRUCTION
void BuildingManager::checkForStartedConstruction()
{
    // for each building unit which is being constructed
    for (auto buildingStarted : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // filter out units which aren't buildings under construction
        if (!buildingStarted.getType().isBuilding() || !buildingStarted.isBeingConstructed())
        {
            continue;
        }

        // check all our building status objects to see if we have a match and if we do, update it

        for (auto & b : m_buildings)
        {
            if (b.status != BuildingStatus::Assigned)
            {
                continue;
            }

            // check if the positions match
            int dx = b.finalPosition.x - buildingStarted.getTilePosition().x;
            int dy = b.finalPosition.y - buildingStarted.getTilePosition().y;

            if (dx*dx + dy*dy < Util::TileToPosition(1.0f))
            {
                if (b.buildingUnit.isValid())
                {
                    std::cout << "Building mis-match somehow\n";
                }

                // the resources should now be spent, so unreserve them
                m_reservedMinerals -= buildingStarted.getType().mineralPrice();
                m_reservedGas      -= buildingStarted.getType().gasPrice();
                
                // flag it as started and set the buildingUnit
                b.underConstruction = true;
                b.buildingUnit = buildingStarted;


                // put it in the under construction vector
                b.status = BuildingStatus::UnderConstruction;

                // free this space
                m_buildingPlacer.freeTiles((int)b.finalPosition.x, (int)b.finalPosition.y, b.type.tileWidth(), b.type.tileHeight());

                // only one building will match
                break;
            }
        }
    }
}

// STEP 5: IF WE ARE TERRAN, THIS MATTERS, SO: LOL
void BuildingManager::checkForDeadTerranBuilders() {
	if (!m_bot.isUnderAttack()) {
		std::vector<Building> toRemove;
		for (auto & b : m_buildings) {
			if (b.status == BuildingStatus::UnderConstruction || b.status == BuildingStatus::Assigned)
			{
				if (!b.builderUnit.isValid() || !b.builderUnit.isAlive()) {
					Unit builder = m_bot.Workers().getBuilder(b);
					b.builderUnit = builder;

					std::cout << "Assigning new worker to " << b.type.getName() << ", since the last one probably died\n";
					builder.rightClick(b.buildingUnit);

				}
			}
		}
	}
}

void BuildingManager::checkForDamagedBuildings()
{

	for (Unit b : m_bot.UnitInfo().getUnits(Players::Self)) {
		//check if it belongs to a player
		if (b.isValid() && b.getType().isBuilding() && b.isCompleted()) {
			
			

			if ((b.getHitPoints() < b.getUnitPtr()->health_max) ||
				(m_bot.GetThreatLevel() == 3 && b.getType().getName() == "TERRAN_BUNKER"))
			{

				bool found = false;
				//if its not there already

				for (Building damagedB : m_damagedBuildings) {
					if (damagedB.buildingUnit.getID() == b.getID()) {
						found = true;
						break;
					}
				}

				if (!found) {
					std::cout << "New building in need of repairs!\n";
					Building damagedBuilding(b);
					m_damagedBuildings.push_back(damagedBuilding);

				}

				
			}
		}
		
	}
}

void BuildingManager::repairDamagedBuildings()
{

	for (Building & damagedB : m_damagedBuildings) {

		

		int numberOfWorkersNeeded = 1;

		switch (m_bot.GetThreatLevel()) {

		case 0:
			if (damagedB.buildingUnit.getType().getName() == "TERRAN_BUNKER") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_MISSILETURRET") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_PLANTERYFORTRESS" ||
				damagedB.buildingUnit.getType().getName() == "TERRAN_ORBITALCOMMAND") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
				numberOfWorkersNeeded = 1;
			}
			else {
				if (!m_bot.isUnderAttack()) {
					numberOfWorkersNeeded = 1;
				}
				else {
					numberOfWorkersNeeded = 0;
				}

			}
			break;
		case 1:
			if (damagedB.buildingUnit.getType().getName() == "TERRAN_BUNKER") {
				numberOfWorkersNeeded = 3;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_MISSILETURRET") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_PLANTERYFORTRESS" ||
				damagedB.buildingUnit.getType().getName() == "TERRAN_ORBITALCOMMAND") {
				numberOfWorkersNeeded = 3;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
				numberOfWorkersNeeded = 1;
			}
			else {

				if (!m_bot.isUnderAttack()) {
					numberOfWorkersNeeded = 1;
				}
				else {
					numberOfWorkersNeeded = 0;
				}
			}
			break;
		case 2:
			if (damagedB.buildingUnit.getType().getName() == "TERRAN_BUNKER") {
				numberOfWorkersNeeded = 4;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_MISSILETURRET") {
				numberOfWorkersNeeded = 3;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_PLANTERYFORTRESS" ||
				damagedB.buildingUnit.getType().getName() == "TERRAN_ORBITALCOMMAND") {
				numberOfWorkersNeeded = 4;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
				numberOfWorkersNeeded = 2;
			}
			else {
				if (!m_bot.isUnderAttack()) {
					numberOfWorkersNeeded = 1;
				}
				else {
					numberOfWorkersNeeded = 0;
				}
			}
			break;
		case 3:
			if (damagedB.buildingUnit.getType().getName() == "TERRAN_BUNKER") {
				numberOfWorkersNeeded = 5;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_MISSILETURRET") {
				numberOfWorkersNeeded = 4;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_PLANTERYFORTRESS" ||
				damagedB.buildingUnit.getType().getName() == "TERRAN_ORBITALCOMMAND") {
				numberOfWorkersNeeded = 5;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
				numberOfWorkersNeeded = 3;
			}
			else {

				if (!m_bot.isUnderAttack()) {
					numberOfWorkersNeeded = 2;
				}
				else {
					numberOfWorkersNeeded = 0;
				}
			}
			break;
		default:
			if (damagedB.buildingUnit.getType().getName() == "TERRAN_BUNKER") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_MISSILETURRET") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_PLANTERYFORTRESS") {
				numberOfWorkersNeeded = 2;
			}
			else if (damagedB.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
				numberOfWorkersNeeded = 1;
			}
			else {

				if (!m_bot.isUnderAttack()) {
					numberOfWorkersNeeded = 1;
				}
				else {
					numberOfWorkersNeeded = 0;
				}
			}
			break;

		}


		if (damagedB.m_repairUnits.size() < numberOfWorkersNeeded || damagedB.m_repairUnits.size() == 0) {

			std::cout << "Assigning repair workers to building " << damagedB.buildingUnit.getType().getName() << " id " << damagedB.buildingUnit.getID() << "\n";

			for (int i = damagedB.m_repairUnits.size(); i < numberOfWorkersNeeded; i++) {

				Unit builder = m_bot.Workers().getRepairWorker(damagedB.buildingUnit.getPosition(), damagedB.buildingUnit);
				
				if (builder.isAlive() && builder.isValid()) {
					damagedB.m_repairUnits.push_back(builder);
					builder.repair(damagedB.buildingUnit);
				}
				
				
			}

		}	
		else {

			for (int i = 0; i < damagedB.m_repairUnits.size(); i++)
			{
				Unit & repairMan = damagedB.m_repairUnits[i];

				if (repairMan.isValid() && repairMan.isAlive()) {

					if (damagedB.buildingUnit.getHitPoints() == damagedB.buildingUnit.getUnitPtr()->health_max) {

						if (m_bot.Map().getGroundDistance(repairMan.getPosition(), damagedB.buildingUnit.getPosition()) > 8) {
							repairMan.move(damagedB.buildingUnit.getPosition());
						}
						
					}
					else {
						repairMan.repair(damagedB.buildingUnit);
					}
				}
				else {
					std::cout << "Repair worker died assigned to building " << damagedB.buildingUnit.getID() << "\n";
					damagedB.m_repairUnits.erase(damagedB.m_repairUnits.begin() + i);
					break;
				}
			}
		}
	}
}



// STEP 6: CHECK FOR COMPLETED BUILDINGS
void BuildingManager::checkForCompletedBuildings()
{
    std::vector<Building> toRemove;

    // for each of our buildings under construction
    for (auto & b : m_buildings)
    {
        if (b.status != BuildingStatus::UnderConstruction)
        {
            continue;
        }

        // if the unit has completed
        if (b.buildingUnit.isCompleted())
        {
            // if we are terran, give the worker back to worker manager
            if (Util::IsTerran(m_bot.GetPlayerRace(Players::Self)))
            {
                m_bot.Workers().finishedWithWorker(b.builderUnit);

				std::cout << "Building " << b.buildingUnit.getType().getName() << " id: " << b.buildingUnit.getID() << " has finished\n";

				if (b.buildingUnit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
					b.buildingUnit.lowerDepot();
				}
            }

            // remove this unit from the under construction vector
            toRemove.push_back(b);

        }

		//if it does not need any repairs
		
    }

	//custom, if the finished building is a depot, lower it

    removeBuildings(toRemove);
}

void BuildingManager::checkFinishedRepairs()
{
	for (int i = 0; i < m_damagedBuildings.size(); i++) {
		if (((m_damagedBuildings[i].buildingUnit.getHitPoints() == m_damagedBuildings[i].buildingUnit.getUnitPtr()->health_max) &&

			!(m_damagedBuildings[i].buildingUnit.getType().getName() == "TERRAN_BUNKER" && m_bot.GetThreatLevel() == 3)) || 

			!m_damagedBuildings[i].buildingUnit.isAlive()) {

			//std::cout << " building " << m_damagedBuildings[i].buildingUnit.getID() << " is fully repaired " << m_damagedBuildings[i].buildingUnit.getHitPoints() << "\n";
			
			for (auto & unit : m_damagedBuildings[i].m_repairUnits) {
				//std::cout << "worker has finished with repairs, freeing him\n";
				m_bot.Workers().finishedWithWorker(unit);
			}
			m_damagedBuildings.erase(m_damagedBuildings.begin() + i);
			break;
		}
	}

}

void BuildingManager::prepositionWorkers(const UnitType & type, const CCTilePosition & desiredPosition)
{
	if (!m_bot.isUnderAttack()) {

		Building b(type, desiredPosition);

		CCTilePosition testLocation = getBuildingLocation(b); \

			const CCPosition buildingPos(testLocation.x, testLocation.y);

		// grab the worker unit from WorkerManager which is closest to this final position
		Unit builderUnit = m_bot.Workers().getClosestMineralWorkerTo(buildingPos);

		//std::cout << "desired loc, x:" << testLocation.x << " y:" << testLocation.y << "\n";
		//std::cout << builderUnit.getID() << " x:" << builderUnit.getPosition().x << " y:" << builderUnit.getPosition().y << "\n";
		builderUnit.move(testLocation);

		/**
		if (builderUnit.isValid()) {
		builderUnit.move(testLocation);
		}
		else {
		std::cout << "invalid worker \n";
		}
		*/
	}
	
	
}

bool BuildingManager::executeLift(Unit * originBuilding, Unit * designatedBuilding, Unit * designatedAddon)
{
	std::cout << originBuilding->isValid() << designatedBuilding->isValid() << designatedAddon->isValid() << "\n";
	
	originBuildingVar = originBuilding;
	originBuildingID = originBuilding->getID();

	designatedBuildingVar = designatedBuilding;
	designatedBuildingID = designatedBuilding->getID();

	designatedAddonVar = designatedAddon;
	designatedAddonID = designatedAddon->getID();

	//std::cout << " ~executing lift~ ,b1: " << originBuildingVar->getType().getName() << " " << originBuildingVar->getID() << "\n";

	originBuildingVar->lift();
	designatedBuildingVar->lift();

	//cout all positions!
	//std::cout << " ~executed lift~ \n";

	return false;
}

bool BuildingManager::executeSwap()
{


	if (!executingSwap) {

		//std::cout << " origin building id: " << originBuildingID << "\n";
		CCUnitID lokalID;
		Unit lokalBuildingOrigin, lokalBuildingDest;

		for (Unit unit : m_bot.UnitInfo().getUnits(Players::Self)) {
			if (unit.getID() == originBuildingID) {
				originBuildingVar = &unit;

				lokalBuildingOrigin = unit;
				lokalID = unit.getID();

				//std::cout << " origin building id2: " << originBuildingVar->getID() << " should equal to " << originBuildingID << "\n";
			}
			else if (unit.getID() == designatedBuildingID) {
				lokalBuildingDest = unit;
			}
			else if (unit.getID() == designatedAddonID) {
				//designatedBuildingVar = unit;
			}
		}

		//std::cout << " origin building global id3: " << originBuildingVar->getID() << " should equal to " << originBuildingID << "\n";

		//std::cout << " origin building lokal id4: " << lokalBuildingOrigin.getID() << " should equal to " << lokalID << "\n";

		//std::cout << " ~acquiring swap positions~ \n";
		//std::cout << " ~executing swap~ ,b1: " << lokalBuildingOrigin.getType().getName() << " " << lokalBuildingOrigin.getID() << "\n";


		CCTilePosition originPos = lokalBuildingOrigin.getTilePosition();
		CCTilePosition designatedPos = lokalBuildingDest.getTilePosition();


		//std::cout << " ~swap position acquired~ \n";

		lokalBuildingOrigin.land(designatedPos);
		lokalBuildingDest.land(originPos);

		executingSwap = true;

		//lokalBuildingOrigin.queuedMove(designatedPos);
		//lokalBuildingDest.queuedMove(originPos);
		//cout all positions!
		//std::cout << " ~executed~ \n";
		
		return false;
	}
	else {

		for (Unit unit : m_bot.UnitInfo().getUnits(Players::Self)) {
			if (unit.getType().isBuilding() && unit.isFlying()) {
				//std::cout << " ~some building is flying apparently~ \n";
				return false;
			}
		}
		executingSwap = false;
		//std::cout << " ~building swap has finished~ \n";
		return true;
	}
	
}


bool BuildingManager::canBuild(const UnitType & type)
{
	if (m_bot.GetMinerals() >= type.mineralPrice() && m_bot.GetGas() >= type.gasPrice()) {
		return true;
	}
	return false;
}

// add a new building to be constructed
void BuildingManager::addBuildingTask(const UnitType & type, const CCTilePosition & desiredPosition)
{
    m_reservedMinerals  += m_bot.Data(type).mineralCost;
    m_reservedGas	    += m_bot.Data(type).gasCost;

    Building b(type, desiredPosition);
    b.status = BuildingStatus::Unassigned;

    m_buildings.push_back(b);
}

// TODO: may need to iterate over all tiles of the building footprint
bool BuildingManager::isBuildingPositionExplored(const Building & b) const
{
    return m_bot.Map().isExplored(b.finalPosition);
}


char BuildingManager::getBuildingWorkerCode(const Building & b) const
{
    return b.builderUnit.isValid() ? 'W' : 'X';
}

int BuildingManager::getReservedMinerals()
{
    return m_reservedMinerals;
}

int BuildingManager::getReservedGas()
{
    return m_reservedGas;
}

void BuildingManager::drawBuildingInformation()
{
    m_buildingPlacer.drawReservedTiles();

    if (!m_bot.Config().DrawBuildingInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Building Information " << m_buildings.size() << "\n\n\n";

    int yspace = 0;

    for (const auto & b : m_buildings)
    {
        std::stringstream dss;

        if (b.builderUnit.isValid())
        {
            dss << "\n\nBuilder: " << b.builderUnit.getID() << "\n";
        }

        if (b.buildingUnit.isValid())
        {
            dss << "Building: " << b.buildingUnit.getID() << "\n" << b.buildingUnit.getBuildPercentage();
            m_bot.Map().drawText(b.buildingUnit.getPosition(), dss.str());
        }
        
        if (b.status == BuildingStatus::Unassigned)
        {
            ss << "Unassigned " << b.type.getName() << "    " << getBuildingWorkerCode(b) << "\n";
        }
        else if (b.status == BuildingStatus::Assigned)
        {
            ss << "Assigned " << b.type.getName() << "    " << b.builderUnit.getID() << " " << getBuildingWorkerCode(b) << " (" << b.finalPosition.x << "," << b.finalPosition.y << ")\n";

            int x1 = b.finalPosition.x;
            int y1 = b.finalPosition.y;
            int x2 = b.finalPosition.x + b.type.tileWidth();
            int y2 = b.finalPosition.y + b.type.tileHeight();

            m_bot.Map().drawBox((CCPositionType)x1-1, (CCPositionType)y1-1, (CCPositionType)x2-1, (CCPositionType)y2-1, CCColor(255, 0, 0));
            //m_bot.Map().drawLine(b.finalPosition, m_bot.GetUnit(b.builderUnitTag)->pos, CCColors::Yellow);
        }
        else if (b.status == BuildingStatus::UnderConstruction)
        {
            ss << "Constructing " << b.type.getName() << "    " << getBuildingWorkerCode(b) << "\n";
        }
    }



    m_bot.Map().drawTextScreen(0.3f, 0.05f, ss.str());
}

std::vector<UnitType> BuildingManager::buildingsQueued() const
{
    std::vector<UnitType> buildingsQueued;

    for (const auto & b : m_buildings)
    {
        if (b.status == BuildingStatus::Unassigned || b.status == BuildingStatus::Assigned)
        {
            buildingsQueued.push_back(b.type);
        }
    }

    return buildingsQueued;
}

CCTilePosition BuildingManager::getBuildingLocation(const Building & b)
{
    size_t numPylons = m_bot.UnitInfo().getUnitTypeCount(Players::Self, Util::GetSupplyProvider(m_bot.GetPlayerRace(Players::Self), m_bot), true);


    if (b.type.isRefinery())
    {
        return m_buildingPlacer.getRefineryPosition();
    }

    if (b.type.isResourceDepot())
    {
        return m_bot.Bases().getNextExpansion(Players::Self);
    }

	if (b.type.getName() == "TERRAN_SUPPLYDEPOT") {

		sc2::Point2D testPoint = m_bot.Map().getWallPositionDepot();
		if (testPoint != sc2::Point2D(0, 0))
		{
			return CCTilePosition(testPoint.x, testPoint.y);
		}
		return m_buildingPlacer.getBuildLocationNear(b, 0);
	}

	if (b.type.getName() == "TERRAN_MISSILETURRET" ||
		b.type.getName() == "TERRAN_BUNKER"  ||
		b.type.getName() == "TERRAN_ENGINEERINGBAY" || 
		b.type.getName() == "TERRAN_ARMORY" ||
		b.type.getName() == "TERRAN_STARPORT") {
		return m_buildingPlacer.getBuildLocationNear(b, 0);
	}

    // get a position within our region
    return m_buildingPlacer.getBuildLocationNear(b, m_bot.Config().BuildingSpacing);
}

void BuildingManager::freeAddonTiles(CCTilePosition pos)
{
	int posX = pos.x;
	int posY = pos.y;

	int addPosX = posX + 3;
	int addPosY = posY;

	int addonWidth = 2;
	int addonHeight = 2;
	
	m_buildingPlacer.freeTiles(addPosX, addPosY, addonWidth, addonHeight);
}

void BuildingManager::removeBuildings(const std::vector<Building> & toRemove)

{
    for (auto & b : toRemove)
    {
        const auto & it = std::find(m_buildings.begin(), m_buildings.end(), b);

        if (it != m_buildings.end())
        {
            m_buildings.erase(it);
        }
    }
}