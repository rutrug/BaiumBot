#include "ProductionManager.h"
#include "Util.h"
#include "CCBot.h"
#include <cstdlib>
#include <math.h>

ProductionManager::ProductionManager(CCBot & bot)
    : m_bot             (bot)
    , m_buildingManager (bot)
    , m_queue           (bot)
{

}

void ProductionManager::setBuildOrder(const BuildOrder & buildOrder)
{
    m_queue.clearAll();

    for (size_t i(0); i<buildOrder.size(); ++i)
    {
        m_queue.queueAsLowestPriority(buildOrder[i], true);
    }
}


void ProductionManager::onStart()
{
    m_buildingManager.onStart();
    setBuildOrder(m_bot.Strategy().getOpeningBookBuildOrder());
}

void ProductionManager::onFrame()
{
	if (manageBOcdMax <= manageBOcdCurrent) {

		fixBuildOrderDeadlock();
		manageBuildOrderQueue();
		ManageOCEnergy();
		manageMacroLoop();

		manageBOcdCurrent = 0;
	}
	else {
		manageBOcdCurrent++;
		
	}

	m_buildingManager.onFrame();

	drawProductionInformation();
}

void ProductionManager::onUnitDestroy(const Unit & unit)
{
    
}

void ProductionManager::manageBuildOrderQueue()
{
    // if there is nothing in the queue, oh well
    if (m_queue.isEmpty())
    {
        return;
    }

    // the current item to be used
    BuildOrderItem & currentItem = m_queue.getHighestPriorityItem();

    // while there is still something left in the queue
	while (!m_queue.isEmpty())
	{

		if (currentItem.type.isSwap()) {
			std::cout << "Swapping " << currentItem.type.getSwapOriginBuildingType().getName() << " with " <<
				currentItem.type.getSwapDesignatedBuildingType().getName() << " looking for addon: " <<
				currentItem.type.getSwapDesignatedAddonType().getName() << "\n";

			swap(currentItem);

			
			break;
		}


		// this is the unit which can produce the currentItem
		Unit producer = getProducer(currentItem.type);

		// check to see if we can make it right now
		bool canMake = canMakeNow(producer, currentItem.type);

		bool isAddon = false;
		if (!(currentItem.type.isTech() || currentItem.type.isUpgrade())) {
			isAddon = currentItem.type.getUnitType().isAddon();
		}

		//std::cout << currentItem.type.getName() << " " << producer.getType().getName() << " can make " << canMake << " (#3)\n";

		// if we can make the current item
		if (producer.isValid() && canMake && !isAddon)
		{
			
			// create it and remove it from the _queue

			create(producer, currentItem);
			m_queue.removeCurrentHighestPriorityItem();

			// don't actually loop around in here
			break;
		}
		else //even if i cant make it right now, i can preposition workers 

			if ((producer.isValid() && 
				(currentItem.type.getName() == "CommandCenter" || currentItem.type.getName() == "SupplyDepot")
				&& 
				!canMake)) {

				if (currentItem.type.getName() == "SupplyDepot" && m_bot.Observation()->GetMinerals() >= 60) {

					m_buildingManager.prepositionWorkers(currentItem.type.getUnitType(), Util::GetTilePosition(m_bot.GetStartLocation()));
				}


				if (currentItem.type.getName() == "CommandCenter" && m_bot.Observation()->GetMinerals() >= 200) {
					m_buildingManager.prepositionWorkers(currentItem.type.getUnitType(), Util::GetTilePosition(m_bot.GetStartLocation()));
				}

				//create(producer, currentItem);
				//m_queue.removeCurrentHighestPriorityItem();

				break;


			}

			// if im trying to build addon its little bit different
			else if (producer.isValid() && canMake && isAddon) {

				create(producer, currentItem);
				m_queue.removeCurrentHighestPriorityItem();

				break;

			}

			// otherwise, if we can skip the current item
			else if (m_queue.canSkipItem())
			{

				// skip it
				m_queue.skipItem();

				// and get the next one
				currentItem = m_queue.getNextHighestPriorityItem();

			}
        else
        {
            // so break out
            break;
        }
    }
}

