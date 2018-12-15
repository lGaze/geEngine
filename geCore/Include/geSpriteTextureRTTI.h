/*****************************************************************************/
/**
 * @file    geSpriteTextureRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/29
 * @brief   RTTI Objects for geSpriteTexture.
 *
 * RTTI Objects for geSpriteTexture.
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
#include "geSpriteTexture.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT SpriteTextureRTTI
    : public RTTIType<SpriteTexture, Resource, SpriteTextureRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_REFL(m_atlasTexture, 0)
      GE_RTTI_MEMBER_PLAIN(m_uvOffset, 1)
      GE_RTTI_MEMBER_PLAIN(m_uvScale, 2)
      GE_RTTI_MEMBER_PLAIN(m_animation, 3)
      GE_RTTI_MEMBER_PLAIN(m_playback, 4)
    GE_END_RTTI_MEMBERS

   public:
    const String&
    getRTTIName() override {
      static String name = "SpriteTexture";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_SpriteTexture;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return SpriteTexture::createEmpty();
    }
  };

  template<>
  struct RTTIPlainType<SpriteSheetGridAnimation>
  {
    enum { kID = TYPEID_CORE::kID_SpriteSheetGridAnimation };
    enum { kHasDynamicSize = 1 };

    static void
    toMemory(const SpriteSheetGridAnimation& data, char* memory) {
      static constexpr uint32 VERSION = 0;
      const uint32 size = getDynamicSize(data);

      memory = rttiWriteElement(size, memory);
      memory = rttiWriteElement(VERSION, memory);
      memory = rttiWriteElement(data.numRows, memory);
      memory = rttiWriteElement(data.numColumns, memory);
      memory = rttiWriteElement(data.count, memory);
      memory = rttiWriteElement(data.fps, memory);
    }

    static uint32
    fromMemory(SpriteSheetGridAnimation& data, char* memory) {
      uint32 size = 0;
      memory = rttiReadElement(size, memory);

      uint32 version = 0;
      memory = rttiReadElement(version, memory);

      switch (version)
      {
        case 0:
        {
          memory = rttiReadElement(data.numRows, memory);
          memory = rttiReadElement(data.numColumns, memory);
          memory = rttiReadElement(data.count, memory);
          memory = rttiReadElement(data.fps, memory);
          break;
        }
        default:
          LOGERR("Unknown version. Unable to deserialize.");
          break;
      }

      return size;
    }

    static uint32
    getDynamicSize(const SpriteSheetGridAnimation& data) {
      uint32 size = sizeof(uint32) * 2 +
        rttiGetElementSize(data.numRows) +
        rttiGetElementSize(data.numColumns) +
        rttiGetElementSize(data.count) +
        rttiGetElementSize(data.fps);
      return size;
    }
  };
}
