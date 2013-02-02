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

	const int2& gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	const int gridCount = (gs.x + gs.y) / 2;
	int cells = GetMapSize().x + GetMapSize().y;

	//Init to random height
	std::fill(map.begin(), map.end(), GetRndFloat(-40.0f, 40.0f));

	//Spawn general map area by just raising random hillchains
	int branches = GetRndInt(2, 25);
	for(int x = 0; x < branches; x++)
	{
		int2 s(GetRndInt(0, gs.x), GetRndInt(0, gs.y));
		float r  = GetRndFloat(gridCount / 32, gridCount / 4);

		//Allow for both land and sea, but dont make the seas too deep
		float h = GetRndFloat(-30.0f, 10.0f);
		h /= r / gridCount;
		h = std::max(-60.0f, h);

		RaiseArea(s, h, r, 2.0f, 0.02f);
	}

	//Spawn POI
	PlacePOI();

	//Spawn player areas
	int sizePerPlayer = gridCount / startPositions.size();
	float spawnSize = GetRndFloat(sizePerPlayer / 2, sizePerPlayer);
	for(std::vector<int2>::const_iterator it = startPositions.begin(); it != startPositions.end(); ++it)
	{
		int2 p = *it;

		float h = GetRndFloat(10.0f, 30.0f);
		RaiseArea(p, h, spawnSize / 5, 20.0f, 0.15f);

		PlaceMetalSpot(p, 20.0f);
	}

	//Do several post-fixes
	//PostProcess();
}

void CSimpleMapGenerator::PlaceMetalSpot(int2 p, float amount, float radius)
{
	std::vector<unsigned char>& mm = GetMetalMap();

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
			int i = GetMetalOffset(x, y);
			float dx = (p.x - x) / radius;
			float dy = (p.y - y) / radius;
			float d1 = 1.0f - math::sqrt(dx * dx + dy * dy);
			if(d1 > 0.0f)
			{
				mm[i] = (unsigned int)amount;
			}
		}
	}
}

void CSimpleMapGenerator::PlacePOI()
{
	const int2& gs = GetGridSize();
	int cells = GetMapSize().x + GetMapSize().y;

	//Make several 'Point Of Interest'
	//Points placed somewhere vaguely near the center of the map
	//Each point is mirrored as this is fair in most cases
	std::vector<int2> poi;
	int pointCount = GetRndInt(0, cells / 2) + 2;
	const int2 centerPoint(gs.x / 2, gs.y / 2);
	for(int x = 0; x < pointCount; x++)
	{
		int2 p;
		//p.x = centerPoint.x + GetRndInt(-gs.x / 3, gs.x / 3);
		//p.y = centerPoint.y + GetRndInt(-gs.y / 3, gs.y / 3);
		p.x = GetRndInt(0, gs.x);
		p.y = GetRndInt(0, gs.y);
		poi.push_back(p);

		//mirror point
		int2 pm;
		pm.x = centerPoint.x + (centerPoint.x - p.x);
		pm.y = centerPoint.y + (centerPoint.x - p.y);
		poi.push_back(pm);

		//add metal spots
		PlaceMetalSpot(p, 40.0f);
		PlaceMetalSpot(pm, 40.0f);

		//raise land for POI
		float d = GetRndFloat(10.0f, 40.0f);
		RaiseArea(p, d, 8.0f, 2.5f, 0.12f);
		RaiseArea(pm, d, 8.0f, 2.5f, 0.12f);
	}

	//Link POI
	for(int x = 0; x < pointCount; x++)
	{
		int i = x * 2;
		int im = i + 1;
		int linkto = GetRndInt(x + 1, poi.size());
		int linktom = (linkto % 2 == 0) ? linkto + 1 : linkto - 1;

		float radius = GetRndFloat(5.0f, 8.0f);
		float variance = GetRndFloat(1.0f, 1.2f);
		float smoothness = 0.3f;

		int2 p1 = poi[i];
		int2 p2 = poi[linkto];
		LinkPoints(p1, p2, radius, variance, smoothness);

		//check if we're not linking the same points again
		if(i != linktom)
		{
			int2 pm1 = poi[im];
			int2 pm2 = poi[linktom];
			LinkPoints(pm1, pm2, radius, variance, smoothness);
		}
	}
}

void CSimpleMapGenerator::LinkPoints(int2 p1, int2 p2, float radius, float variance, float smoothness)
{
	float h1 = GetHeight(p1.x, p1.y);
	float h2 = GetHeight(p2.x, p2.y);
	int2 pd(p2.x - p1.x, p2.y - p1.y);
	float dist = math::sqrt(pd.x * pd.x + pd.y * pd.y);
	float2 pdd(pd.x / dist, pd.y / dist);
	float pdh = (h2 - h1) / dist;

	float density = 5.0f;
	for(float d = 0.0f; d < dist; d += density)
	{
		int2 pcur(p1.x + pdd.x * d, p1.y + pdd.y * d);
		float hcur = h1 + pdh * d;
		float hdelta = hcur - GetHeight(pcur.x, pcur.y);

		RaiseArea(pcur, hdelta * 0.2f, radius, variance, 0.0f, 0.3f);
	}

	for(float d = 0.0f; d < dist; d += density)
	{
		int2 pcur(p1.x + pdd.x * d, p1.y + pdd.y * d);
		FlattenArea(pcur, radius * 3.0f, smoothness);
	}
}