void ProductionManager::fixBuildOrderDeadlock()
{
	int startingDeadlockValue = deadlockValue;

    if (m_queue.isEmpty()) { return; }
    BuildOrderItem & currentItem = m_queue.getHighestPriorityItem();

    // check to see if we have the prerequisites for the topmost item
    bool hasRequired = m_bot.Data(currentItem.type).requiredUnits.empty();
    for (auto & required : m_bot.Data(currentItem.type).requiredUnits)
    {
        if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, required, false) > 0 || m_buildingManager.isBeingBuilt(required))
        {
            hasRequired = true;
            break;
        }
    }

    if (!hasRequired)
    {
        m_queue.queueAsHighestPriority(MetaType(m_bot.Data(currentItem.type).requiredUnits[0], m_bot), true);
        fixBuildOrderDeadlock();
        return;
    }

    // build the producer of the unit if we don't have one
    bool hasProducer = m_bot.Data(currentItem.type).whatBuilds.empty();
    for (auto & producer : m_bot.Data(currentItem.type).whatBuilds)
    {
        if (m_bot.UnitInfo().getUnitTypeCount(Players::Self, producer, false) > 0 || m_buildingManager.isBeingBuilt(producer))
        {
            hasProducer = true;
            break;
        }
    }

    if (!hasProducer)
    {
		std::cout << "Warning, BO deadlock imminent: " << deadlockValue++ << " /100\n"; 
		if (deadlockValue > 100) {
			m_queue.queueAsHighestPriority(MetaType(m_bot.Data(currentItem.type).whatBuilds[0], m_bot), true);
			fixBuildOrderDeadlock();
		}
    }

    // build a refinery if we don't have one and the thing costs gas
    auto refinery = Util::GetRefinery(m_bot.GetPlayerRace(Players::Self), m_bot);
    if (m_bot.Data(currentItem.type).gasCost > 0 && m_bot.UnitInfo().getUnitTypeCount(Players::Self, refinery, false) == 0)
    {
        m_queue.queueAsHighestPriority(MetaType(refinery, m_bot), true);
    } 

    // build supply if we need some
    auto supplyProvider = Util::GetSupplyProvider(m_bot.GetPlayerRace(Players::Self), m_bot);
    if (m_bot.Data(currentItem.type).supplyCost > (m_bot.GetMaxSupply() - m_bot.GetCurrentSupply()) && !m_buildingManager.isBeingBuilt(supplyProvider))
    {
        m_queue.queueAsHighestPriority(MetaType(supplyProvider, m_bot), true);
    }

	if (startingDeadlockValue >= deadlockValue) {
		deadlockValue = 0;
	}
}

void ProductionManager::freeBuildOrderQueue()
{
	this->m_queue.clearAll();
}

int ProductionManager::numberOfAutomatedItems()
{
	return metaTypePriorityQueue.size();
}

int ProductionManager::numberOfQueuedItems()
{
	return m_queue.size();
}

Unit ProductionManager::getProducer(const MetaType & type, CCPosition closestTo, bool queueLimit)
{
    // get all the types of units that cna build this type
    auto & producerTypes = m_bot.Data(type).whatBuilds;

    // make a set of all candidate producers
    std::vector<Unit> candidateProducers;
	std::vector<Unit> finalCandidateProducers;
	size_t leastAmountOfOrders = 10;

    for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        // reasons a unit can not train the desired type
        if (std::find(producerTypes.begin(), producerTypes.end(), unit.getType()) == producerTypes.end()) { continue; }
        if (!unit.isCompleted()) { continue; }
        
		
		//if (m_bot.Data(unit).isBuilding && unit.isTraining()) { continue; } //custom edited
		if(type.isTech() && m_bot.Data(unit).isBuilding && unit.isTraining()) { continue; }


        if (unit.isFlying()) { continue; }


        // if we haven't cut it, add it to the set of candidates
		if (unit.getNumberOfOrders() < leastAmountOfOrders) {
			leastAmountOfOrders = unit.getNumberOfOrders();
		}
		if (queueLimit) {
			if (unit.getNumberOfOrders() >= 2) {
				continue;
			}
		}
        candidateProducers.push_back(unit);
    }


	if (type.isAddon() && candidateProducers.size() > 0) {

		int randIndex = rand() % candidateProducers.size();
		return candidateProducers.at(randIndex);

	}

	//get producers with the least amount of orders but check if producer has reactor addon

	if (type.isUnit()) {
		if (candidateProducers.size() >= 1) {
			for (auto unit : candidateProducers) {

				CCPosition unitPos = unit.getPosition();
				CCPosition addonApproximation = unitPos + CCPosition(2, 0);

				//check if it has an reactor
				for (auto lookingForAddon : m_bot.UnitInfo().getUnits(Players::Self)) {

					if (lookingForAddon.getType().isAddon()) {
						if (lookingForAddon.getType().isReactor()) {
							

							if (lookingForAddon.isValid() && !lookingForAddon.isBeingConstructed()) {

								int grDist = m_bot.Map().getGroundDistance(lookingForAddon.getPosition(), addonApproximation);
								
								if (grDist < 3) {
									//we found a building with reactor!
									
									if (unit.getNumberOfOrders() - 1 <= leastAmountOfOrders) {
										finalCandidateProducers.push_back(unit);
									}

								}
							}
						}
					}

				}
			}
		}
	}
	
	if (finalCandidateProducers.size() == 0) {
		if (candidateProducers.size() > 1) {
			for (auto unit : candidateProducers) {
				if (unit.getNumberOfOrders() <= leastAmountOfOrders) {
					finalCandidateProducers.push_back(unit);
				}
			}
		}
	}

	if (finalCandidateProducers.size() > 0) {
		return getClosestUnitToPosition(finalCandidateProducers, closestTo);
	}
	else {
		return getClosestUnitToPosition(candidateProducers, closestTo);
	}

	
}

