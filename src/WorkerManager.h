#pragma once

#include "WorkerData.h"

class Building;
class CCBot;

class WorkerManager
{
    CCBot & m_bot;

    mutable WorkerData  m_workerData;
    Unit m_previousClosestWorker;

    void setMineralWorker(const Unit & unit);
    
    void handleIdleWorkers();
    void handleGasWorkers();
    void handleRepairWorkers();

	void handleMineralWorkers();	//custom method, worker distribution amongst bases
	void updateStartingWorkers();	//custom method, assign starting workers as mineral workers without sending them grouped to 1 mineral.
public:

    WorkerManager(CCBot & bot);

    void onStart();
    void onFrame();

    void finishedWithWorker(const Unit & unit);
    void drawResourceDebugInfo();
    void drawWorkerInformation();
    void setScoutWorker(Unit worker);
    void setCombatWorker(Unit worker);
    void setBuildingWorker(Unit worker, Building & b);
    void setRepairWorker(Unit worker,const Unit & unitToRepair);
    void stopRepairing(Unit worker);

    int  getNumMineralWorkers();
    int  getNumGasWorkers();
    bool isWorkerScout(Unit worker) const;
    bool isFree(Unit worker) const;
    bool isBuilder(Unit worker) const;

    Unit getBuilder(Building & b,bool setJobAsBuilder = true) const;
	Unit getRepairWorker(const CCPosition & pos, Unit & u) const;

    Unit getClosestDepot(CCPosition unit) const;
    Unit getGasWorker(Unit refinery) const;
    Unit getClosestMineralWorkerTo(const CCPosition & pos) const;
};

