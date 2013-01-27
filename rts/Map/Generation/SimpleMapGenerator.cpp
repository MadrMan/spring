/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleMapGenerator.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/FastMath.h"
#include "Map/SMF/SMFFormat.h"

CSimpleMapGenerator::CSimpleMapGenerator(const CGameSetup* setup) : CMapGenerator(setup)
{
	GenerateInfo();
}

CSimpleMapGenerator::~CSimpleMapGenerator()
{

}

void CSimpleMapGenerator::GenerateInfo()
{
	SetMapSize(int2(5, 5));
	GenerateStartPositions();

	SetMapName("My Generated Map");
}

void CSimpleMapGenerator::GenerateMap()
{
	mapDescription = "My Map Description";

	int2 gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	for(int x = 0; x < gs.x; x++)
	{
		for(int y = 0; y < gs.y; y++)
		{
			map[y * gs.x + x] = 0.0f;
		}
	}
}

void CSimpleMapGenerator::GenerateStartPositions()
{
	//Algorithm here is fairly simple:
	// 1. Start at random point in map
	// 2. Place all team members of team x in that area
	// 3. Rotate trough map 360/teams degrees
	// 4. Back to step 2
	const int2& gs = GetGridSize();

	int allies = setup->allyStartingData.size();
	int teams = setup->teamStartingData.size();
	
	float startAngle = GetRndFloat() * fastmath::PI2;
	const float angleIncrement = fastmath::PI2 / allies;
	const int moveLimit = (gs.x + gs.y) / (6 + allies * 2);
	const int borderDistance = 16;

	for(int x = 0; x < teams; x++)
	{
		const TeamBase& team = setup->teamStartingData[x];
		float allyAngle = startAngle + angleIncrement * team.teamAllyteam;

		float px = cos(allyAngle);
		float py = sin(allyAngle);

		//Move px/py to somewhere near the edge (between 70% and 80%)
		px *= (0.6f + GetRndFloat() * 0.3f);
		py *= (0.6f + GetRndFloat() * 0.3f);

		//Scale px/py to map size
		//Translate positions to have their origin at 0,0 instead of map center
		int ipx = (px * (gs.x / 2)) + gs.x / 2;
		int ipy = (py * (gs.y / 2)) + gs.y / 2;

		//Move the start positions around a bit
		//The less allies are playing, the more players get moved
		//Make sure start pos always stay away a certain distance from the map border
		int moveMinX = std::min(ipx - borderDistance, moveLimit);
		int moveMinY = std::min(ipy - borderDistance, moveLimit);
		int moveMaxX = std::min(gs.x - ipx - borderDistance, moveLimit);
		int moveMaxY = std::min(gs.y - ipy - borderDistance, moveLimit);
		ipx += GetRndInt(-moveMinX, moveMaxX);
		ipy += GetRndInt(-moveMinY, moveMaxY);

		//Add position for player
		startPositions.push_back(int2(ipx * GRID_SQUARE_SIZE, ipy * GRID_SQUARE_SIZE));
	}
}