/*****************************************************************************/
/**
 * @file    geSpriteTexture.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/30
 * @brief   Descriptor that describes a simple sprite sheet animation.
 *
 * Descriptor that describes a simple sprite sheet animation. The parent
 * texture is split into a grid of @p numRows x @p numColumns, each
 * representing one frame of the animation. Every frame is of equal size.
 * Frames are sequentially evaluated starting from the top-most row, iterating
 * over all columns in a row and then moving to next row, up to @p count
 * frames. Frames in rows/columns past @p count. @p fps frames are evaluated
 * every second, allowing you to control animation speed.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geSpriteTexture.h"
#include "geSpriteTextureRTTI.h"
#include "geTexture.h"
#include "geResources.h"
#include "geBuiltinResources.h"
#include "geCoreObjectSync.h"

#include <geBox2D.h>

namespace geEngineSDK {
  using std::move;
  using std::static_pointer_cast;

  Box2D
  SpriteTextureBase::evaluate(float t) const {
    if (SpriteAnimationPlayback::None == m_playback) {
      return Box2D(Vector2(m_uvOffset.x, m_uvOffset.y),
                   Vector2(m_uvScale.x, m_uvScale.y));
    }

    //NOTE: Duration could be pre-calculated
    float duration = 0.0f;
    if (m_animation.fps > 0) {
      duration = m_animation.count / static_cast<float>(m_animation.fps);
    }

    switch (m_playback)
    {
      default:
      case SpriteAnimationPlayback::Normal:
        t = Math::clamp(t, 0.0f, duration);
        break;
      case SpriteAnimationPlayback::Loop:
        t = Math::repeat(t, duration);
        break;
      case SpriteAnimationPlayback::PingPong:
        t = Math::pingPong(t, duration);
        break;
    }

    const float pct = t / duration;
    uint32 frame = 0;

    if (m_animation.count > 0) {
      frame = Math::clamp(static_cast<uint32>(Math::round(pct * m_animation.count)),
                          0U,
                          m_animation.count - 1);
    }

    const uint32 row = frame / m_animation.numRows;
    const uint32 column = frame % m_animation.numColumns;

    Box2D output;

    //NOTE: These could be pre-calculated
    output.m_max.x = m_uvScale.x / m_animation.numColumns;
    output.m_max.y = m_uvScale.y / m_animation.numRows;

    output.m_min.x = m_uvOffset.x + column * output.m_max.x;
    output.m_min.y = m_uvOffset.y + row * output.m_max.y;

    return output;
  }

  template<bool Core>
  template<class P>
  void
  TSpriteTexture<Core>::rttiEnumFields(P p) {
    p(m_uvOffset);
    p(m_uvScale);
    p(m_animation);
    p(m_playback);
    p(m_atlasTexture);
  }

  SpriteTexture::SpriteTexture(const Vector2& uvOffset,
                               const Vector2& uvScale,
                               const HTexture& texture)
    : TSpriteTexture(uvOffset, uvScale, texture)
  {}

  const HSpriteTexture&
  SpriteTexture::dummy() {
    return BuiltinResources::instance().getDummySpriteTexture();
  }

  bool
  SpriteTexture::checkIsLoaded(const HSpriteTexture& tex) {
    return nullptr != tex &&
           tex.isLoaded(false) &&
           tex->getTexture() != nullptr &&
           tex->getTexture().isLoaded(false);
  }

  uint32
  SpriteTexture::getWidth() const {
    return Math::round(m_atlasTexture->getProperties().getWidth() * m_uvScale.x);
  }

  uint32
  SpriteTexture::getHeight() const {
    return Math::round(m_atlasTexture->getProperties().getHeight() * m_uvScale.y);
  }

  void
  SpriteTexture::_markCoreDirty() {
    markCoreDirty();
  }

  SPtr<geCoreThread::CoreObject>
  SpriteTexture::createCore() const {
    SPtr<geCoreThread::Texture> texturePtr;
    if (m_atlasTexture.isLoaded()) {
      texturePtr = m_atlasTexture->getCore();
    }

    geCoreThread::SpriteTexture*
      spriteTexture = new (ge_alloc<geCoreThread::SpriteTexture>())
      geCoreThread::SpriteTexture(m_uvOffset,
                                  m_uvScale,
                                  move(texturePtr),
                                  m_animation,
                                  m_playback);

    auto spriteTexPtr = ge_shared_ptr<geCoreThread::SpriteTexture>(spriteTexture);
    spriteTexPtr->_setThisPtr(spriteTexPtr);

    return spriteTexPtr;
  }

  CoreSyncData
  SpriteTexture::syncToCore(FrameAlloc* allocator) {
    uint32 size = coreSyncGetElemSize(*this);
    uint8* buffer = allocator->alloc(size);
    auto dataPtr = reinterpret_cast<char*>(buffer);
    dataPtr = coreSyncWriteElem(*this, dataPtr);

    return CoreSyncData(buffer, size);
  }

  void
  SpriteTexture::getResourceDependencies(FrameVector<HResource>& dependencies) const {
    if (nullptr != m_atlasTexture) {
      dependencies.push_back(m_atlasTexture);
    }
  }

  void
  SpriteTexture::getCoreDependencies(Vector<CoreObject*>& dependencies) {
    if (m_atlasTexture.isLoaded()) {
      dependencies.push_back(m_atlasTexture.get());
    }
  }

  SPtr<geCoreThread::SpriteTexture>
  SpriteTexture::getCore() const {
    return static_pointer_cast<geCoreThread::SpriteTexture>(m_coreSpecific);
  }

  HSpriteTexture
  SpriteTexture::create(const HTexture& texture) {
    SPtr<SpriteTexture> texturePtr = _createPtr(texture);
    return static_resource_cast<SpriteTexture>
             (g_resources()._createResourceHandle(texturePtr));
  }

  HSpriteTexture
  SpriteTexture::create(const Vector2& uvOffset,
                        const Vector2& uvScale,
                        const HTexture& texture) {
    SPtr<SpriteTexture> texturePtr = _createPtr(uvOffset, uvScale, texture);
    return static_resource_cast<SpriteTexture>
             (g_resources()._createResourceHandle(texturePtr));
  }

  SPtr<SpriteTexture>
  SpriteTexture::_createPtr(const HTexture& texture) {
    auto texturePtr = ge_core_ptr<SpriteTexture>(new (ge_alloc<SpriteTexture>())
      SpriteTexture(Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), texture));

    texturePtr->_setThisPtr(texturePtr);
    texturePtr->initialize();

    return texturePtr;
  }

  SPtr<SpriteTexture>
  SpriteTexture::_createPtr(const Vector2& uvOffset,
                            const Vector2& uvScale,
                            const HTexture& texture) {
    auto texturePtr = ge_core_ptr<SpriteTexture>(new (ge_alloc<SpriteTexture>())
      SpriteTexture(uvOffset, uvScale, texture));

    texturePtr->_setThisPtr(texturePtr);
    texturePtr->initialize();

    return texturePtr;
  }

  SPtr<SpriteTexture>
  SpriteTexture::createEmpty() {
    auto texturePtr = ge_core_ptr<SpriteTexture>(new (ge_alloc<SpriteTexture>())
      SpriteTexture(Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), HTexture()));

    texturePtr->_setThisPtr(texturePtr);
    texturePtr->initialize();

    return texturePtr;
  }

  RTTITypeBase*
  SpriteTexture::getRTTIStatic() {
    return SpriteTextureRTTI::instance();
  }

  RTTITypeBase*
  SpriteTexture::getRTTI() const {
    return SpriteTexture::getRTTIStatic();
  }

  namespace geCoreThread {
    SpriteTexture::SpriteTexture(const Vector2& uvOffset,
                                 const Vector2& uvScale,
                                 SPtr<Texture> texture,
                                 const SpriteSheetGridAnimation& anim,
                                 SpriteAnimationPlayback playback)
      : TSpriteTexture(uvOffset, uvScale, texture) {
      m_animation = anim;
      m_playback = playback;
    }

    void SpriteTexture::syncToCore(const CoreSyncData& data)
    {
      auto dataPtr = reinterpret_cast<char*>(data.getBuffer());
      dataPtr = coreSyncReadElem(*this, dataPtr);
    }
  }
}
