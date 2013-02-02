/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MapGenerator.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/Archives/VirtualArchive.h"
#include "Map/SMF/SMFFormat.h"
#include "Rendering/GL/myGL.h"
#include "Game/LoadScreen.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

CMapGenerator::CMapGenerator(const CGameSetup* setup)
	: mapSeed(setup->mapSeed), setup(setup), archive(NULL), fileSMF(NULL),
	fileSMT(NULL), fileMapInfo(NULL), rng(setup->mapSeed)
{
}

CMapGenerator::~CMapGenerator()
{
	archive->missingFileHandler = 0;

	//At this point we should assume the map won't be touched again to save memory
	if(archive)
	{
		archive->clear();
	}
}

void CMapGenerator::SetMapSize(const int2& mapSize)
{
	this->mapSize = mapSize;
	gridSize.x = mapSize.x * CSMFReadMap::bigSquareSize + 1;
	gridSize.y = mapSize.y * CSMFReadMap::bigSquareSize + 1;
	metalSize.x = gridSize.x / 2;
	metalSize.y = gridSize.y / 2;
}

void CMapGenerator::Generate()
{
	//Create archive for map
	const std::string mapArchiveName = std::string("GenMap") + boost::lexical_cast<std::string>(mapSeed);
	const std::string mapArchivePath = mapArchiveName + "." + virtualArchiveFactory->GetDefaultExtension();
	archive = virtualArchiveFactory->AddArchive(mapArchiveName);

	//Set handler to be called when a non-loaded file is requested
	archive->missingFileHandler = std::bind1st(std::mem_fun(&CMapGenerator::MissingFileHandler), this);

	//Add initital files for scanning
	fileSMF = archive->AddFile("maps/generated.smf");
	fileMapInfo = archive->AddFile("mapinfo.lua");
	fileSMT = archive->AddFile("maps/generated.smt");

	//Create arrays that can be filled by top class
	const int dimensions = gridSize.x * gridSize.y;
	const int dimensionsMetal = metalSize.x * metalSize.y;
	heightMap.resize(dimensions);
	metalMap.resize(dimensionsMetal);

	//Add archive to vfs (will request mapinfo.lua)
	archiveScanner->ScanArchive(mapArchivePath);
}

bool CMapGenerator::MissingFileHandler(CVirtualFile* file)
{
	//This should usually be available
	if(!fileMapInfo->IsLoaded())
	{
		GenerateMapInfo(fileMapInfo);
	}

	//Only load these when actually requested, they can take a while
	if(file == fileSMF || file == fileSMT)
	{
		loadscreen->SetLoadMessage("Generating Map");

		GenerateMap();
		GenerateSMF(fileSMF);
		GenerateSMT(fileSMT);
	}

	return true;
}

void CMapGenerator::AppendToBuffer(CVirtualFile* file, const void* data, int size)
{
	file->buffer.insert(file->buffer.end(), (boost::uint8_t*)data, (boost::uint8_t*)data + size);
}

void CMapGenerator::SetToBuffer(CVirtualFile* file, const void* data, int size, int position)
{
	std::copy((boost::uint8_t*)data, (boost::uint8_t*)data + size, file->buffer.begin() + position);
}

