#include "GameCommander.h"
#include "CCBot.h"
#include "Util.h"

GameCommander::GameCommander(CCBot & bot)
    : m_bot                 (bot)
    , m_productionManager   (bot)
    , m_scoutManager        (bot)
    , m_combatCommander     (bot)
    , m_initialScoutSet     (false)
{

}

void GameCommander::onStart()
{
    m_productionManager.onStart();
    m_scoutManager.onStart();
    m_combatCommander.onStart();

	m_bot.setThreatTolerance(800);
}

void GameCommander::onFrame()
{
    m_timer.start();

    handleUnitAssignments();

    m_productionManager.onFrame();

    m_scoutManager.onFrame();
    m_combatCommander.onFrame(m_combatUnits);

	//detectCurrentThreats();
	manageThreatMap();

	drawDebugInterface();

}

void GameCommander::drawDebugInterface()
{
    drawGameInformation(4, 1);
}

void GameCommander::drawGameInformation(int x, int y)
{
    std::stringstream ss;
    ss << "Players: " << "\n";
    ss << "Strategy: " << m_bot.Config().StrategyName << "\n";
    ss << "Map Name: " << "\n";
    ss << "Time: " << "\n";
}

// assigns units to various managers
void GameCommander::handleUnitAssignments()
{
    m_validUnits.clear();
    m_combatUnits.clear();

    // filter our units for those which are valid and usable
    setValidUnits();

    // set each type of unit
    setScoutUnits();
    setCombatUnits();
}

bool GameCommander::isAssigned(const Unit & unit) const
{
    return     (std::find(m_combatUnits.begin(), m_combatUnits.end(), unit) != m_combatUnits.end())
        || (std::find(m_scoutUnits.begin(), m_scoutUnits.end(), unit) != m_scoutUnits.end());
}

// validates units as usable for distribution to various managers
void GameCommander::setValidUnits()
{
    // make sure the unit is completed and alive and usable
    for (auto & unit : m_bot.UnitInfo().getUnits(Players::Self))
    {
        m_validUnits.push_back(unit);
    }
}

void GameCommander::setScoutUnits()
{
    // if we haven't set a scout unit, do it
    if (m_scoutUnits.empty() && !m_initialScoutSet)
    {
        // if it exists
        if (shouldSendInitialScout())
        {
            // grab the closest worker to the supply provider to send to scout
            Unit workerScout = m_bot.Workers().getClosestMineralWorkerTo(m_bot.GetStartLocation());

            // if we find a worker (which we should) add it to the scout units
            if (workerScout.isValid())
            {
                m_scoutManager.setWorkerScout(workerScout);
                assignUnit(workerScout, m_scoutUnits);
                m_initialScoutSet = true;
            }
        }
    }
}

bool GameCommander::shouldSendInitialScout()
{
    return m_bot.Strategy().scoutConditionIsMet();
}

// sets combat units to be passed to CombatCommander
void GameCommander::setCombatUnits()
{
    for (auto & unit : m_validUnits)
    {
        BOT_ASSERT(unit.isValid(), "Have a null unit in our valid units\n");

        if (!isAssigned(unit) && unit.getType().isCombatUnit())
        {
            assignUnit(unit, m_combatUnits);
        }
    }
}

void GameCommander::onUnitCreate(const Unit & unit)
{

}

void GameCommander::onUnitDestroy(const Unit & unit)
{
    //_productionManager.onUnitDestroy(unit);
}

