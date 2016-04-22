/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleMapGenerator.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/FastMath.h"
#include "Map/SMF/SMFFormat.h"
#include "System/TimeProfiler.h"

#include <algorithm>

CSimpleMapGenerator::CSimpleMapGenerator(const CGameSetup* setup)
	: CMapGenerator(setup)
{
	GenerateInfo();
}

CSimpleMapGenerator::~CSimpleMapGenerator()
{
}

void CSimpleMapGenerator::GenerateInfo()
{
	SetMapSize(int2(10, 10));

	GenerateStartPositions2Teams();
	GenerateMapText();

	MirrorStartup(true);
}

void CSimpleMapGenerator::GenerateMapText()
{
	SetMapName("My Generated Map");
	SetMapDescription("My Map Description");
}

void CSimpleMapGenerator::GenerateMap()
{
	const int2& gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	const int gridCount = (gs.x * gs.y) / 2;
	int cells = GetMapSize().x + GetMapSize().y;

	// Metavars
	const float baseHeight = GetRndInt(0, 2) ? GetRndFloat(10.0f, 30.0f) : -60.0f;

	// Init to random height
	std::fill(map.begin(), map.end(), baseHeight);

	//Spawn general map area by just raising random hillchains
	{
		ScopedOnceTimer startserver("CSimpleMapGenerator::GenerateMap() Hillchain");
		int branches = GetRndInt(6, 25);
		float generalVariance = GetRndFloat(5.0f, 60.0f);
		float generalCurl = GetRndFloat(0.8f, 2.0f);
		for (int x = 0; x < branches; x++)
		{
			int2 s(GetRndInt(0, gs.x), GetRndInt(0, gs.y));
			float r = GetRndFloat(80.0f, 200.0f);

			//Allow for both land and sea, but dont make the seas too deep
			float h = GetRndFloat(-30.0f, 60.0f);
			if (abs(h) < 10.0f) continue;

			RaiseArea(s, h, r, 10.0f, 0.005f, GetRndInt(0, 2), GetRndInt(2, 4), 0.7f);
		}
	}

	//Spawn POI
	{
		ScopedOnceTimer startserver("CSimpleMapGenerator::GenerateMap() PlacePOI");
		PlacePOI();
	}

	// Spawn player areas
	{
		ScopedOnceTimer startserver("CSimpleMapGenerator::GenerateMap() Spawning player areas");
		const auto& startPositions = GetStartPositions();
		int sizePerPlayer = gridCount / startPositions.size();
		float spawnSize = GetRndFloat(sizePerPlayer / 3, sizePerPlayer / 2);
		float playerSpawnHeight = GetRndFloat(40, 80.0f);
		for (auto sp : startPositions)
		{
			float h = playerSpawnHeight;
			RaiseArea(sp, h, spawnSize / 2.0f, 30.0f, 0.005f, 1, 0, 0.7f);

			// Place 3 mspots for the player spawn
			const int STARTING_MSPOTS = 3;
			float angle = GetRndFloat(0.0f, fastmath::PI2);
			for (int x = 0; x < STARTING_MSPOTS; x++)
			{
				int2 moffset(cos(angle) * 30.0f, sin(angle) * 30.0f);

				PlaceMetalSpot(sp + moffset, 40.0f);
				angle += fastmath::PI2 / STARTING_MSPOTS;
			}
		}
	}

	// Perform some delayed actions
	{
		ScopedOnceTimer startserver("CSimpleMapGenerator::GenerateMap() Performing delayed actions");
		PerformFlattens();
	}

	//Do several post-fixes
	{
		ScopedOnceTimer startserver("CSimpleMapGenerator::GenerateMap() Post-fixing");
		Mirror(true);
		PostProcess();
	}
}

