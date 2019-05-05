#pragma once

#include "Common.h"
#include "Timer.hpp"
#include "ProductionManager.h"
#include "ScoutManager.h"
#include "CombatCommander.h"
#include "ThreatMap.h"

class CCBot;

class GameCommander
{
    CCBot &                 m_bot;
    Timer                   m_timer;

    ProductionManager       m_productionManager;
    ScoutManager            m_scoutManager;
    CombatCommander         m_combatCommander;

    std::vector<Unit>    m_validUnits;
    std::vector<Unit>    m_combatUnits;
    std::vector<Unit>    m_scoutUnits;

    bool                    m_initialScoutSet;

    void assignUnit(const Unit & unit, std::vector<Unit> & units);
    bool isAssigned(const Unit & unit) const;

	int threatDetectionCD = 30;
	int threatDetectionCurrent = 0;
	int depotCDMax = 60;
	int depotCDNow = 0;
	int upgradeCDMax = 300;
	int upgradeCDNow = 0;

public:

    GameCommander(CCBot & bot);

    void onStart();
    void onFrame();

    void handleUnitAssignments();
    void setValidUnits();
    void setScoutUnits();
    void setCombatUnits();

    void drawDebugInterface();
    void drawGameInformation(int x, int y);

    bool shouldSendInitialScout();

    void onUnitCreate(const Unit & unit);
    void onUnitDestroy(const Unit & unit);

	void detectCurrentThreats();
	void manageThreatMap();
};
