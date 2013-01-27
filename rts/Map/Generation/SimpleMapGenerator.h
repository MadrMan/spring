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

	virtual const std::vector<int2>& GetStartPositions() const
	{ return startPositions; }

	virtual const std::string& GetMapDescription() const
	{ return mapDescription; }

	std::vector<int2> startPositions;
	std::string mapDescription;

};

#endif // _SIMPLE_MAP_GENERATOR_H_
