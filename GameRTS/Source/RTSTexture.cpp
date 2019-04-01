#include "RTSTexture.h"

#include <geException.h>

using namespace geEngineSDK;
namespace RTSGame {

  RTSTexture::RTSTexture()
    : m_texture( nullptr ),
    m_sprite( nullptr )
  {
  }

  RTSTexture::~RTSTexture()
  {
    if ( nullptr != m_texture )
    {
      ge_delete( m_texture );
    }
    if ( nullptr != m_sprite )
    {
      ge_delete( m_sprite );
    }
  }

  void
    RTSTexture::draw()
  {
    GE_ASSERT( nullptr != m_texture );
    GE_ASSERT( nullptr != m_sprite );
    m_sprite->setTexture( *m_texture );
    m_pTarget->draw( *m_sprite );
  }

  void
    RTSTexture::loadFromFile( sf::RenderTarget* pTarget, const String& fileName )
  {
    GE_ASSERT( nullptr == m_texture );
    GE_ASSERT( nullptr == m_sprite );

    m_texture = ge_new<sf::Texture>();
    m_sprite = ge_new<sf::Sprite>();

    if ( !m_texture->loadFromFile( fileName.c_str() ) )
    {
      GE_EXCEPT( FileNotFoundException, "Texture not found: " + fileName );
    }

    m_pTarget = pTarget;
  }
}
