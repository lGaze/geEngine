#include "RTSUnit.h"
#include "RTSTiledMap.h"

namespace RTSGame {

RTSUnit::RTSUnit( RTSTexture & texture,
                  const Vector<Animation>& animation ) : m_texture(&texture),
                                                   m_animation(animation),
                                                   m_state(STATE::kIdle),
                                                   m_direction(DIRECTIONS::kE),
                                                   m_elapsedTime(0),
                                                   m_frameCount(0),
                                                   m_Circle(TILESIZE_X / 2)
                                                    { }

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
  tileMap->getMapToScreenCoords( static_cast<int32>(m_position.x), 
                                 static_cast< int32 >(m_position.y), 
                                 coordX, coordY );

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

void RTSUnit::initCircle( sf::RenderTarget * target )
{
  m_target = target;
  m_Circle.setScale( sf::Vector2( 1.0f, 0.5f ) );
  m_Circle.scale( 0.5f, 1.0f );


  m_Circle.setOutlineThickness( 1.5 );
  m_Circle.setFillColor( sf::Color::Transparent );
  float x = m_Circle.getLocalBounds().width / 2;
  float y = m_Circle.getLocalBounds().height / 2;
  m_Circle.setOrigin( x, y );
}

void RTSUnit::setCirclePosition( float x, float y )
{
  m_Circle.setPosition( x, y );
}

void RTSUnit::drawCircle()
{
  m_target->draw( m_Circle );
}

}