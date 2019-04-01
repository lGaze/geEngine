#pragma once

#include "RTSUnitType.h"

namespace RTSGame {

  class RTSTiledMap;

  namespace STATE {

    enum E
    {
      kAttack = 0,
      kDie,
      kIdle,
      kRun
    };
  }


  class RTSUnit
  {
  public:

    RTSUnit( RTSTexture &texture, Vector<Animation> &animation );

    ~RTSUnit();

    void
      Update( float deltaTime );

    void
      Render( RTSTiledMap * tileMap );

    void
      setPosition( float x, float y );


  private:


    sf::Vector2f m_position;

    float m_elapsedTime;

    uint32 m_frameCount;

    Vector<Animation> m_animation;

    RTSTexture * m_texture;

    STATE::E m_state;

    DIRECTIONS::E m_direction;

  };
}