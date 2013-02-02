/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SIMPLE_MAP_GENERATOR_H_
#define _SIMPLE_MAP_GENERATOR_H_

#include "MapGenerator.h"

class CSimpleMapGenerator : public CMapGenerator
{
public:
	CSimpleMapGenerator(const CGameSetup* setup);
	virtual ~CSimpleMapGenerator();

private:
	void GenerateInfo();
	void GenerateStartPositions();
	virtual void GenerateMap();
	void PostProcess();
	void PlacePOI();

	//height generation functions
	void RaiseArea(int2 p, float delta, float radius, float variance, float smoothness, float curl = 0.1f);
	void RaiseHill(int2 p, float delta, float radius);
	void FlattenArea(int2 p, float radius, float smoothness);
	void GetMinMaxRadius(int2 p, float radius, int2* px, int2* py, int div = 1);
	void LinkPoints(int2 p1, int2 p2, float radius, float variance, float smoothness);
	void PlaceMetalSpot(int2 p, float amount, float radius = 8.0f);

	virtual const std::vector<int2>& GetStartPositions() const
	{ return startPositions; }

	virtual const std::string& GetMapDescription() const
	{ return mapDescription; }

	std::vector<int2> startPositions;
	std::string mapDescription;
};

#endif // _SIMPLE_MAP_GENERATOR_H_