void CMapGenerator::GenerateSMF(CVirtualFile* fileSMF)
{
	SMFHeader smfHeader;
	MapTileHeader smfTile;
	MapFeatureHeader smfFeature;

	//--- Make SMFHeader ---
	strcpy(smfHeader.magic, "spring map file");
	smfHeader.version = 1;
	smfHeader.mapid = 0x524d4746 ^ mapSeed;

	//Set settings
	smfHeader.mapx = GetGridSize().x - 1;
	smfHeader.mapy = GetGridSize().y - 1;
	smfHeader.squareSize = GRID_SQUARE_SIZE;
	smfHeader.texelPerSquare = 8;
	smfHeader.tilesize = 32;
	smfHeader.minHeight = -1000.0f;
	smfHeader.maxHeight = 5000.0f;

	const int numSmallTiles = 1; //2087; //32 * 32 * (mapSize.x  / 2) * (mapSize.y / 2);
	const char smtFileName[] = "generated.smt";

	//--- Extra headers ---
	ExtraHeader vegHeader;
	vegHeader.type = MEH_Vegetation;
	vegHeader.size = sizeof(int);

	smfHeader.numExtraHeaders =  1;

	//Make buffers for each map
	int heightmapDimensions = heightMap.size();
	int typemapDimensions = (smfHeader.mapx / 2) * (smfHeader.mapy / 2);
	int tilemapDimensions =  (smfHeader.mapx * smfHeader.mapy) / 16;
	int vegmapDimensions = (smfHeader.mapx / 4) * (smfHeader.mapy / 4);

	int heightmapSize = heightmapDimensions * sizeof(short);
	int typemapSize = typemapDimensions * sizeof(unsigned char);
	int tilemapSize = tilemapDimensions * sizeof(int);
	int tilemapTotalSize = sizeof(MapTileHeader) + sizeof(numSmallTiles) + sizeof(smtFileName) + tilemapSize;
	int vegmapSize = vegmapDimensions * sizeof(unsigned char);
	int minimapSize = MINIMAP_SIZE;

	short* heightmapPtr = new short[heightmapDimensions];
	unsigned char* typemapPtr = new unsigned char[typemapDimensions];
	int* tilemapPtr = new int[tilemapDimensions];
	unsigned char* vegmapPtr = new unsigned char[vegmapDimensions];
	unsigned char* minimapPtr = new unsigned char[minimapSize];

	//--- Set offsets, increment each member with the previous one ---
	int vegmapOffset = sizeof(smfHeader) + sizeof(vegHeader) + sizeof(int);
	smfHeader.heightmapPtr = vegmapOffset + vegmapSize;
	smfHeader.typeMapPtr = smfHeader.heightmapPtr + heightmapSize;
	smfHeader.tilesPtr = smfHeader.typeMapPtr + typemapSize;
	smfHeader.minimapPtr = smfHeader.tilesPtr + tilemapTotalSize;
	smfHeader.metalmapPtr = smfHeader.minimapPtr + minimapSize;
	smfHeader.featurePtr = smfHeader.metalmapPtr + metalMap.size();

	//--- Make MapTileHeader ---
	smfTile.numTileFiles = 1;
	smfTile.numTiles = numSmallTiles;

	//--- Make MapFeatureHeader ---
	smfFeature.numFeatures = 0;
	smfFeature.numFeatureType = 0;

	//--- Update Ptrs and write to buffer ---
	memset(vegmapPtr, 0, vegmapSize);

	float heightMin = smfHeader.minHeight;
	float heightMax = smfHeader.maxHeight;
	float heightMul = (float)0xFFFF / (smfHeader.maxHeight - smfHeader.minHeight);
	for(int x = 0; x < heightmapDimensions; x++)
	{
		float h = heightMap[x];
		if(h < heightMin) h = heightMin;
		if(h > heightMax) h = heightMax;
		h -= heightMin;
		h *= heightMul;
		heightmapPtr[x] = (short)h;
	}

	memset(typemapPtr, 0, typemapSize);

	/*for(u32 x = 0; x < smfHeader.mapx; x++)
	{
		for(u32 y = 0; y < smfHeader.mapy; y++)
		{
			u32 index =
			tilemapPtr[]
		}
	}*/

	memset(tilemapPtr, 0, tilemapSize);
	memset(minimapPtr, 0, minimapSize);

	//--- Write to final buffer ---
	AppendToBuffer(fileSMF, smfHeader);

	AppendToBuffer(fileSMF, vegHeader);
	AppendToBuffer(fileSMF, vegmapOffset);
	AppendToBuffer(fileSMF, vegmapPtr, vegmapSize);

	AppendToBuffer(fileSMF, heightmapPtr, heightmapSize);
	AppendToBuffer(fileSMF, typemapPtr, typemapSize);

	AppendToBuffer(fileSMF, smfTile);
	AppendToBuffer(fileSMF, numSmallTiles);
	AppendToBuffer(fileSMF, smtFileName, sizeof(smtFileName));
	AppendToBuffer(fileSMF, tilemapPtr, tilemapSize);

	AppendToBuffer(fileSMF, minimapPtr, minimapSize);

	AppendToBuffer(fileSMF, metalMap.data(), metalMap.size());
	AppendToBuffer(fileSMF, smfFeature);

	delete[] heightmapPtr;
	delete[] typemapPtr;
	delete[] tilemapPtr;
	delete[] vegmapPtr;
	delete[] minimapPtr;

	fileSMF->SetLoaded(true);
}