void CSimpleMapGenerator::PlaceMetalSpot(int2 p, float amount, float radius)
{
	std::vector<unsigned char>& mm = GetMetalMap();

	metalSpots.push_back(p);

	const int metalDiv = 2;
	radius /= metalDiv;
	p.x /= metalDiv;
	p.y /= metalDiv;

	int2 px, py;
	GetMinMaxRadius(p, radius, &px, &py, metalDiv);

	for(int x = px.x; x < px.y; x++)
	{
		for(int y = py.x; y < py.y; y++)
		{
			
			float dx = (p.x - x) / radius;
			float dy = (p.y - y) / radius;
			float d1 = 1.0f - math::sqrt(dx * dx + dy * dy);
			if(d1 > 0.0f)
			{
				int i = GetMetalOffset(x, y);
				mm[i] = static_cast<unsigned int>(amount);
			}
		}
	}
}

void CSimpleMapGenerator::PlacePOIHill(int2 p, float height, float radius)
{
	RaiseArea(p, height, radius, 60.0f, 0.8f, 1, 0, 2.0f);
	PrettyFlatten(p, radius * 1.5f);
	SmoothArea(p, radius * 2.5f, 0.8f, 30);
}

void CSimpleMapGenerator::PlacePOI()
{
	const int2& gs = GetGridSize();
	int cells = GetMapSize().x * GetMapSize().y;

	// Make several 'Point Of Interest'
	// Points placed somewhere vaguely near the center of the map
	// Each point is mirrored as this is fair in most cases (regardless of general map mirror)
	std::vector<std::pair<int2, float>> poi;

	const int POI_BORDER = gs.x / 4;
	const int pointCount = GetRndInt(0, cells / 8 + 1);
	const int2 centerPoint(gs.x / 2, gs.y / 2);
	for(int x = 0; x < pointCount; x++)
	{
		float width = GetRndFloat(20.0f, 40.0f);

		int2 p;
		p.x = GetRndInt(POI_BORDER, gs.x - POI_BORDER);
		p.y = GetRndInt(0, gs.y);
		poi.push_back(std::make_pair(p, width));

		//mirror point
		int2 pm;
		pm.x = centerPoint.x + (centerPoint.x - p.x);
		pm.y = centerPoint.y + (centerPoint.x - p.y);
		poi.push_back(std::make_pair(pm, width));

		//raise land for POI
		float d = GetRndFloat(40.0f, 90.0f);
		PlacePOIHill(p, d, width);
		PlacePOIHill(pm, d, width);
	}

	// Lets make all player spawn positions points of interest as well
	for (const auto& sp : GetStartPositions())
	{
		poi.push_back(std::make_pair(sp, 40.0f));
	}
}

void CSimpleMapGenerator::LinkPoints(int2 p1, int2 p2, float radius)
{
	const int2 pd(p2.x - p1.x, p2.y - p1.y);
	const float dist = math::sqrt(pd.x * pd.x + pd.y * pd.y);
	const float2 pdd(pd.x / dist, pd.y / dist);

	float h1 = GetAverageHeight(p1, 10.0f);
	float h2 = GetAverageHeight(p2, 10.0f);
	float hd = (h2 - h1) / dist;

	const float density = 1.0f;
	for (float d = 0.0f; d < dist; d += density)
	{
		int2 pcur(p1.x + pdd.x * d, p1.y + pdd.y * d);
		float hcur = h1 + hd * d;

		SFlattenSpot spot;
		spot.height = hcur;
		spot.p = pcur;
		spot.radius = radius;
		spot.steepness = 1.0f;
		spot.tolerance = 0;
		PerformFlatten(spot);
	}

	for (float d = 0.0f; d < dist; d += density)
	{
		int2 pcur(p1.x + pdd.x * d, p1.y + pdd.y * d);

		SmoothArea(pcur, radius * 1.4f, 0.9f, 5);
	}
}