int ProductionManager::numberOfViableProducers(const MetaType & type, CCPosition closestTo)
{
	// get all the types of units that cna build this type
	auto & producerTypes = m_bot.Data(type).whatBuilds;

	// make a set of all candidate producers

	int freeSlots = 0;

	for (auto unit : m_bot.UnitInfo().getUnits(Players::Self))
	{
		// reasons a unit can not train the desired type
		if (std::find(producerTypes.begin(), producerTypes.end(), unit.getType()) == producerTypes.end()) { continue; }
		if (!unit.isCompleted()) { continue; }


		if (type.isTech() && m_bot.Data(unit).isBuilding && unit.isTraining()) { continue; }

		if (unit.isFlying()) { continue; }

		
		if (unit.getNumberOfOrders() >= 2) {
			continue;
		}

		freeSlots += 2 - unit.getNumberOfOrders();
	}

	return freeSlots;
}

Unit ProductionManager::getClosestUnitToPosition(const std::vector<Unit> & units, CCPosition closestTo)
{
    if (units.size() == 0)
    {
        return Unit();
    }

    // if we don't care where the unit is return the first one we have
    if (closestTo.x == 0 && closestTo.y == 0)
    {
        return units[0];
    }

    Unit closestUnit;
    double minDist = std::numeric_limits<double>::max();

    for (auto & unit : units)
    {
        double distance = Util::Dist(unit, closestTo);
        if (!closestUnit.isValid() || distance < minDist)
        {
            closestUnit = unit;
            minDist = distance;
        }
    }

    return closestUnit;
}