void CMapGenerator::GenerateMapInfo(CVirtualFile* fileMapInfo)
{
	//Open template mapinfo.lua
	const static std::string luaTemplate = "mapgenerator/mapinfo_template.lua";
	CFileHandler fh(luaTemplate, SPRING_VFS_PWD_ALL);
	if(!fh.FileExists())
	{
		throw content_error("Error generating map: " + luaTemplate + " not found");
	}

	std::string luaInfo;
	fh.LoadStringData(luaInfo);

	//Make info to put in mapinfo
	std::stringstream ss;
	std::string startPosString;
	const std::vector<int2>& startPositions = GetStartPositions();

	assert(!startPositions.empty());

	for(size_t x = 0; x < startPositions.size(); x++)
	{
		ss << "[" << x << "] = {startPos = {x = "
			<< startPositions[x].x * GRID_SQUARE_SIZE << ", z = "
			<< startPositions[x].y * GRID_SQUARE_SIZE << "}},";
	}
	startPosString = ss.str();

	//Replace tags in mapinfo.lua
	boost::replace_first(luaInfo, "${NAME}", mapName);
	boost::replace_first(luaInfo, "${DESCRIPTION}", GetMapDescription());
	boost::replace_first(luaInfo, "${START_POSITIONS}", startPosString);

	//Copy to filebuffer
	fileMapInfo->buffer.assign(luaInfo.begin(), luaInfo.end());
	fileMapInfo->SetLoaded(true);
}

void CMapGenerator::GenerateSMT(CVirtualFile* fileSMT)
{
	const int tileSize = 32;

	//--- Make TileFileHeader ---
	TileFileHeader smtHeader;
	strcpy(smtHeader.magic, "spring tilefile");
	smtHeader.version = 1;
	smtHeader.numTiles = 1; //32 * 32 * (generator->GetMapSize().x * 32) * (generator->GetMapSize().y * 32);
	smtHeader.tileSize = tileSize;
	smtHeader.compressionType = 1;

	const int bpp = 3;
	int tilePos = 0;
	unsigned char tileData[tileSize * tileSize * bpp];
	for(int x = 0; x < tileSize; x++)
	{
		for(int y = 0; y < tileSize; y++)
		{
			tileData[tilePos] = 0;
			tileData[tilePos + 1] = 0xFF;
			tileData[tilePos + 2] = 0;
			tilePos += bpp;
		}
	}
	glClearErrors();
	GLuint tileTex;
	glGenTextures(1, &tileTex);
	glBindTexture(GL_TEXTURE_2D, tileTex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, tileSize, tileSize, 0, GL_RGB, GL_UNSIGNED_BYTE, tileData);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	char tileDataDXT[SMALL_TILE_SIZE];

	int dxtImageOffset = 0;
	int dxtImageSize = 512;
	for(int x = 0; x < 4; x++)
	{
		glGetCompressedTexImage(GL_TEXTURE_2D, x, tileDataDXT + dxtImageOffset);
		dxtImageOffset += dxtImageSize;
		dxtImageSize /= 4;
	}

	glDeleteTextures(1, &tileTex);

	GLenum errorcode = glGetError();
	if(errorcode != GL_NO_ERROR)
	{
		throw content_error("Error generating map - texture generation not supported");
	}

	size_t totalSize = sizeof(TileFileHeader);
	fileSMT->buffer.resize(totalSize);

	int writePosition = 0;
	memcpy(&(fileSMT->buffer[writePosition]), &smtHeader, sizeof(smtHeader));
	writePosition += sizeof(smtHeader);

	fileSMT->buffer.resize(fileSMT->buffer.size() + smtHeader.numTiles * SMALL_TILE_SIZE);
	for(int x = 0; x < smtHeader.numTiles; x++)
	{
		memcpy(&(fileSMT->buffer[writePosition]), tileDataDXT, SMALL_TILE_SIZE);
		writePosition += SMALL_TILE_SIZE;
	}

	fileSMT->SetLoaded(true);
}
