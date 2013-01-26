/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_GENERATOR_H_
#define _MAP_GENERATOR_H_

#include "System/type2.h"
#include "Map/SMF/SMFReadMap.h"

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

protected:
	CMapGenerator(unsigned int mapSeed);

	const unsigned int mapSeed;

	std::vector<float>& GetHeightMap()
	{ return heightMap; }

	void SetMapName(const std::string& mapName)
	{ this->mapName = mapName; }

	virtual int2 GetGridSize() const
	{ return int2(GetMapSize().x * CSMFReadMap::bigSquareSize, GetMapSize().y * CSMFReadMap::bigSquareSize); }

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
	virtual const int2& GetMapSize() const = 0;
	virtual const std::vector<int2>& GetStartPositions() const = 0;
	virtual const std::string& GetMapDescription() const = 0;

	std::vector<float> heightMap;
	std::vector<float> metalMap;
	std::string mapName;
	bool isGenerated;

	CVirtualArchive* archive;
	CVirtualFile* fileSMF;
	CVirtualFile* fileMapInfo;
	CVirtualFile* fileSMT;
};

#endif // _MAP_GENERATOR_H_