// this function will check to see if all preconditions are met and then create a unit
void ProductionManager::create(const Unit & producer, BuildOrderItem & item)
{
    if (!producer.isValid())
    {
        return;
    }
	

	//if we are building an addon
	if(item.type.isAddon()) {

		producer.train(item.type.getUnitType());

		//new buildingManager method to locate producer and free his addon tiles ?

		m_buildingManager.freeAddonTiles(CCTilePosition(producer.getPosition().x, producer.getPosition().y));
		

	} 
    // if we're dealing with a building
    else if (item.type.isBuilding())
    {
        if (item.type.getUnitType().isMorphedBuilding())
        {
			//std::cout << "we are morphing \n";
            producer.morph(item.type.getUnitType());
        }
        else
        {
			if (item.type.getUnitType().getName() == "TERRAN_BARRACKS") {
				int numberOfBarracks = 0;
				int numberOfDepots = 0;
				CCPosition depotPos;
				

				for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self)) {

					if (unit.getType().getName() == "TERRAN_BARRACKS") {
						numberOfBarracks++;
					}
					else if (unit.getType().getName() == "TERRAN_SUPPLYDEPOTLOWERED" ||
						unit.getType().getName() == "TERRAN_SUPPLYDEPOT") {
						numberOfDepots++;
						depotPos = unit.getPosition();
					}
				}
				if (numberOfDepots == 1 && numberOfBarracks == 0) {
					CCPosition basePos = m_bot.Bases().getPlayerStartingBaseLocation(0)->getPosition();
					CCPosition finalPosition = (depotPos  + basePos) / 2;

					m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(finalPosition));
				}
				else {
					m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(m_bot.GetStartLocation()));
				}

			} else 
			if (item.type.getUnitType().getName() == "TERRAN_STARPORT") {
				for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self)) {
					if (unit.getType().getName() == "TERRAN_FACTORY") {
						m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(unit.getPosition()));
						break;
					}
				}
				//find location for missile turrets
			} else
			if (item.type.getUnitType().getName() == "TERRAN_MISSILETURRET") {
				const BaseLocation *chosenBase;
				int leastTurrets = 100;

				for (auto & base : m_bot.Bases().getOccupiedBaseLocations(Players::Self)) {
					
					int turretsFound = base->getNumberOfTurrets();
					if (turretsFound < leastTurrets) {
						leastTurrets = turretsFound;
						chosenBase = base;
					}
				}
				if (leastTurrets == 0) {
					m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(chosenBase->getInlineTurretPosition()));
				}
				else {
					
					srand(time(NULL));
					int randomo;
					randomo = rand() % 10 + 1;

					if (randomo < 5) {
						m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(chosenBase->m_mineralEdge1));
					}
					else {
						m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(chosenBase->m_mineralEdge2));
					}
				}
				
			}
			else if (item.type.getUnitType().getName() == "TERRAN_ENGINEERINGBAY" || item.type.getUnitType().getName() == "TERRAN_ARMORY") {
				for (auto & base : m_bot.Bases().getOccupiedBaseLocations(Players::Self)) {
					if (base->isStartLocation()) {
						m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(base->m_behindMineralLine));
						break;
					}
				}
			}
			 else if (item.type.getUnitType().getName() == "TERRAN_BUNKER") {

				 sc2::Point2D bPoint(0.0, 0.0);

				 if (m_bot.getDefendMainRamp()) {
					  bPoint = m_bot.Map().getBunkerPosition();
				 }
				 else {
					  bPoint = m_bot.Map().getNaturalBunkerPosition();
				 }

				CCTilePosition tp(bPoint.x, bPoint.y);
				m_buildingManager.addBuildingTask(item.type.getUnitType(), tp);


			}
			else {
				sc2::Point2D bPoint = m_bot.Map().getBunkerPosition();
				CCPosition basePos = m_bot.Bases().getPlayerStartingBaseLocation(0)->getPosition();

				CCPosition finalPosition = (basePos + basePos + CCPosition(bPoint.x,bPoint.y)) / 3;


				m_buildingManager.addBuildingTask(item.type.getUnitType(), Util::GetTilePosition(m_bot.Bases().getPlayerStartingBaseLocation(Players::Self)->getCenterOfBase()));
			}
            
        }
    }
    // if we're dealing with a non-building unit
    else if (item.type.isUnit())
    {
        producer.train(item.type.getUnitType());
    }
    else if (item.type.isUpgrade())
    {

		producer.upgrade(item.type);
    }
	
}

void ProductionManager::preposition(const Unit & producer, BuildOrderItem & item)
{
}