void CSimpleMapGenerator::PostProcess()
{
	std::vector<float>& map = GetHeightMap();
	const int2 gs = GetGridSize();

	// Make absolutely sure all metal spots are flattened
	for (auto p : metalSpots)
	{
		PrettyFlatten(p, 14.0f);
	}

	// Flatten out the oceans and such a bit, prevents buggy sea
	const float SEA_BORDER = -3.0f;
	const float SEA_UPPER_BORDER = 6.0f;
	for (int x = 0; x < gs.x; x++)
	{
		for (int y = 0; y < gs.y; y++)
		{
			float h = GetHeight(x, y);
			if (h < SEA_UPPER_BORDER && h > SEA_BORDER)
			{
				SetHeight(x, y, SEA_UPPER_BORDER);
				SmoothArea(int2(x, y), 10.0f, 0.2f, 10);
			}
		}
	}

	for (int x = 0; x < gs.x; x++)
	{
		for (int y = 0; y < gs.y; y++)
		{
			float h = GetHeight(x, y);
			if (h <= SEA_BORDER)
			{
				SmoothArea(int2(x, y), 10.0f, 0.8f, 20);
			}
		}
	}
}

void  CSimpleMapGenerator::GetMinMaxRadius(int2 p, float radius, int2* px, int2* py, int div)
{
	int2 gs = GetGridSize();

	gs.x /= div;
	gs.y /= div;

	int radi = ((int)radius);
	px->x = std::max(p.x - radi, 0);
	px->y = std::min(p.x + radi, gs.x);
	py->x = std::max(p.y - radi, 0);
	py->y = std::min(p.y + radi, gs.y);
}

void CSimpleMapGenerator::GenerateStartPositions2Teams()
{
	// Simply place an equal amount of spawns on the left and right side of the map
	const int2& gs = GetGridSize();

	const auto& teamData = setup->GetTeamStartingDataCont();
	const auto& allyData = setup->GetAllyStartingDataCont();
	const int teams = teamData.size();
	int minPlacementDistance = gs.y / teams;
	const int border = gs.x / 10;

	// Technically, we can even _set_ the ally starting squares here?
	for (int x = 0; x < teams; x++)
	{
		const int allyteam = teamData[x].teamAllyteam;
		
		int top, left, right, bottom;
		if (allyteam == 0)
		{
			top = 0;
			left = 0;
			right = gs.x / 4;
			bottom = gs.y;
		}
		else
		{
			top = 0;
			left = gs.x - gs.x / 4;
			right = gs.x;
			bottom = gs.y;
		}

		bool placed = false;
		while (!placed)
		{
			int spawnx = GetRndInt(left + border, right - border);
			int spawny = GetRndInt(top + border, bottom - border);
			int2 pos(spawnx, spawny);

			for (auto sp : GetStartPositions())
			{
				if (sp.distance(pos) < minPlacementDistance)
				{
					continue; // Too close to ally
				}
			}

			GetStartPositions().push_back(pos);
			placed = true;
		}
	}
}

void CSimpleMapGenerator::GenerateStartPositionsFFA()
{
	//Algorithm here is fairly simple:
	// 1. Start at random point in map
	// 2. Place all team members of team x in that area
	// 3. Rotate trough map 360/teams degrees
	// 4. Back to step 2
	const int2& gs = GetGridSize();

	const auto& teamData = setup->GetTeamStartingDataCont();
	const size_t allies = setup->GetAllyStartingDataCont().size();
	const size_t teams = teamData.size();
	const int moveLimit = (gs.x + gs.y) / (6 + allies * 2);
	const int borderDistance = 16;

	const float startAngle = GetRndFloat() * fastmath::PI2;
	const float angleIncrement = fastmath::PI2 / allies;

	for(size_t x = 0; x < teams; x++)
	{
		const auto& team = teamData[x];
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
		const int moveMinX = std::min(ipx - borderDistance, moveLimit);
		const int moveMinY = std::min(ipy - borderDistance, moveLimit);
		const int moveMaxX = std::min(gs.x - ipx - borderDistance, moveLimit);
		const int moveMaxY = std::min(gs.y - ipy - borderDistance, moveLimit);
		ipx += GetRndInt(-moveMinX, moveMaxX);
		ipy += GetRndInt(-moveMinY, moveMaxY);

		//Add position for player
		GetStartPositions().push_back(int2(ipx, ipy));
	}
}

