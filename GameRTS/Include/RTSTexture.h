#pragma once

#include <gePrerequisitesUtil.h>
#include <geDegree.h>
#include <geVector2.h>
#include <geVector2I.h>

#include <SFML/Graphics.hpp>

using namespace::geEngineSDK;
namespace RTSGame {
  class RTSTexture
  {
  public:
    RTSTexture();
    ~RTSTexture();

    void
      loadFromFile( sf::RenderTarget* pTarget, const String& );

    sf::Texture*
      getTexture()
    {
      GE_ASSERT( m_texture );
      return m_texture;
    }

    uint32
      getWidth()
    {
      GE_ASSERT( m_texture );
      return m_texture->getSize().x;
    }

    uint32
      getHeight()
    {
      GE_ASSERT( m_texture );
      return m_texture->getSize().y;
    }

    void
      setColor( uint8 red, uint8 green = 255, uint8 blue = 255, uint8 alpha = 255 )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setColor( sf::Color( red, green, blue, alpha ) );
    }

    void
      setPosition( float x, float y )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setPosition( x, y );
    }

    void
      setPosition( int32 x, int32 y )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setPosition( static_cast< float >( x ), static_cast< float >( y ) );
    }

    void
      setPosition( const Vector2& position )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setPosition( position.x, position.y );
    }

    void
      setPosition( const Vector2I& position )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setPosition( static_cast< float >( position.x ),
                             static_cast< float >( position.y ) );
    }

    void
      setOrigin( float x, float y )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setOrigin( x, y );
    }

    void
      setOrigin( int32 x, int32 y )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setOrigin( static_cast< float >( x ), static_cast< float >( y ) );
    }

    void
      setOrigin( const Vector2& origin )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setOrigin( origin.x, origin.y );
    }

    void
      setOrigin( const Vector2I& origin )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setOrigin( static_cast< float >( origin.x ),
                           static_cast< float >( origin.y ) );
    }

    void
      setRotation( Degree angle )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setRotation( angle.valueDegrees() );
    }

    void
      setScale( float x, float y )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setScale( x, y );
    }

    void
      setScale( const Vector2& scaleFactor )
    {
      GE_ASSERT( m_sprite );
      m_sprite->setScale( scaleFactor.x, scaleFactor.y );
    }

    void
      setSrcRect( int32 x, int32 y, int32 w, int32 h )
    {
      GE_ASSERT( m_sprite );
      sf::IntRect srcRect( x, y, w, h );
      m_sprite->setTextureRect( srcRect );
    }

    void
      draw();

  protected:

  private:
    sf::RenderTarget* m_pTarget;
    sf::Texture* m_texture;
    sf::Sprite* m_sprite;
  };
}