void ProductionManager::swap(BuildOrderItem & item)
{
	const UnitType originB = item.type.getSwapOriginBuildingType();
	const UnitType designatedB = item.type.getSwapDesignatedBuildingType();
	const UnitType designatedA = item.type.getSwapDesignatedAddonType();

	bool canSwapOriginB = false;
	bool canSwapDesignatedB = false;
	bool canSwapDesignatedA = false;

	bool onHoldOrigin = false;
	bool onHoldAddon = false;

	Unit u_originB;
	Unit u_designatedB;
	Unit u_designatedA;
	//if we are not already in the middle of a swap, begin
	if (!areWeSwapping) {
		for (Unit unit : m_bot.GetUnits()) {

			if (canSwapDesignatedA && canSwapDesignatedB && canSwapOriginB) {
				break;
			}

			if (unit.getPlayer() == Players::Self && unit.isValid()) {
				if (unit.getType() == originB) {

					if (unit.isCompleted()) {
						u_originB = unit;
						onHoldOrigin = false;
						canSwapOriginB = true;
					}
					else if (unit.getBuildPercentage() > 0.6 && unit.isBeingConstructed()) {
						if (!u_originB.isValid() || (u_originB.getBuildPercentage() < unit.getBuildPercentage())) {
							u_originB = unit;
							onHoldOrigin = true;
						}
					}
				}

				//if this is suitable designated position building,
				if (unit.getType() == designatedB && unit.isCompleted() && (!canSwapDesignatedA || !onHoldAddon)) {

					//go through all units to find addon close enough to the suitable buildings and check if this is the best combination find yet
					for (Unit unitt : m_bot.GetUnits()) {
						if (unitt.getPlayer() == Players::Self && unitt.isValid() && unitt.getType() == designatedA) {

							if (Util::Dist(unit.getPosition(), unitt.getPosition()) < 3) {

								if (unitt.isCompleted()) {
									u_designatedA = unitt;
									u_designatedB = unit;

									onHoldAddon = false;
									canSwapDesignatedA = true;
									canSwapDesignatedB = true;

									break;
								}
								else if (unitt.getBuildPercentage() > 0.3) {

									if (!u_designatedA.isValid() && !u_designatedB.isValid()) {
										u_designatedA = unitt;
										u_designatedB = unit;

									
										onHoldAddon = true;

									}
									else if (unitt.getBuildPercentage() > u_designatedA.getBuildPercentage()) {
										u_designatedA = unitt;
										u_designatedB = unit;

										
										onHoldAddon = true;
									}

								}
							}
						}
					}
				}
			}
		}

		

		if ((onHoldAddon && canSwapOriginB) || (onHoldAddon && onHoldOrigin) || (canSwapDesignatedA && onHoldOrigin)) {
			//waiting

		}
		else if (canSwapDesignatedA && canSwapDesignatedB && canSwapOriginB) {
			//executing the swap


			if (1 > u_originB.getUnitPtr()->build_progress > 0.9 || 1 > u_designatedB.getUnitPtr()->build_progress > 0.9) {
				//wait
			}
			else {

				while (u_originB.getUnitPtr()->build_progress < 1 || u_designatedB.getUnitPtr()->build_progress < 1) {
					u_originB.cancel();
					u_designatedB.cancel();
				}

				Unit * pointerA = &u_originB;
				Unit * pointerB = &u_designatedB;
				Unit * pointerC = &u_designatedA;

				m_buildingManager.executeLift(pointerA, pointerB, pointerC);
				areWeSwapping = true;


			}

		}
		else {
			//exiting without executing
			m_queue.removeCurrentHighestPriorityItem();
		}
	}
	 else {
		 //if we are in the middle of swapping, wait for finish

		 if (m_buildingManager.executeSwap()) {
			 areWeSwapping = false;
			 m_queue.removeCurrentHighestPriorityItem();
		 }
	}

}

void ProductionManager::ManageOCEnergy()
{
	int currentTotalEnergy = 0;
	int remainingReservedRequirement = m_bot.GetReservedEnergy();
	bool meetsReservedRequirement = false;

	int unitEnergy;
	for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
	{
		if (unit.getType().getName() == "TERRAN_ORBITALCOMMAND")
		{
			unitEnergy = unit.getEnergy();

			if (remainingReservedRequirement > 0) {
				while (unitEnergy >= 50 && remainingReservedRequirement > 0) {
					remainingReservedRequirement -= 50;
					unitEnergy -= 50;
				}
			}
			if (remainingReservedRequirement <= 0 && unitEnergy >= 50) {
				//std::cout << "Usable energy from this OC: " << floor(unitEnergy / 50) * 50 << "\n";

				
				unit.useMULE(Util::getClostestMineral(unit.getPosition(), m_bot));

			}
		}

	}
	
}

void ProductionManager::manageMacroLoop()
{
	if (m_queue.size() < 4) {

		
		priority_queue<priorityMetaType> newQueue;

		while (!metaTypePriorityQueue.empty()) {
			priorityMetaType top(metaTypePriorityQueue.top().priority, metaTypePriorityQueue.top().metaType);
			metaTypePriorityQueue.pop();

			int numberOfPossibleProducers = numberOfViableProducers(top.metaType);

			for (int i = numberOfPossibleProducers; i > 0; i--) {
				m_queue.queueAsLowestPriority(top.metaType, false);
			}

			newQueue.push(top);
		}
		metaTypePriorityQueue = newQueue;

	}
	
}




