#pragma once

//Define only if the map should be isometric
//#define MAP_IS_ISOMETRIC

#ifdef MAP_IS_ISOMETRIC
//Tiles pixel size
# define TILESIZE_X 64
# define TILESIZE_Y 32
#else
//Tiles pixel size
# define TILESIZE_X 64
# define TILESIZE_Y 64
#endif

#include <geModule.h>
#include <geVector2I.h>
#include <geVector2.h>
#include <geColor.h>

using namespace geEngineSDK;

class GameOptions : public Module<GameOptions>
{
 public:
  //Application values
  static Vector2I s_Resolution;
  static Vector2 s_MapMovementSpeed;

  //World Values

  //Map Values
  static bool s_MapShowGrid;
  static Color s_MapGridColor;

  //Editor
  static int s_Terrain;

  //PathFinders
  static int s_PfPositions;
  static int s_PathFindingTypes;

  //Global Constants
  static const Vector2I TILEHALFSIZE;
  static const Vector2I BITSFHT_TILEHALFSIZE;
  static const Vector2I BITSHFT_TILESIZE;

  //Windows 
  static bool s_Editor;
  static bool s_GameOptions;
  static bool s_PathFinders;

};

GameOptions&
g_gameOptions();
