/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleMapGenerator.h"

CSimpleMapGenerator::CSimpleMapGenerator(unsigned int mapSeed) : CMapGenerator(mapSeed)
{
	GenerateInfo();
}

CSimpleMapGenerator::~CSimpleMapGenerator()
{

}

void CSimpleMapGenerator::GenerateInfo()
{
	mapSize = int2(5, 5);

	SetMapName("My Generated Map");
}

void CSimpleMapGenerator::GenerateMap()
{
	startPositions.push_back(int2(20, 20));
	startPositions.push_back(int2(500, 500));

	mapDescription = "My Map Description";

	int2 gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	for(int x = 0; x < gs.x; x++)
	{
		for(int y = 0; y < gs.y; y++)
		{
			map[y * gs.x + x] = 50.0f;
		}
	}
}
