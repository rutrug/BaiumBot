#pragma once
#include "Common.h"

class CCBot;

class ThreatMap {

	CCBot & m_bot;

	std::vector< std::vector<int> > m_threatMap;

	int threatLevel(int x, int y) const;
	CCColor getColor(int threat) const;

public:

	ThreatMap(CCBot & bot);

	void onStart();
	void drawThreatMap();

	void cleanMap();
	void setThreatAt(int x, int y, int spaceInner, int spaceOuter, int threatInner, int threatOuter);

	bool canBuildHereWithTolerance(int x, int y, int height, int width, int maxTolerance);
	
	int getThreatAt(int x, int y);


};