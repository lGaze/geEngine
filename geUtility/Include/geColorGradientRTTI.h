/*****************************************************************************/
/**
 * @file    geRandom.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/06
 * @brief   Generates pseudo random numbers using the Xorshift128 algorithm.
 *
 * Generates pseudo random numbers using the Xorshift128 algorithm.
 * Suitable for high performance requirements.
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
#include "gePrerequisitesUtil.h"
#include "geNumericLimits.h"
#include "geColorGradient.h"
#include "geException.h"
#include "geDebug.h"

namespace geEngineSDK {
  template<>
  struct RTTIPlainType<ColorGradient>
  {
    enum { kID = TYPEID_UTILITY::kID_ColorGradient };
    enum { kHasDynamicSize = 1 };

    static void
    toMemory(const ColorGradient& data, char* memory) {
      uint32 size = getDynamicSize(data);
      const uint32 curSize = sizeof(uint32);
      memcpy(memory, &size, curSize);
      memory += curSize;

      const uint32 version = 0;
      memory = rttiWriteElement(version, memory);

      for (uint32 i = 0; i < ColorGradient::MAX_KEYS; ++i) {
        memory = rttiWriteElement(data.m_colors[i], memory);
        memory = rttiWriteElement(data.m_times[i], memory);
      }

      memory = rttiWriteElement(data.m_numKeys, memory);
      memory = rttiWriteElement(data.m_duration, memory);
    }

    static uint32
    fromMemory(ColorGradient& data, char* memory) {
      uint32 size;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 version;
      memory = rttiReadElement(version, memory);

      switch (version) {
        case 0:
          for (uint32 i = 0; i < ColorGradient::MAX_KEYS; ++i) {
            memory = rttiReadElement(data.m_colors[i], memory);
            memory = rttiReadElement(data.m_times[i], memory);
          }

          memory = rttiReadElement(data.m_numKeys, memory);
          memory = rttiReadElement(data.m_duration, memory);
        break;

        default:
          LOGERR("Unknown version of ColorGradient data.\n"
                 "Unable to deserialize.");
        break;
      }

      return size;
    }

    static uint32
    getDynamicSize(const ColorGradient& data) {
      const uint64 dataSize =
        rttiGetElementSize(data.m_colors[0]) * ColorGradient::MAX_KEYS +
        rttiGetElementSize(data.m_times[0]) * ColorGradient::MAX_KEYS +
        rttiGetElementSize(data.m_numKeys) +
        rttiGetElementSize(data.m_duration) + sizeof(uint32) * 2;

#if GE_DEBUG_MODE
      if (NumLimit::MAX_UINT32 < dataSize) {
        GE_EXCEPT(InternalErrorException,
                  "Data overflow! Size doesn't fit into 32 bits.");
      }
#endif

      return static_cast<uint32>(dataSize);
    }
  };
}