bool ProductionManager::canMakeNow(const Unit & producer, const MetaType & type)
{
    if (!producer.isValid() || !meetsReservedResources(type))
    {
		
        return false;
    }

#ifdef SC2API
    sc2::AvailableAbilities available_abilities = m_bot.Query()->GetAbilitiesForUnit(producer.getUnitPtr());

    // quick check if the unit can't do anything it certainly can't build the thing we want
    if (available_abilities.abilities.empty())
    {
		
		//std::cout << "empty abilities fail \n";
        
		return false;
    }
    else
    {
        // check to see if one of the unit's available abilities matches the build ability type
        sc2::AbilityID MetaTypeAbility = m_bot.Data(type).buildAbility;

		//std::cout << MetaTypeAbility.to_string() << " meta type ability that we are looking for \n";

        for (const sc2::AvailableAbility & available_ability : available_abilities.abilities)
        {
			//std::cout << available_ability.ability_id << " ability id we found \n";
            if (available_ability.ability_id == MetaTypeAbility)
            {
                return true;
            }
        }
    }

	//std::cout << "end method fail \n";
    return false;
#else
    bool canMake = meetsReservedResources(type);
	if (canMake)
	{
		if (type.isUnit())
		{
			canMake = BWAPI::Broodwar->canMake(type.getUnitType().getAPIUnitType(), producer.getUnitPtr());
		}
		else if (type.isTech())
		{
			canMake = BWAPI::Broodwar->canResearch(type.getTechType(), producer.getUnitPtr());
		}
		else if (type.isUpgrade())
		{
			canMake = BWAPI::Broodwar->canUpgrade(type.getUpgrade(), producer.getUnitPtr());
		}
		else
		{	
			BOT_ASSERT(false, "Unknown type");
		}
	}

	return canMake;
#endif
}

bool ProductionManager::detectBuildOrderDeadlock()
{
    return false;
}

int ProductionManager::getFreeMinerals()
{
    return m_bot.GetMinerals() - m_buildingManager.getReservedMinerals();
}

int ProductionManager::getFreeGas()
{
    return m_bot.GetGas() - m_buildingManager.getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool ProductionManager::meetsReservedResources(const MetaType & type)
{
    // return whether or not we meet the resources
    int minerals = m_bot.Data(type).mineralCost;
    int gas = m_bot.Data(type).gasCost;


    return (m_bot.Data(type).mineralCost <= getFreeMinerals()) && (m_bot.Data(type).gasCost <= getFreeGas());
}

void ProductionManager::drawProductionInformation()
{

    if (!m_bot.Config().DrawProductionInfo)
    {
        return;
    }

    std::stringstream ss;
    ss << "Production Information\n\n";

    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        if (unit.isBeingConstructed())
        {
            //ss << sc2::UnitTypeToName(unit.unit_type) << " " << unit.build_progress << "\n";
        }
    }

    ss << m_queue.getQueueInformation();

    m_bot.Map().drawTextScreen(0.01f, 0.01f, ss.str(), CCColor(255, 255, 0));
}

void ProductionManager::buildBunker()
{
	if (m_queue.getHighestPriorityItem().type.getName() != MetaType(std::string("Bunker"),m_bot).getName()) {

		std::cout << "Prioritizing new bunker\n";

		
		m_queue.queueAsHighestPriority(MetaType(std::string("Bunker"), m_bot), true);
	}
	else {
		//std::cout << "cant queue because there is bunker already? \n";
	}
}

void ProductionManager::buildTurret()
{
	if (m_queue.getHighestPriorityItem().type.getName() != MetaType(std::string("MissileTurret"), m_bot).getName()) {
		std::cout << "Prioritizing new turret\n";
		m_queue.queueAsHighestPriority(MetaType(std::string("MissileTurret"), m_bot), true);
	}
}

void ProductionManager::buildNow(const MetaType & type)
{
	m_queue.queueAsHighestPriority(type,true);
	
}

void ProductionManager::pushToMacroLoopQueue(const MetaType & type, int priority)
{
	std::cout << "Macro looping " << type.getName() << " \n";
	metaTypePriorityQueue.push(priorityMetaType(priority,type));
}

void ProductionManager::removeFromMacroLoopQueue(const MetaType & type)
{
	priority_queue<priorityMetaType> newQueue;

	while (!metaTypePriorityQueue.empty()) {
		priorityMetaType top(metaTypePriorityQueue.top().priority, metaTypePriorityQueue.top().metaType);


		metaTypePriorityQueue.pop();

		if (top.metaType.getName() == type.getName()) {
			//do nothing
		}
		else {
			//include in new list
			newQueue.push(top);
		}
	}
	metaTypePriorityQueue = newQueue;
}
