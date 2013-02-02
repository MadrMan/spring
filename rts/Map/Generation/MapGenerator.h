/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_GENERATOR_H_
#define _MAP_GENERATOR_H_

#include "System/type2.h"
#include "Map/SMF/SMFReadMap.h"
#include "Game/GameSetup.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

class CVirtualFile;
class CVirtualArchive;
class CMapGenerator
{
public:
	virtual ~CMapGenerator();

	void Generate();

	const std::string& GetMapName() const
	{ return mapName; }

	unsigned int GetMapSeed() const
	{ return mapSeed; }

	const static unsigned int GRID_SQUARE_SIZE = 8;

protected:
	CMapGenerator(const CGameSetup* setup);

	const CGameSetup* setup;

	std::vector<float>& GetHeightMap()
	{ return heightMap; }

	std::vector<unsigned char>& GetMetalMap()
	{ return metalMap; }

	void SetMapName(const std::string& mapName)
	{ this->mapName = mapName; }

	void SetHeight(int x, int y, float v)
	{ heightMap[GetMapOffset(x, y)] = v; }

	float GetHeight(int x, int y) const
	{ return heightMap[GetMapOffset(x, y)]; }

	int GetMapOffset(int x, int y) const
	{ return y * gridSize.x + x; }

	int GetMetalOffset(int x, int y) const
	{ return y * metalSize.x + x; }

	void SetMapSize(const int2& mapSize);

	const int2& GetMapSize() const
	{ return mapSize; }

	const int2& GetGridSize() const
	{ return gridSize; }

	int GetRndInt(int min, int max)
	{ return (rng() % (max - min)) + min; }

	float GetRndFloat(float min, float max)
	{ return fdistribution(rng) * (max - min) + min; }

	float GetRndFloat()
	{ return fdistribution(rng); }

private:
	void GenerateSMF(CVirtualFile* fileSMF);
	void GenerateMapInfo(CVirtualFile* fileMapInfo);
	void GenerateSMT(CVirtualFile* fileSMT);

	bool MissingFileHandler(CVirtualFile* file);

	template<typename T>
	void AppendToBuffer(CVirtualFile* file, const T& data)
	{ AppendToBuffer(file, &data, sizeof(T)); }

	template<typename T>
	void SetToBuffer(CVirtualFile* file, const T& data, int position)
	{ SetToBuffer(file, &data, sizeof(T), position); }

	void AppendToBuffer(CVirtualFile* file, const void* data, int size);
	void SetToBuffer(CVirtualFile* file, const void* data, int size, int position);

	virtual void GenerateMap() = 0;
	virtual const std::vector<int2>& GetStartPositions() const = 0;
	virtual const std::string& GetMapDescription() const = 0;

	std::vector<float> heightMap;
	std::vector<unsigned char> metalMap;
	std::string mapName;
	const unsigned int mapSeed;

	boost::random::mt19937 rng;
	boost::random::uniform_real_distribution<float> fdistribution;

	int2 mapSize;
	int2 gridSize;
	int2 metalSize;

	CVirtualArchive* archive;
	CVirtualFile* fileSMF;
	CVirtualFile* fileMapInfo;
	CVirtualFile* fileSMT;
};

#endif // _MAP_GENERATOR_H_