void GameCommander::detectCurrentThreats()
{

	//m_bot.setDefendMainRamp(true);

	if (threatDetectionCD <= threatDetectionCurrent) {

		

		int mutaliskThreat = 0;
		int roachThreat = 0;
		int zerglingThreat = 0;

		int numberOfBunkers = 0;
		int numberOfTurrets = 0;
		int numberOfBases = 0;


		for (Unit unitt : m_bot.GetUnits()) {
			
			if (unitt.getPlayer() == Players::Enemy && unitt.isValid()) {
				if (unitt.getType().getName() == "ZERG_ZERGLING") {
					zerglingThreat++;
				}
				if (unitt.getType().getName() == "ZERG_ROACH") {
					roachThreat++;
				}
				if (unitt.getType().getName() == "ZERG_MUTALISK" ||
					unitt.getType().getName() == "ZERG_SPIRE") {
					mutaliskThreat++;
				}

			}
			else if (unitt.getPlayer() == Players::Self && unitt.isValid()) {
				if (unitt.getType().getName() == "TERRAN_MISSILETURRET") {
					numberOfTurrets++;
				}
				else if (unitt.getType().getName() == "TERRAN_BUNKER")
				{
					numberOfBunkers++;
				}
			}
		}
		numberOfBases = m_bot.Bases().getOccupiedBaseLocations(Players::Self).size();

		//std::cout << "detecting threat zergling " << zerglingThreat << " roach " << roachThreat << " mutalisk " << mutaliskThreat << " \n" ; 

		bool noThreat = true;
		int numberOfBunkersNeeded = 0;
		int numberOfTurretsPerBase = 0;

		//calculate roach, ling, muta threat
		if (zerglingThreat > 20) {
			m_bot.setDefendMainRamp(true);

			if (m_bot.GetThreatLevel() < 3) {
				std::cout << "# Detected strategy Zergling rush! " << zerglingThreat << "+ of zerglings on the field!\n";
				std::cout << "# Threat level risen to critical value - 3!\n";
				std::cout << "# Aborting old strategy, defining new one\n";
				std::cout << "# Bunker location is set at main ramp\n";
				std::cout << "# Workers are preparing for repairs!\n";

				m_productionManager.freeBuildOrderQueue();



				m_productionManager.buildNow(MetaType("SupplyDepot", m_bot));
				m_productionManager.buildNow(MetaType("Barracks",m_bot));
				m_productionManager.buildNow(MetaType("Marine", m_bot));
				m_productionManager.buildNow(MetaType("Marine", m_bot));


				m_productionManager.pushToMacroLoopQueue(MetaType("Marine", m_bot),20);
				m_productionManager.pushToMacroLoopQueue(MetaType("SCV", m_bot), 10);

				numberOfBunkersNeeded = 2;
				m_bot.setThreatLevel(3);

			}
			else {

			}

			
			noThreat = false;
		}
		else if (zerglingThreat > 8) {

			if (m_bot.GetThreatLevel() < 2) {

				std::cout << "# Detected moderate amount of Zerglings: " << zerglingThreat << "+ on the field!\n";
				std::cout << "# Threat level changed to 2\n";
				std::cout << "# Bunker location is set at main ramp\n";

				m_bot.setDefendMainRamp(true);
				numberOfBunkersNeeded = 1;
				m_bot.setThreatLevel(2);

			}

			
			noThreat = false;
		}
		else if (zerglingThreat > 4) {

			if (m_bot.GetThreatLevel() < 1) {

				std::cout << "# Detected small amount of Zerglings: " << zerglingThreat << "+ on the field\n";
				std::cout << "# Threat level changed to 1\n";
				std::cout << "# Bunker location is set at main ramp\n";

				m_bot.setDefendMainRamp(true);
				numberOfBunkersNeeded = 0;
				m_bot.setThreatLevel(1);

			}

			noThreat = false;

		}

		if (roachThreat > 12) {
			if (m_bot.GetThreatLevel() != 3) {

				std::cout << "# Detected possible strategy: Roach bust; " << roachThreat << "+ roaches on the field.";
				std::cout << "# Threat level changed to 3\n";
				std::cout << "# Bunker location is set at natural expansion\n";
				m_bot.setThreatLevel(3);
				numberOfBunkersNeeded = 3;
			}
			noThreat = false;
		}
		else if (roachThreat > 8) {
			if (m_bot.GetThreatLevel() < 2) {
				std::cout << "# Detected moderate amount of Roaches: " << roachThreat << "+ on the field\n";
				std::cout << "# Threat level changed to 2\n";
				std::cout << "# Bunker location is set at natural expansion\n";

				m_bot.setThreatLevel(2);
				numberOfBunkersNeeded = 2;
			}
			noThreat = false;
		}
		else if (roachThreat > 4) {
			if (m_bot.GetThreatLevel() != 1) {
				std::cout << "# Detected small amount of Roaches: " << roachThreat << "+ on the field\n";
				std::cout << "# Threat level changed to 1\n";
				std::cout << "# Bunker location is set at natural expansion\n";

				m_bot.setThreatLevel(1);
				numberOfBunkersNeeded = 1;
			}
			noThreat = false;
		}

		if (mutaliskThreat > 10) {
			numberOfTurretsPerBase = 3;
			if (m_bot.GetThreatLevel() < 3) {
				m_bot.setThreatLevel(3);

				std::cout << "# Detected Mutalisk strategy. Current threat is " << mutaliskThreat << "\n";
				std::cout << "# Number of turrets per base is advised to be: " << numberOfTurretsPerBase << "\n";
				std::cout << "# Threat level changed to 3\n";

			}
			noThreat = false;
		}
		else if (mutaliskThreat > 5) {

			numberOfTurretsPerBase = 2;
			if (m_bot.GetThreatLevel() < 2) {

				m_bot.setThreatLevel(2);

				std::cout << "# Detected moderate amount of mutalisks. Mutalisk threat is " << mutaliskThreat << "\n";
				std::cout << "# Number of turrets per base is advised to be: " << numberOfTurretsPerBase << "\n";
				std::cout << "# Threat level changed to 2\n";
				
			}
			
			noThreat = false;
		}
		else if (mutaliskThreat > 0) {

			numberOfTurretsPerBase = 1;
			if (m_bot.GetThreatLevel() < 1) {


				m_bot.setThreatLevel(1);

				std::cout << "# Detected possible Mutalisk strategy, mutalisk threat is " << mutaliskThreat << "\n";
				std::cout << "# Number of turrets per base is advised to be: " << numberOfTurretsPerBase << "\n";
				std::cout << "# Threat level changed to 1\n";

			}
			
			noThreat = false;
		}

		for (int i = numberOfBunkers; i < numberOfBunkersNeeded; i++) {
			//queue new building
			std::cout << "# Building a new bunker!\n";
			m_productionManager.buildBunker();
		}

		//std::cout << "number of turrets " << numberOfTurrets << " per base " << numberOfTurretsPerBase << " bases " << numberOfBases << " muta th " << mutaliskThreat << "\n";
		if (numberOfTurrets < (numberOfTurretsPerBase * numberOfBases)) {
			//queue new building
			std::cout << "# Building a new turret!\n";
			m_productionManager.buildTurret();
		}


		if (noThreat) {
			if (m_bot.GetThreatLevel() > 0) {
				std::cout << "No threat detected!\n";
				m_bot.setThreatLevel(0);
			}
			
			// m_bot.setDefendMainRamp(false);
		}

		for (Unit u : m_bot.UnitInfo().getUnits(Players::Self)) {
			if (u.getType().getName() == "TERRAN_MARINE" &&
				u.isValid()) {
				for (Unit bunk : m_bot.UnitInfo().getUnits(Players::Self)) {
					if (bunk.getType().getName() == "TERRAN_BUNKER") {
						if (!(bunk.getUnitPtr()->cargo_space_taken >= bunk.getUnitPtr()->cargo_space_max)) {
							u.rightClick(bunk);
							break;
						}
					}
				}
			}
		}

		threatDetectionCurrent = 0;
	}
	else {
		threatDetectionCurrent++;
	}
	//testing method for macro module

	if (m_bot.Observation()->GetFoodUsed() + 20 > m_bot.Observation()->GetFoodCap() && m_bot.Observation()->GetFoodCap() < 199
		&& depotCDNow >= depotCDMax && m_productionManager.numberOfAutomatedItems() >= 1) {
		//std::cout << "building two emergency depots \n";
		m_productionManager.buildNow(MetaType("SupplyDepot", m_bot));
		//m_productionManager.buildNow(MetaType("SupplyDepot", m_bot));
		depotCDNow = 0;
	}
	else {
		depotCDNow++;
	}

	if (m_productionManager.numberOfQueuedItems() <= 1 && m_productionManager.numberOfAutomatedItems() == 0) {
		m_productionManager.pushToMacroLoopQueue(MetaType("Marine", m_bot), 10);
		m_productionManager.pushToMacroLoopQueue(MetaType("Medivac", m_bot), 9);
		m_productionManager.pushToMacroLoopQueue(MetaType("SCV", m_bot), 8);
	}
	/**
	if (m_productionManager.numberOfAutomatedItems() >= 1) {
		if (upgradeCDNow >= upgradeCDMax) {

			for (Unit unitt : m_bot.GetUnits()) {
				if (unitt.isValid() && unitt.getPlayer() == Players::Self && !unitt.isBeingConstructed()) {
					if (unitt.getType().getName() == "TERRAN_ENGINEERINGBAY") {
						if (unitt.getNumberOfOrders() == 0) {
							m_productionManager.buildNow(MetaType("TerranInfantryWeaponsLevel1", m_bot));
							m_productionManager.buildNow(MetaType("TerranInfantryArmorsLevel1", m_bot));
						}
					}
				}
			}
			upgradeCDNow = 0;
		}
		else {
			upgradeCDNow++;
		}
		
	}
	*/

}

