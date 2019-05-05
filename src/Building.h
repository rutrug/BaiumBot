#pragma once

#include "Common.h"
#include "Unit.h"
#include "UnitType.h"

namespace BuildingStatus
{
    enum { Unassigned = 0, Assigned = 1, UnderConstruction = 2, Size = 3 };
}

class Building
{
public:

    CCTilePosition  desiredPosition;
    CCTilePosition  finalPosition;
    CCTilePosition  position;
    UnitType        type;
    Unit            buildingUnit;
	
    Unit            builderUnit;
    size_t          status;
    int             lastOrderFrame;
    bool            buildCommandGiven;
    bool            underConstruction;
	int				maxDelay = 10;
	int				currentDelay;
	bool			hasSkipped = false;
	int				numberOfAssignedWorkers;

	std::vector<Unit> m_repairUnits;

    Building();

    // constructor we use most often
    Building(UnitType t, CCTilePosition desired);
	Building(Unit buildingUnit);

    // equals operator
    bool operator == (const Building & b);
};
