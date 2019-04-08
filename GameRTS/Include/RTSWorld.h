#pragma once

#include <gePrerequisitesUtil.h>
#include <geVector2I.h>

#include <SFML/Graphics.hpp>

#include "RTSUnitType.h"

using namespace geEngineSDK;

namespace RTSGame {

  class RTSMapGridWalker;
  class RTSUnitType;
  class RTSUnit;
  class RTSTiledMap;


  class RTSWorld
  {
  public:
    RTSWorld();
    ~RTSWorld();

  public:
    bool
      init( sf::RenderTarget* pTarget );

    void
      destroy();

    void
      update( float deltaTime );

    void
      render();

    RTSTiledMap*
      getTiledMap()
    {
      return m_pTiledMap;
    }

    void
      resetPath();

    void
      setStartPos( int32 x, int32 y );

    void
      setEndPos( int32 x, int32 y );

    void
      updateResolutionData();

    void
      setCurrentWalker( const int8 index );

    void
      createUnit( UNIT_TYPE::E type, uint32 posX, uint32 posY );

    void
      selectUnits(Vector2 a, Vector2 b);

  private:
    RTSTiledMap* m_pTiledMap;
    Vector<RTSGame::RTSUnitType*> m_lstUnitTypes;
    Vector<RTSUnit*> m_lstUnits;

    Vector<RTSMapGridWalker*> m_walkersList;
    Vector<RTSUnit*> m_selectedUnits;
   // Vector<void*> m_walkersList;
    RTSMapGridWalker* m_activeWalker;
   //void* m_activeWalker;
    int8 m_activeWalkerIndex;

    sf::RenderTarget* m_pTarget;
  };
}