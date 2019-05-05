#include "ThreatMap.h"
#include "CCBot.h"
#include "Util.h"

int ThreatMap::threatLevel(int x, int y) const
{
	int rwidth = (int)m_threatMap.size();
	int rheight = (int)m_threatMap[0].size();
	if (x < 0 || y < 0 || x >= rwidth || y >= rheight)
	{
		return false;
	}

	return m_threatMap[x][y];

}

CCColor ThreatMap::getColor(int threat) const
{
	
	int r = 0;
	int g = 0;
	int b = 0;

	if (threat > 900) {
		r = 230;
		g = 10;
		b = 4;
	}
	else if (threat > 800) {
		r = 230;
		g = 40;
		b = 2;
	}
	else if (threat > 700) {
		r = 231;
		g = 80;
		b = 0;
	}
	else if (threat > 600) {
		r = 232;
		g = 116;
		b = 0;
	}
	else if (threat > 500) {
		r = 233;
		g = 150;
		b = 0;
	}
	else if (threat > 400) {
		r = 235;
		g = 190;
		b = 0;
	}
	else if (threat > 300) {
		r = 240;
		g = 230;
		b = 0;
	}
	else if (threat > 200) {
		r = 210;
		g = 240;
		b = 0;
	}
	else if (threat > 100) {
		r = 170;
		g = 240;
		b = 0;
	}
	else if (threat > 0) {
		r = 135;
		g = 255;
		b = 0;
	}

	return CCColor(r, g, b);;
}

ThreatMap::ThreatMap(CCBot & bot) : m_bot(bot)
{
}

void ThreatMap::onStart()
{
	m_threatMap = std::vector< std::vector<int> >(m_bot.Map().width(), std::vector<int>(m_bot.Map().height(), false));

}

void ThreatMap::drawThreatMap()
{
	if (!m_bot.Config().DrawThreatMap)
	{
		return;
	}

	int rwidth = (int)m_threatMap.size();
	int rheight = (int)m_threatMap[0].size();

	for (int x = 0; x < rwidth; ++x)
	{
		for (int y = 0; y < rheight; ++y)
		{
			if (m_threatMap[x][y] > 0)
			{
				m_bot.Map().drawTile(x, y, getColor(m_threatMap[x][y]));
				m_bot.Map().drawText(CCPosition(x, y), std::to_string(m_threatMap[x][y]));
			}
		}
	}
	std::stringstream dss;
	dss << "Threat tolerance: " << m_bot.getThreatTolerance();

	m_bot.Map().drawTextScreen(0.75f, 0.10f, dss.str(), getColor(m_bot.getThreatTolerance()));
}

void ThreatMap::cleanMap()
{
	for (int i = 0; i < m_threatMap.size(); i++) {
		for (int j = 0; j < m_threatMap.size(); j++) {
			m_threatMap[i][j] = 0;
		}
	}
}

void ThreatMap::setThreatAt(int x, int y, int spaceInner, int spaceOuter, int threatInner, int threatOuter)
{
	int rwidth = (int)m_threatMap.size();
	int rheight = (int)m_threatMap[0].size();

	//std::cout << "rwidth " << rwidth << " rheight " << rheight << "\n";

	if (spaceInner <= 0 && spaceOuter <= 0 && threatInner <= 0 && threatOuter <= 0) {
		std::cout << "THREAT MAP ERROR, bad definition\n";
	}
	//std::cout << " trying  to set a threat " << x << " " << y << " \n";

	if (!((x - spaceOuter - spaceInner) < 0 || (y - spaceOuter - spaceInner) < 0 ||
		(x + spaceOuter + spaceInner >= rwidth) || (y + spaceOuter + spaceInner) >= rheight)) {

		//std::cout << "setting threat for x " << x << " y " << y << "\n";
		//set outer space

		//define the box
		for (int i = x - spaceOuter - spaceInner; i <= x + spaceOuter + spaceInner; i++) {
			for (int j = y - spaceOuter - spaceInner; j <= y + spaceOuter + spaceInner; j++) {

				//if it belongs to the inner space
				if (i >= x - spaceInner && i <= x + spaceInner &&
					j >= y - spaceInner && j <= y + spaceInner) {
					//std::cout << "setting inner threat\n";
					
					m_threatMap[i][j] += threatInner;
				} //else it belongs to outer ring
				else {
					//std::cout << "setting outer threat\n";
					m_threatMap[i][j] += threatOuter;
				}
				//std::cout << "set x: " << x << " y: " << y << " i " << i << " j " << j << "\n";
			}
		}

	}
}

bool ThreatMap::canBuildHereWithTolerance(int x, int y, int height, int width, int maxTolerance)
{
	int maxThreatLevelFound = -1;
	bool can = true;

	for (int i = x - 1 ; i < x + width - 1; i++) {
		for (int j = y - 1; j < y + height - 1; j++) {

			if (m_threatMap[i][j] > maxThreatLevelFound) {

				maxThreatLevelFound = m_threatMap[i][j];
				if (maxThreatLevelFound > maxTolerance) {
					can = false;
					break;
				}
			}
		}
		if (!can) {
			break;
		}
	}
	return can;
}

int ThreatMap::getThreatAt(int x, int y)
{
	int rwidth = (int)m_threatMap.size();
	int rheight = (int)m_threatMap[0].size();
	if (x < 0 || y < 0 || x >= rwidth || y >= rheight)
	{
		return 0;
	}

	return m_threatMap[x][y];
}
