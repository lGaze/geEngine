#include "RTSUnit.h"
#include "RTSTiledMap.h"

namespace RTSGame {

RTSUnit::RTSUnit( RTSTexture & texture,
                  Vector<Animation>& animation ) : m_texture(&texture),
                                                   m_animation(animation),
                                                   m_state(STATE::kIdle),
                                                   m_direction(DIRECTIONS::kE),
                                                   m_elapsedTime(0),
                                                   m_frameCount(0) { }

RTSUnit::~RTSUnit()
{
}

void RTSUnit::Update( float deltaTime )
{
  m_elapsedTime += deltaTime;
}

void RTSUnit::Render( RTSTiledMap * tileMap )
{
  int32 coordX, coordY;
  tileMap->getMapToScreenCoords( m_position.x, m_position.y, coordX, coordY );

  float frameTime = 
    m_animation[m_state].duration / m_animation[m_state].numFrames;
  AnimationFrame& currentFrame = 
    m_animation[m_state].frames[m_direction][m_frameCount];
  m_texture->setSrcRect( currentFrame.x, 
                         currentFrame.y, 
                         currentFrame.w, 
                         currentFrame.h );

  m_texture->setPosition( coordX + (TILESIZE_X/4), coordY - (currentFrame.h/2 ));
 
  if (m_elapsedTime > frameTime)
  {
    m_frameCount = ( ++m_frameCount ) % m_animation[m_state].numFrames;
    m_elapsedTime = 0.0f;
  }

  m_texture->draw();
  
}

void
RTSUnit::setPosition( float x, float y )
{
  m_position.x = x;
  m_position.y = y;
}

}