void GameCommander::manageThreatMap()
{
	m_bot.ThreatMap().cleanMap();

	for (auto u : m_bot.UnitInfo().getUnits(Players::Enemy)) {

		if (u.getType().getName() == "ZERG_ZERGLING") {
			m_bot.ThreatMap().setThreatAt(u.getPosition().x,u.getPosition().y,2,2,550,150);
			
		} 

		if (u.getType().getName() == "ZERG_ULTRALISK") {
			m_bot.ThreatMap().setThreatAt(u.getPosition().x, u.getPosition().y, 3, 2, 550, 150);
		}

		if (u.getType().getName() == "ZERG_MUTALISK") {
			m_bot.ThreatMap().setThreatAt(u.getPosition().x, u.getPosition().y, 2, 2, 190, 90);
		}

	}
	m_bot.ThreatMap().drawThreatMap();
}


void GameCommander::assignUnit(const Unit & unit, std::vector<Unit> & units)
{
    if (std::find(m_scoutUnits.begin(), m_scoutUnits.end(), unit) != m_scoutUnits.end())
    {
        m_scoutUnits.erase(std::remove(m_scoutUnits.begin(), m_scoutUnits.end(), unit), m_scoutUnits.end());
    }
    else if (std::find(m_combatUnits.begin(), m_combatUnits.end(), unit) != m_combatUnits.end())
    {
        m_combatUnits.erase(std::remove(m_combatUnits.begin(), m_combatUnits.end(), unit), m_combatUnits.end());
    }

    units.push_back(unit);
}
