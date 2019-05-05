#pragma once

#include "Common.h"

#include "MapTools.h"
#include "BaseLocationManager.h"
#include "UnitInfoManager.h"
#include "WorkerManager.h"
#include "BotConfig.h"
#include "GameCommander.h"
#include "BuildingManager.h"
#include "StrategyManager.h"
#include "TechTree.h"
#include "MetaType.h"
#include "Unit.h"
#include "ThreatMap.h"

#ifdef SC2API
class CCBot : public sc2::Agent 
#else
class CCBot
#endif
{
    MapTools                m_map;
    BaseLocationManager     m_bases;
    UnitInfoManager         m_unitInfo;
    WorkerManager           m_workers;
    StrategyManager         m_strategy;
    BotConfig               m_config;
    TechTree                m_techTree;
    GameCommander           m_gameCommander;
	ThreatMap				m_threatMap;


    std::vector<Unit>       m_allUnits;
    std::vector<CCPosition> m_baseLocations;

	int reservedEnergy = 0;//custom	
	int threatLevel = 3;//custom
	bool underAttack = false; //custom
	bool defendMainRamp = false; //custom
	int threatTolerance = -1;
	

	bool expandingProhibited = false;

    void setUnits();

#ifdef SC2API
    void OnError(const std::vector<sc2::ClientError> & client_errors, 
                 const std::vector<std::string> & protocol_errors = {}) override;
#endif

public:

    CCBot();

#ifdef SC2API
    void OnGameStart() override;
    void OnStep() override;
#else
    void OnGameStart();
    void OnStep();
#endif

          BotConfig & Config();
          WorkerManager & Workers();
    const BaseLocationManager & Bases() const;
    const MapTools & Map() const;
	      ThreatMap & ThreatMap();
    const UnitInfoManager & UnitInfo() const;
    const StrategyManager & Strategy() const;
    const TypeData & Data(const UnitType & type) const;
    const TypeData & Data(const CCUpgrade & type) const;
    const TypeData & Data(const MetaType & type) const;
    const TypeData & Data(const Unit & unit) const;
    CCRace GetPlayerRace(int player) const;
    CCPosition GetStartLocation() const;

	
	int GetReservedEnergy(); //custom
	void increaseReservedEnergy(); //c
	void decreaseReservedEnergy(); //c
	int GetThreatLevel(); //c
	bool getDefendMainRamp();//c
	void setThreatTolerance(int tolerance);
	int getThreatTolerance();

	bool isExpandingProhibited();
	void setThreatLevel(int to);
	void setDefendMainRamp(bool value);

	bool isUnderAttack();

    int GetCurrentFrame() const;
    int GetMinerals() const;
    int GetCurrentSupply() const;
    int GetMaxSupply() const;
    int GetGas() const;
    Unit GetUnit(const CCUnitID & tag) const;
    const std::vector<Unit> & GetUnits() const;
    const std::vector<CCPosition> & GetStartLocations() const;


};