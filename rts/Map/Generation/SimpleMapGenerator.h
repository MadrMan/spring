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
	struct SFlattenSpot
	{
		int2 p;
		float radius;
		float height;
		float tolerance;
		float steepness;
	};

	void GenerateMap() override;

	// Map generation functions
	void GenerateInfo();
	void GenerateStartPositions2Teams();
	void GenerateStartPositionsFFA();
	void GenerateMapText();
	void PostProcess();

	void PlacePOIHill(int2 p, float height, float radius);
	void PlacePOI();

	// Height generation functions
	void RaiseArea(int2 p, float delta, float radius, float variance, float smoothness, int flattens = 0, int mspots = 0, float curl = 0.4f);
	void RaiseHill(int2 p, float delta, float radius);
	void SmoothArea(int2 p, float radius, float smoothness, int iterations);
	void FlattenArea(int2 p, float radius, float height, float tolerance, float steepness);
	void GetMinMaxRadius(int2 p, float radius, int2* px, int2* py, int div = 1);
	void LinkPoints(int2 p1, int2 p2, float radius);
	void PlaceMetalSpot(int2 p, float amount, float radius = 6.0f);
	void PrettyFlatten(int2 p, float radius);

	void PerformFlatten(const SFlattenSpot& SFlattenSpot);
	void PerformFlattens();

	// Symmetry-related functions
	void Mirror(bool vertical);
	void MirrorStartup(bool vertical);

	// Other
	float GetAverageHeight(int2 p, float range);

	// Vars
	std::vector<int2> metalSpots;
	std::vector<SFlattenSpot> flattens;
};

#endif // _SIMPLE_MAP_GENERATOR_H_
