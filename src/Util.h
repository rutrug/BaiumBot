#pragma once

#include "Common.h"
#include "UnitType.h"

class CCBot;
class Unit;

namespace Util
{
    CCRace          GetRaceFromString(const std::string & str);
    CCTilePosition  GetTilePosition(const CCPosition & pos);
    CCPosition      GetPosition(const CCTilePosition & tile);
    std::string     GetStringFromRace(const CCRace & race);
    bool            UnitCanMetaTypeNow(const Unit & unit, const UnitType & type, CCBot & m_bot);
    UnitType        GetTownHall(const CCRace & race, CCBot & bot);
    UnitType        GetRefinery(const CCRace & race, CCBot & bot);
    UnitType        GetSupplyProvider(const CCRace & race, CCBot & bot);
    CCPosition      CalcCenter(const std::vector<Unit> & units);
    bool            IsZerg(const CCRace & race);
    bool            IsProtoss(const CCRace & race);
    bool            IsTerran(const CCRace & race);
    CCPositionType  TileToPosition(float tile);

	const Unit		getClostestMineral(sc2::Point2D pos, CCBot & bot); //custom

#ifdef SC2API
    sc2::BuffID     GetBuffFromName(const std::string & name, CCBot & bot);
    sc2::AbilityID  GetAbilityFromName(const std::string & name, CCBot & bot);
#endif

    float Dist(const Unit & unit, const CCPosition & p2);
    float Dist(const Unit & unit1, const Unit & unit2);
    float Dist(const CCPosition & p1, const CCPosition & p2);
    CCPositionType DistSq(const CCPosition & p1, const CCPosition & p2);

	float DistSq(const sc2::Point2D & p1);
	float fastsqrt(const float S);
	float Dist(const sc2::Point2D pos);
	sc2::Point2D normalizeVector(const sc2::Point2D pos, const float length = 1.0f);
	CCPosition	prolongDirection(CCPosition from, CCPosition to, double percentage);
};