void CSimpleMapGenerator::PostProcess()
{
	std::vector<float>& map = GetHeightMap();
	const int2& gs = GetGridSize();

	int i = 0;
	const float ztolerance = 1.5f;
	for(int y = 0; y < gs.y; y++)
	{
		for(int x = 0; x < gs.x; x++)
		{
			//Large patches of land near 0 cause z-fighting
			float h = map[i];
			if(h < ztolerance && h > -ztolerance)
			{
				//Push land underwater instead (less noticable)
				h = -ztolerance;
			}
			map[i] = h;

			i++;
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

void CSimpleMapGenerator::GenerateStartPositions()
{
	//Algorithm here is fairly simple:
	// 1. Start at random point in map
	// 2. Place all team members of team x in that area
	// 3. Rotate trough map 360/teams degrees
	// 4. Back to step 2
	const int2& gs = GetGridSize();

	const int allies = setup->allyStartingData.size();
	const int teams = setup->teamStartingData.size();
	const int moveLimit = (gs.x + gs.y) / (6 + allies * 2);
	const int borderDistance = 16;

	const float startAngle = GetRndFloat() * fastmath::PI2;
	const float angleIncrement = fastmath::PI2 / allies;

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
		const int moveMinX = std::min(ipx - borderDistance, moveLimit);
		const int moveMinY = std::min(ipy - borderDistance, moveLimit);
		const int moveMaxX = std::min(gs.x - ipx - borderDistance, moveLimit);
		const int moveMaxY = std::min(gs.y - ipy - borderDistance, moveLimit);
		ipx += GetRndInt(-moveMinX, moveMaxX);
		ipy += GetRndInt(-moveMinY, moveMaxY);

		//Add position for player
		startPositions.push_back(int2(ipx, ipy));
	}
}

void CSimpleMapGenerator::RaiseArea(int2 p, float delta, float radius, float variance, float smoothness, float curl)
{
	//fixed variance
	float rv = 1.0f - 1.0f / std::max(variance, 1.0f);

	//amount of hills to put in area
	int hillsMax = rv * 420.0f + 5;
	int hills = GetRndInt(hillsMax / 2, hillsMax);

	float hdelta = GetRndFloat(0.9f, 1.3f) * delta;

	//raise hills
	std::vector<int2> chain(hills);
	float dradius = (radius / hills);
	float jradius = dradius * 4.0f;
	float2 hillPoint(p.x, p.y);
	float angle = GetRndFloat() * fastmath::PI2;

	for(int h = 0; h < hills; h++)
	{
		angle += GetRndFloat(-fastmath::PI, fastmath::PI) * curl;
		hillPoint.x += cos(angle) * jradius;
		hillPoint.y += sin(angle) * jradius;
		int2 hp(hillPoint.x, hillPoint.y);

		chain[h] = hp;

		float hradius = GetRndFloat(20.0f, 60.0f) * dradius;
		RaiseHill(hp, hdelta, hradius + abs(hdelta));
	}

	//flatten the hill chain
	for(int h = 0; h < hills; h++)
	{
		FlattenArea(chain[h], dradius * 150.0f, smoothness);
	}
}

void CSimpleMapGenerator::RaiseHill(int2 p, float height, float radius)
{
	std::vector<float>& map = GetHeightMap();
	const int2& gs = GetGridSize();

	int2 px, py;
	GetMinMaxRadius(p, radius, &px, &py);
	float tx = GetRndFloat(-1.0f, 1.0f);
	float ty = GetRndFloat(-1.0f, 1.0f);

	for(int x = px.x; x < px.y; x++)
	{
		for(int y = py.x; y < py.y; y++)
		{
			int i = GetMapOffset(x, y);
			float dx = (p.x - x) / radius;
			float dy = (p.y - y) / radius;
			float d1 = std::max(1.0f - math::sqrt(dx * dx + dy * dy), 0.0f);
			float d2 = std::max(0.5f - sqrt(Square(dx - tx) + Square(dy - ty)), 0.0f) * 2.0f;
			float s = d1 * d2;

			map[i] += (height * s);
		}
	}
}

void CSimpleMapGenerator::FlattenArea(int2 p, float radius, float smoothness)
{
	//Check if it will make a visible difference
	if(smoothness < 0.0001f) return;

	std::vector<float>& map = GetHeightMap();

	int2 px, py;
	GetMinMaxRadius(p, radius, &px, &py);

	float avg = 0.0f;
	for(int x = px.x; x < px.y; x++)
	{
		for(int y = py.x; y < py.y; y++)
		{
			avg += GetHeight(x, y);
		}
	}
	avg /= (px.y - px.x) * (py.y - py.x);

	for(int x = px.x; x < px.y; x++)
	{
		for(int y = py.x; y < py.y; y++)
		{
			int i = GetMapOffset(x, y);

			float dx = (p.x - x) / radius;
			float dy = (p.y - y) / radius;
			float d1 = std::max(0.9f - math::sqrt(dx * dx + dy * dy), 0.0f);
			d1 = std::min(d1 * smoothness, 1.0f);

			/*int2 ipx, ipy;
			GetMinMaxRadius(int2(x, y), innerRadius, &ipx, &ipy);
			float avg = 0.0f;

			for(int ix = ipx.x; ix < ipx.y; ix++)
			{
				for(int iy = ipy.x; iy < ipy.y; iy++)
				{
					avg += GetHeight(ix, iy);
				}
			}*/

			//avg /= (ipx.y - ipx.x) * (ipy.y - ipy.x);
			float dh = avg - map[i];
			map[i] += d1 * dh;
		}
	}
}