void CSimpleMapGenerator::RaiseArea(int2 p, float delta, float radius, float variance, float smoothness, int flattens, int mspots, float curl)
{
	assert(variance >= 1.0f); // dont

	curl = 0.05f;

	// Fixed variance
	float rv = 1.0f - 1.0f / variance;
	const float baseHillRadius = 20.0f;

	// Amount of hills to put in area
	const float hillCount = 5000.0f;
	const int hillsMax = rv * hillCount + 6;
	const int hills = GetRndInt(hillsMax / 6, hillsMax / 4);
	const int hillRefocus = hills / 12;
	const float hillMultiplier = hills / (hillCount /  10.0f);

	// Figure out what we want the hills to look like
	std::vector<int2> chain;
	float dradius = (radius / hills);
	float jradius = dradius * 20.0f;
	float2 hillPoint = p;
	float angle = GetRndFloat() * fastmath::PI2;

	// Create a spiralling shape of closely stacked hills
	float currentCurl = curl;
	float currentHeightDir = 0.0f;
	float currentHeight = hillMultiplier;
	for(int h = 0; h < hills; h++)
	{
		angle += GetRndFloat(0.2f, 0.4f) * currentCurl;
		hillPoint.x += cos(angle) * jradius;
		hillPoint.y += sin(angle) * jradius;
		int2 hp(hillPoint.x, hillPoint.y);

		currentHeight += currentHeightDir;

		if (!IsInGrid(hp) || // Reset to start if out of grid
			(hillRefocus && (h % hillRefocus == 0))) // Or if we spawned plenty of hills
		{
			//angle += fastmath::HALFPI; // Turn around if we're about to go out of the map. might help. might not.
			angle = GetRndFloat() * fastmath::PI2; // Random angle
			currentCurl = curl * (GetRndInt(0, 2) * 2 - 1);
			currentCurl *= GetRndFloat(0.2f, 2.0f);

			hillPoint = p;
			hillPoint.x += cos(angle) * baseHillRadius * GetRndFloat(1.0f, 4.0f);
			hillPoint.y += sin(angle) * baseHillRadius * GetRndFloat(1.0f, 4.0f);

			currentHeight = GetRndFloat(0.5f, 1.5f) * hillMultiplier;
			currentHeightDir = GetRndFloat(-0.01f, 0.03f) * hillMultiplier;

			continue;
		}

		chain.push_back(hp);

		float hradius = baseHillRadius * GetRndFloat(0.6f, 1.6f) * dradius * abs(delta);
		RaiseHill(hp, delta * currentHeight * 0.03f, hradius * 0.8f);
	}

	if (chain.empty())
	{
		// Somehow we managed to place absolutely nothing inside the map
		return;
	}

	// Add metal spots
	for (int h = 0; h < mspots; h++)
	{
		int2 p = { chain[GetRndInt(0, chain.size())] };

		PlaceMetalSpot(p, 40.0f);
	}

	// Post-smooth a bit
	for (int h = 0; h < chain.size(); h++)
	{
		const auto& c = chain[h];

		SmoothArea(c, baseHillRadius * 1.5f, smoothness, 2);
	}

	// Flatten the hill chain
	for (int h = 0; h < flattens; h++)
	{
		int2 c = { chain[GetRndInt(0, chain.size())] };

		float hillCenterHeight = GetHeight(c.x, c.y);
		FlattenArea(c, baseHillRadius * 20.0f, hillCenterHeight, 350.0f, 0.1f);
	}
}

