/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Precompiled header file for Visual Studio, greatly speeds up rebuilds and compiles in general
// Note that the order here matters a lot due to macros in Windows.h

// STL
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <deque>
#include <ios>
#include <queue>
#include <deque>

#include <cstdio>
#include <cassert>
#include <cctype>
#include <cstring>
#include <ctime>

// Boost
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/system_error.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

//Assimp
#include "lib/assimp/include/assimp/Importer.hpp"

// OpenGL
#include "Rendering/GL/myGL.h"
#include <GL/wglew.h>

// Lua
//#include "lib/lua/include/LuaInclude.h"
//#undef IntPoint

// GML
#include "lib/gml/gml.h"

// SDL
#include <SDL.h>

// Other
#include <stddef.h>
#include <xmmintrin.h>

// Windows -- last so defines dont mess up stuff
#include "System/Platform/Win/win32.h"
#include <io.h>
#include <direct.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <process.h>
#include <imagehlp.h>
#include <signal.h>
#include <winsock.h>

// Undefine certain windows.h macros, included here instead of win32.h as they are
// only required when compiling with pch as normally windows.h wouldn't be included
#undef VOID
#undef INFINITE
#undef far
#undef near
#undef small
#undef FAR
#undef NEAR
#define FAR
#define NEAR

// Math (uses Windows headers for some reason)
#include "lib/streflop/streflop_cond.h"
#include "System/myMath.h"