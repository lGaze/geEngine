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

    RTSUnit( RTSTexture &texture, const Vector<Animation> &animation );

    ~RTSUnit();

    void
      Update( float deltaTime );

    void
      Render( RTSTiledMap * tileMap );

    void
      setPosition( float x, float y );

    sf::Vector2f
      getPosition()  {
      return m_position;
    }

    void
      setSelected( bool slc) {
      selected = slc;
    }

    void
      initCircle( sf::RenderTarget * target );

    void
      setCirclePosition( float x, float y );

    void
      drawCircle();

  private:


    sf::Vector2f m_position;

    float m_elapsedTime;

    uint32 m_frameCount;

    Vector<Animation> m_animation;

    RTSTexture * m_texture;

    STATE::E m_state;

    DIRECTIONS::E m_direction;

    sf::RenderTarget * m_target;

    sf::CircleShape m_Circle;

    bool selected;

  };
}