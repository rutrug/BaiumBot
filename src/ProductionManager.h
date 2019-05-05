#pragma once

#include "Common.h"
#include "BuildOrder.h"
#include "BuildingManager.h"
#include "BuildOrderQueue.h"
#include <stdlib.h>
#include <time.h>
#include <queue> 

using namespace std;

class CCBot;

struct priorityMetaType {
	int priority;
	MetaType metaType;

	priorityMetaType(int priority, const MetaType metaType) : priority(priority), metaType(metaType) 
	{}

	bool operator<(const struct priorityMetaType & other) const {
		return priority < other.priority;
	}

	bool operator>(const struct priorityMetaType & other) const {
		return priority > other.priority;
	}

	void operator=(const struct priorityMetaType & other) {
		priority = other.priority;
		metaType = other.metaType;
	}

};

class ProductionManager
{
	CCBot &       m_bot;

	BuildingManager m_buildingManager;
	BuildOrderQueue m_queue;

	priority_queue<priorityMetaType> metaTypePriorityQueue; 


	bool areWeSwapping = false;

	Unit    getClosestUnitToPosition(const std::vector<Unit> & units, CCPosition closestTo);
	bool    meetsReservedResources(const MetaType & type);
	bool    canMakeNow(const Unit & producer, const MetaType & type);
	bool    detectBuildOrderDeadlock();
	void    setBuildOrder(const BuildOrder & buildOrder);
	void    create(const Unit & producer, BuildOrderItem & item);

	void	preposition(const Unit & producer, BuildOrderItem & item); //custom, prepull for workers?
	void	swap(BuildOrderItem & item);//custom
	void	ManageOCEnergy();//custom
	void	manageMacroLoop(); //c
	

	void    manageBuildOrderQueue();
	int     getFreeMinerals();
	int     getFreeGas();

	void    fixBuildOrderDeadlock();
	int		deadlockValue = 0;

	int		manageBOcdMax = 3;
	int		manageBOcdCurrent = 0;

public:

	ProductionManager(CCBot & bot);

	void    onStart();
	void    onFrame();
	void    onUnitDestroy(const Unit & unit);
	void    drawProductionInformation();

	void	buildBunker();
	void	buildTurret();

	void	buildNow(const MetaType & type); //c
	void	pushToMacroLoopQueue(const MetaType & type, int priority); //c
	void	removeFromMacroLoopQueue(const MetaType & type); //c
	void	freeBuildOrderQueue(); //c
	int		numberOfAutomatedItems();
	int		numberOfQueuedItems();

    Unit getProducer(const MetaType & type, CCPosition closestTo = CCPosition(0, 0), bool queueLimit = false);
	int numberOfViableProducers(const MetaType & type, CCPosition closestTo = CCPosition(0, 0));
};