void CSimpleMapGenerator::RaiseHill(int2 p, float height, float radius)
{
	std::vector<float>& map = GetHeightMap();
	const int2& gs = GetGridSize();

	int2 px, py;
	GetMinMaxRadius(p, radius, &px, &py);

	for(int x = px.x; x < px.y; x++)
	{
		for(int y = py.x; y < py.y; y++)
		{
			int i = GetMapOffset(x, y);
			float dx = (p.x - x) / radius;
			float dy = (p.y - y) / radius;
			float ddist = sqrt(dx * dx + dy * dy);
			float s = std::max(1.0f - ddist * ddist * ddist, 0.0f);

			map[i] += (height * s);
		}
	}
}

void CSimpleMapGenerator::SmoothArea(int2 p, float radius, float smoothness, int iterations)
{
	// Smooth an area
	// Check if it will make a visible difference
	if(smoothness < 0.0001f) return;

	std::vector<float>& map = GetHeightMap();

	int2 px, py;
	GetMinMaxRadius(p, radius, &px, &py);

	// If you want to do this proper, use a secondary buffer for storing results and copy it later
	// That way you don't get a warped effect where you smooth based on previous results
	// Not that it looks any worse that way.
	for (int i = 0; i < iterations; i++)
	{
		for (int x = px.x; x < px.y; x++)
		{
			for (int y = py.x; y < py.y; y++)
			{
				// Instead of a simple dx/dy we can use a proper gaussian filter here - in theory
				float c = GetHeight(x, y);
				float x1 = c, x2 = c, y1 = c, y2 = c;

				if (IsInGrid(int2(x - 1, y))) x1 = GetHeight(x - 1, y);
				if (IsInGrid(int2(x + 1, y))) x2 = GetHeight(x + 1, y);
				if (IsInGrid(int2(x, y - 1))) y1 = GetHeight(x, y - 1);
				if (IsInGrid(int2(x, y + 1))) y2 = GetHeight(x, y + 1);

				float dx = (p.x - x) / radius;
				float dy = (p.y - y) / radius;
				float ddist = math::sqrt(dx * dx + dy * dy);
				float s = std::max(1.0f - ddist * ddist, 0.0f);

				c = c * (1.0f - smoothness) + ((x1 + x2 + y1 + y2) * 0.25f * smoothness);

				SetHeight(x, y, c);
			}
		}
	}
}

void CSimpleMapGenerator::FlattenArea(int2 p, float radius, float height, float tolerance, float steepness)
{
	SFlattenSpot s;
	s.p = p;
	s.radius = radius;
	s.height = height;
	s.tolerance = tolerance;
	s.steepness = steepness;

	flattens.push_back(s);
}

void CSimpleMapGenerator::PrettyFlatten(int2 p, float radius)
{
	// Just iterate a dozen times for both flattens and smoothing
	for (int x = 0; x < 20; x++)
	{
		SFlattenSpot s;
		s.p = p;
		s.radius = radius;
		s.height = GetAverageHeight(p, radius);
		s.steepness = 0.6f;
		s.tolerance = 0;

		SmoothArea(p, radius * 2.0f, 0.6f, 4);
		PerformFlatten(s);
	}

	SmoothArea(p, radius * 3.0f, 0.4f, 8);
}

