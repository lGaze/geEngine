/*****************************************************************************/
/**
 * @file    geSpriteTexture.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geResource.h"

#include <geVector2.h>

namespace geEngineSDK {
  using std::move;

  struct GE_SCRIPT_EXPORT(m:Rendering, pl:true) SpriteSheetGridAnimation
  {
    SpriteSheetGridAnimation() = default;

    SpriteSheetGridAnimation(uint32 _numRows,
                             uint32 _numColumns,
                             uint32 _count,
                             uint32 _fps)
      : numRows(_numRows),
        numColumns(_numColumns),
        count(_count),
        fps(_fps)
    {}

    /**
     * @brief Number of rows to divide the parent's texture area.
     *        Determines height of the individual frame
     *        (depends on parent texture size).
     */
    uint32 numRows = 1;

    /**
     * @brief Number of columns to divide the parent's texture area.
     *        Determines column of the individual frame
     *        (depends on parent texture size).
     */
    uint32 numColumns = 1;

    /**
     * @brief Number of frames in the animation.
     *        Must be less or equal than @p numRows * @p numColumns.
     */
    uint32 count = 1;

    /**
     * @brief How many frames to evaluate each second.
     *        Determines the animation speed.
     */
    uint32 fps = 8;
  };

  /**
   * @brief Type of playback to use for an animation of a SpriteTexture.
   */
  enum class GE_SCRIPT_EXPORT(m:Rendering) SpriteAnimationPlayback {
    /**
     * Do not animate.
     */
    None,

    /**
     * Animate once until the end of the animation is reached.
     */
    Normal,

    /**
     * Animate to the end of the animation then loop around.
     */
    Loop,

    /**
     * Loop the animation but reverse playback when the end is reached.
     */
    PingPong
  };

  /**
   * @brief Base class used for both simulation and core thread SpriteTexture
   *        implementations.
   */
  class GE_CORE_EXPORT SpriteTextureBase
  {
   public:
    SpriteTextureBase(const Vector2& uvOffset, const Vector2& uvScale)
      : m_uvOffset(uvOffset),
        m_uvScale(uvScale)
    {}

    virtual ~SpriteTextureBase() = default;

    /**
     * @brief Determines the offset into the referenced texture where the
     *        sprite starts. The offset is in UV coordinates, in range [0, 1].
     */
    GE_SCRIPT_EXPORT(n:Offset, pr:setter)
    void
    setOffset(const Vector2& offset) {
      m_uvOffset = offset;
      _markCoreDirty();
    }

    /**
     * @copydoc setOffset()
     */
    GE_SCRIPT_EXPORT(n:Offset, pr:getter)
    Vector2
    getOffset() const {
      return m_uvOffset;
    }

    /**
     * @brief Determines the size of the sprite in the referenced texture.
     *        Size is in UV coordinates, range [0, 1].
     */
    GE_SCRIPT_EXPORT(n:Scale, pr:setter)
    void
    setScale(const Vector2& scale) {
      m_uvScale = scale;
      _markCoreDirty();
    }

    /**
     * @copydoc setScale()
     */
    GE_SCRIPT_EXPORT(n:Scale, pr:getter)
    Vector2
    getScale() const {
      return m_uvScale;
    }

    /**
     * @brief Transforms wanted UV coordinates into coordinates you can use for
     *        sampling the internal texture.
     */
    Vector2
    transformUV(const Vector2& uv) const {
      return m_uvOffset + uv * m_uvScale;
    }

    /**
     * @brief Evaluates the UV coordinate offset and size to use at the
     *        specified time. If the sprite texture doesn't have animation
     *        playback enabled then just the default offset and size will be
     *        provided, otherwise the animation will be evaluated and
     *        appropriate UV returned.
     */
    Box2D
    evaluate(float t) const;

    /**
     * @brief Sets properties describing sprite animation.
     *        The animation splits the sprite area into a grid of sub-images
     *        which can be evaluated over time. In order to view the animation
     *        you must also enable playback through setAnimationPlayback().
     */
    GE_SCRIPT_EXPORT(n:Animation, pr:setter)
    void
    setAnimation(const SpriteSheetGridAnimation& anim) {
      m_animation = anim;
      _markCoreDirty();
    }

    /**
     * @copydoc setAnimation
     */
    GE_SCRIPT_EXPORT(n:Animation, pr:getter)
    const SpriteSheetGridAnimation&
    getAnimation() const {
      return m_animation;
    }

    /**
     * @brief Determines if and how should the sprite animation play.
     */
    GE_SCRIPT_EXPORT(n:AnimationPlayback, pr:setter)
    void
    setAnimationPlayback(SpriteAnimationPlayback playback) {
      m_playback = playback;
      _markCoreDirty();
    }

    /**
     * @copydoc setAnimationPlayback
     */
    GE_SCRIPT_EXPORT(n:AnimationPlayback, pr:getter)
    SpriteAnimationPlayback
    getAnimationPlayback() const {
      return m_playback;
    };

   protected:
    /**
     * @brief Marks the contents of the simulation thread object as dirty,
     *        causing it to sync with its core thread counterpart.
     */
    virtual void
    _markCoreDirty() {}

    Vector2 m_uvOffset;
    Vector2 m_uvScale;

    SpriteAnimationPlayback m_playback = SpriteAnimationPlayback::None;
    SpriteSheetGridAnimation m_animation;
  };

  /**
   * @brief Template base class used for both simulation and core thread
   *        SpriteTexture implementations.
   */
  template<bool Core>
  class GE_CORE_EXPORT TSpriteTexture : public SpriteTextureBase
  {
   public:
    using TextureType = CoreVariantHandleType<Texture, Core>;

    TSpriteTexture(const Vector2& uvOffset,
                   const Vector2& uvScale,
                   TextureType atlasTexture)
      : SpriteTextureBase(uvOffset, uvScale),
        m_atlasTexture(move(atlasTexture))
    {}

    virtual ~TSpriteTexture() = default;

    /**
     * @brief Enumerates all the fields in the type and executes the specified
     *        processor action for each field.
     */
    template<class P>
    void
    rttiEnumFields(P p);

   protected:
    TextureType m_atlasTexture;
  };

  /**
   * @brief Texture that references a part of a larger texture by specifying an
   *        UV subset. When the sprite texture is rendered only the portion of
   *        the texture specified by the UV subset will be rendered. This
   *        allows you to use the same texture which portion of the UV is
   *        selected over time.
   */
  class GE_CORE_EXPORT GE_SCRIPT_EXPORT(m:Rendering) SpriteTexture
    : public Resource, public TSpriteTexture<false>
  {
   public:
    /*
     * @brief Determines the internal texture that the sprite texture references.
     */
    GE_SCRIPT_EXPORT(n:Texture, pr:setter)
    void
    setTexture(const HTexture& texture) {
      m_atlasTexture = texture;
      markDependenciesDirty();
    }

    /**
     * @copydoc setTexture()
     */
    GE_SCRIPT_EXPORT(n:Texture, pr:getter)
    const HTexture&
    getTexture() const {
      return m_atlasTexture;
    }

    /**
     * @brief Returns width of the sprite texture in pixels.
     */
    GE_SCRIPT_EXPORT(n:Width, pr:getter)
    uint32
    getWidth() const;

    /**
     * @brief Returns height of the sprite texture in pixels.
     */
    GE_SCRIPT_EXPORT(n:Height, pr:getter)
    uint32
    getHeight() const;

    /**
     * @brief Retrieves a core implementation of a sprite texture usable only
     *        from the core thread.
     */
    SPtr<geCoreThread::SpriteTexture>
    getCore() const;

    /**
     * @brief Creates a new sprite texture that references the entire area of
     *        the provided texture.
     */
    GE_SCRIPT_EXPORT(ec:SpriteTexture)
    static HSpriteTexture
    create(const HTexture& texture);
    
    GE_SCRIPT_EXPORT(ec:SpriteTexture)
    static HSpriteTexture
    create(const Vector2& uvOffset,
           const Vector2& uvScale,
           const HTexture& texture);

    static bool
    checkIsLoaded(const HSpriteTexture& tex);

    /**
     * @brief Returns a dummy sprite texture.
     */
    static const HSpriteTexture&
    dummy();

    /**
     * @brief Creates a new SpriteTexture without a resource handle.
     *        Use create() for normal use.
     */
    static SPtr<SpriteTexture>
    _createPtr(const HTexture& texture);

    /**
     * @brief Creates a new SpriteTexture without a resource handle.
     *        Use create() for normal use.
     */
    static SPtr<SpriteTexture>
    _createPtr(const Vector2& uvOffset,
               const Vector2& uvScale,
               const HTexture& texture);

    /**
     * @copydoc SpriteTextureBase::_markCoreDirty
     */
    void
    _markCoreDirty() override;

   private:
    friend class SpriteTextureRTTI;

    SpriteTexture(const Vector2& uvOffset,
                  const Vector2& uvScale,
                  const HTexture& texture);
    
    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;
    
    /**
     * @copydoc CoreObject::syncToCore
     */
    CoreSyncData
    syncToCore(FrameAlloc* allocator) override;

    void
    getResourceDependencies(FrameVector<HResource>& dependencies) const override;

    /**
     * @copydoc CoreObject::getCoreDependencies
     */
    void
    getCoreDependencies(Vector<CoreObject*>& dependencies) override;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/

    /**
     * @brief Creates a new empty and uninitialized sprite texture.
     *        To be used by factory methods.
     */
    static SPtr<SpriteTexture>
    createEmpty();

   public:
    friend class SpriteTextureRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread version of a bs::SpriteTexture.
     * @note Core thread.
     */
    class GE_CORE_EXPORT SpriteTexture
      : public CoreObject, public TSpriteTexture<true>
    {
     public:
      /**
       * @brief Determines the internal texture that the sprite texture
       *        references.
       */
      void
      setTexture(const SPtr<geCoreThread::Texture>& texture) {
        m_atlasTexture = texture;
      }

      /**
       * @copydoc setTexture()
       */
      const SPtr<geCoreThread::Texture>&
      getTexture() const {
        return m_atlasTexture;
      }

     private:
      friend class geEngineSDK::SpriteTexture;

      SpriteTexture(const Vector2& uvOffset,
                    const Vector2& uvScale,
                    SPtr<Texture> texture,
                    const SpriteSheetGridAnimation& anim,
                    SpriteAnimationPlayback playback);

      /**
       * @copydoc CoreObject::syncToCore
       */
      void
      syncToCore(const CoreSyncData& data) override;
    };
  }
}