void CSimpleMapGenerator::PerformFlatten(const SFlattenSpot& s)
{
	// Actually flatten an area, pushing heights towards the desired height
	int2 px, py;
	GetMinMaxRadius(s.p, s.radius, &px, &py);

	bool useTolerance = s.tolerance > 0.001f;

	std::vector<float>& map = GetHeightMap();

	for (int x = px.x; x < px.y; x++)
	{
		for (int y = py.x; y < py.y; y++)
		{
			int i = GetMapOffset(x, y);
			float h = map[i];

			float dx = (s.p.x - x) / s.radius;
			float dy = (s.p.y - y) / s.radius;

			// Get (amplified) distance to center
			const float coreStrength = s.steepness;
			float distToCenter = (1.0f + coreStrength) - math::sqrt(dx * dx + dy * dy);
			distToCenter = std::max(std::min((distToCenter * distToCenter) - coreStrength, 1.0f), 0.0f);

			// Push heights to the desired height, assuming they're within tolerance range
			float d = s.height - h;
			float dtt = useTolerance ? ((s.tolerance - abs(d)) / s.tolerance) : 1.0f; // (Distance to tolerance in 0-1)
			dtt = std::min(dtt * 3.0f, 1.0f); // Smooth points close to tolerance limit
			if (useTolerance && abs(d) > s.tolerance) dtt = 0.0f; // Ignore everything beyond tolerance range

			h += d * dtt * distToCenter;

			map[i] = h;
		}
	}
}

void CSimpleMapGenerator::PerformFlattens()
{
	for (const auto& s : flattens)
	{
		PerformFlatten(s);
	}

	flattens.clear();
}

float CSimpleMapGenerator::GetAverageHeight(int2 p, float range)
{
	// If outside of map, clamp to map border so we always get some valid height
	p.x = std::max(std::min(p.x, GetGridSize().x), 0);
	p.y = std::max(std::min(p.y, GetGridSize().y), 0);

	int2 px, py;
	GetMinMaxRadius(p, range, &px, &py);

	float totalWeight = 0.0f;
	float totalHeight = 0.0f;

	assert(px.y - px.x > 0 && py.y - py.x > 0); // entire range outside of map

	for (int x = px.x; x < px.y; x++)
	{
		for (int y = py.x; y < py.y; y++)
		{
			float dx = (p.x - x) / range;
			float dy = (p.y - y) / range;

			float distToCenter = std::max(1.0f - math::sqrt(dx * dx + dy * dy), 0.0f);
			totalHeight += GetHeight(x, y) * distToCenter;
			totalWeight += distToCenter;
			
		}
	}

	return totalHeight / totalWeight;
}

void CSimpleMapGenerator::Mirror(bool vertical)
{
	// Copy all maps:
	// Heightmap
	int2 mapgs = GetGridSize();
	
	for (int x = 0; x < mapgs.x / 2; x++)
	{
		for (int y = 0; y < mapgs.y; y++)
		{
			SetHeight(x, y, GetHeight(mapgs.x - x - 1, y));
		}
	}

	for (int y = 0; y < mapgs.y; y++)
	{
		int2 p = { mapgs.x / 2, y };
		SmoothArea(p, 6, 0.3f, 20);
		SmoothArea(p, 18, 0.2f, 8);
	}

	// Metalmap
	int2 metalgs = GetMetalGridSize();

	for (int x = 0; x < metalgs.x / 2; x++)
	{
		for (int y = 0; y < metalgs.y; y++)
		{
			SetMetal(x, y, GetMetal(metalgs.x - x - 1, y));
		}
	}

	// Remove all metal spots from one side
	metalSpots.erase(std::remove_if(metalSpots.begin(), metalSpots.end(), [mapgs](int2 p) {
		return (p.x < mapgs.x / 2);
	}), metalSpots.end());

	// Copy remaining spots to other side
	size_t spots = metalSpots.size();
	for (size_t x = 0; x < spots; x++)
	{
		// Flip over center
		const int center = mapgs.x / 2;

		int2 p = metalSpots[x];
		p.x = (center - p.x) + center;
		metalSpots.push_back(p);
	}

	// Copy features:
	// (todo)
}

void CSimpleMapGenerator::MirrorStartup(bool vertical)
{
	// Copy start positions:
	auto& sp = GetStartPositions();
	const int teamSize = sp.size() / 2;
	const int2 gs = GetGridSize();

	for (size_t x = 0; x < teamSize; x++)
	{
		// Flip over center
		const int center = gs.x / 2;

		int2 p = sp[x];
		p.x = (center - p.x) + center;
		sp[x + teamSize] = p;
	}
}