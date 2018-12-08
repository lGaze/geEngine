/*****************************************************************************/
/**
 * @file    geBitwise.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/06/01
 * @brief   Class for manipulating bit patterns.
 *
 * Class capable of storing any general type, and safely extracting the proper
 * type from the internal data.
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
#include "geFloat32.h"
#include "geFloat16.h"
#include "geFloat10.h"
#include "geFloat11.h"
#include "geMath.h"

namespace geEngineSDK {
  /**
   * @brief Class for manipulating bit patterns.
   */
  class Bitwise
  {
   public:
     /**
      * @brief Returns the most significant bit set in a value.
      */
    template<typename IntType>
    static uint32
    mostSignificantBitSet(IntType value) {
      uint32 result = 0;
      while (0 != value) {
        ++result;
        value >>= 1;
      }
      return result - 1;
    }

    /**
     * @brief Returns the power-of-two number greater or equal to the provided value.
     */
    static uint32
    nextPow2(uint32 n) {
      --n;
      n |= n >> 16;
      n |= n >> 8;
      n |= n >> 4;
      n |= n >> 2;
      n |= n >> 1;
      ++n;
      return n;
    }

    /**
     * @brief Returns the power-of-two number closest to the provided value.
     */
    static uint32
    closestPow2(uint32 n) {
      uint32 next = nextPow2(n);

      uint32 prev = next >> 1;
      if (n - prev < next - n) {
        return prev;
      }
      return next;
    }

#if GE_COMPILER == GE_COMPILER_MSVC
# pragma intrinsic(_BitScanReverse,_BitScanForward)
#endif
    /**
     * @brief Finds the most-significant non-zero bit in the provided value and
     *        returns the index of that bit.
     */
    static uint32
    mostSignificantBit(uint32 val) {
#if GE_COMPILER == GE_COMPILER_MSVC
      unsigned long index;
      _BitScanReverse(&index, val);
      return index;
#elif GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
      return 31 - __builtin_clz(val);
#else
      static_assert(false, "Not implemented");
#endif
    }

    /**
     * @brief Finds the least-significant non-zero bit in the provided value
     *        and returns the index of that bit.
     */
    static uint32
    leastSignificantBit(uint32 val) {
#if GE_COMPILER == GE_COMPILER_MSVC
      unsigned long index;
      _BitScanForward(&index, val);
      return index;
#elif GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
      return __builtin_ctz(val);
#else
      static_assert(false, "Not implemented");
#endif
    }

#if GE_ARCH_TYPE == GE_ARCHITECTURE_x86_64
    /**
     * @brief Finds the most-significant non-zero bit in the provided value and
     *        returns the index of that bit.
     */
    static uint32
    mostSignificantBit(uint64 val) {
# if GE_COMPILER == GE_COMPILER_MSVC
      unsigned long index;
      _BitScanReverse(&index, static_cast<unsigned long>(val));
      return index;
# elif GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
      return 31 - __builtin_clzll(val);
# else
      static_assert(false, "Not implemented");
# endif
    }

    /**
     * @brief Finds the least-significant non-zero bit in the provided value
     *        and returns the index of that bit.
     */
    static uint32
    leastSignificantBit(uint64 val) {
# if GE_COMPILER == GE_COMPILER_MSVC
      unsigned long index;
      _BitScanForward64(&index, static_cast<unsigned long>(val));
      return index;
# elif GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
      return __builtin_ctzll(val);
# else
      static_assert(false, "Not implemented");
# endif
    }
#endif

    /**
     * @brief Determines whether the number is power-of-two or not.
     */
    template<typename T>
    static bool
    isPow2(T n) {
      return Math::isPowerOfTwo(static_cast<uint32>(n));
    }

    /**
     * @brief Returns the number of bits a pattern must be shifted right by to
     *        remove right-hand zeros.
     */
    template<typename T>
    static uint32
    getBitShift(T mask) {
      if (0 == mask) {
        return 0;
      }

      uint32 result = 0;
      while (0 == (mask & 1)) {
        ++result;
        mask >>= 1;
      }
      return result;
    }

    /**
     * @brief Takes a value with a given src bit mask, and produces another
     *        value with a desired bit mask.
     */
    template<typename SrcT, typename DestT>
    static DestT
    convertBitPattern(SrcT srcValue, SrcT srcBitMask, DestT destBitMask) {
      //Mask off irrelevant source value bits (if any)
      srcValue = srcValue & srcBitMask;

      //Shift source down to bottom of DWORD
      const uint32 srcBitShift = getBitShift(srcBitMask);
      srcValue >>= srcBitShift;

      //Get max value possible in source from srcMask
      const SrcT srcMax = srcBitMask >> srcBitShift;

      //Get max available in dest
      const uint32 destBitShift = getBitShift(destBitMask);
      const DestT destMax = destBitMask >> destBitShift;

      //Scale source value into destination, and shift back
      DestT destValue = (srcValue * destMax) / srcMax;
      return (destValue << destBitShift);
    }

    /**
     * @brief Convert N bit color channel value to P bits. It fills P bits with
     *        the bit pattern repeated. (this is /((1<<n)-1) in fixed point).
     */
    static uint32
    fixedToFixed(uint32 value, uint32 n, uint32 p) {
      if (n > p) {
        //Less bits required than available; this is easy
        value >>= n - p;
      }
      else if (n < p) {
        //More bits required than are there, do the fill
        //Use old fashioned division, probably better than a loop
        if (0 == value) {
          value = 0;
        }
        else if (value == (static_cast<uint32>(1) << n) - 1) {
          value = (1 << p) - 1;
        }
        else {
          value = value*(1 << p) / ((1 << n) - 1);
        }
      }
      return value;
    }

    /**
     * @brief Convert floating point color channel value between 0.0 and 1.0
     *        (otherwise clamped) to integer of a certain number of bits. Works
     *        for any value of bits between 0 and 31.
     */
    static uint32
    floatToFixed(const float value, const uint32 bits) {
      if (0.0f >= value) {
        return 0;
      }
      else if (1.0f <= value) {
        return (1 << bits) - 1;
      }
      else {
        return static_cast<uint32>(value * (1 << bits));
      }
    }

    /**
     * @brief Fixed point to float.
     */
    static float
    fixedToFloat(uint32 value, uint32 bits) {
      return static_cast<float>(value) / static_cast<float>((1 << bits) - 1);
    }

    /**
     * @brief Converts floating point value in range [0, 1] to an unsigned
     *        integer of a certain number of bits. Works for any value of bits
     *        between 0 and 31.
     */
    template<uint32 bits = 8>
    static uint32
    unormToUint(float value) {
      if (0.0f >= value) {
        return 0;
      }
      else if (1.0f <= value) {
        return (1 << bits) - 1;
      }

      return Math::round(value * (1 << bits));
    }

    /**
     * @brief Converts floating point value in range [0, 1] to an unsigned
     *        integer of a certain number of bits. Works for any value of bits
     *        between 0 and 31.
     */
    static uint32
    unormToUint(float value, uint32 bits) {
      if (value <= 0.0f) {
        return 0;
      }
      else if (value >= 1.0f) {
        return (1 << bits) - 1;
      }

      return static_cast<uint32>(value * (1 << bits));
    }

    /**
     * @brief Converts floating point value in range [-1, 1] to an unsigned
     *        integer of a certain number of bits. Works for any value of bits
     *        between 0 and 31.
     */
    template<uint32 bits = 8>
    static uint32
    snormToUint(float value) {
      return unormToUint<bits>((value + 1.0f) * 0.5f);
    }

    /**
     * @brief Converts floating point value in range [-1, 1] to an unsigned
     *        integer of a certain number of bits. Works for any value of bits
     *        between 0 and 31.
     */
    static uint32
    snormToUint(float value, uint32 bits) {
      return unormToUint((value + 1.0f) * 0.5f, bits);
    }

    /**
     * @brief Converts an unsigned integer to a floating point in range [0, 1].
     */
    template<uint32 bits = 8>
    static float
    uintToUnorm(uint32 value) {
      return static_cast<float>(value) / static_cast<float>((1 << bits) - 1);
    }

    /**
     * @brief Converts an unsigned integer to a floating point in range [0, 1].
     */
    static float
    uintToUnorm(uint32 value, uint32 bits) {
      return static_cast<float>(value) / static_cast<float>((1 << bits) - 1);
    }

    /**
     * @brief Converts an unsigned int to a floating point in range [-1, 1].
     */
    template<uint32 bits = 8>
    static float
    uintToSnorm(uint32 value) {
      return uintToUnorm<bits>(value) * 2.0f - 1.0f;
    }

    /**
     * @brief Converts an unsigned int to a floating point in range [-1, 1].
     */
    static float
    uintToSnorm(uint32 value, uint32 bits) {
      return uintToUnorm(value, bits) * 2.0f - 1.0f;
    }

    /**
     * @brief Write a n*8 bits integer value to memory in native endian.
     */
    static void
    intWrite(void *dest, const int32 n, const uint32 value) {
      switch (n)
      {
        case 1:
          (reinterpret_cast<uint8*>(dest))[0] = static_cast<uint8>(value);
          break;
        case 2:
          (reinterpret_cast<uint16*>(dest))[0] = static_cast<uint16>(value);
          break;
        case 3:
#if GE_ENDIAN == GE_ENDIAN_BIG
          (reinterpret_cast<uint8*>(dest))[0] = static_cast<uint8>((value >> 16) & 0xFF);
          (reinterpret_cast<uint8*>(dest))[1] = static_cast<uint8>((value >> 8) & 0xFF);
          (reinterpret_cast<uint8*>(dest))[2] = static_cast<uint8>(value & 0xFF);
#else
          (reinterpret_cast<uint8*>(dest))[2] = static_cast<uint8>((value >> 16) & 0xFF);
          (reinterpret_cast<uint8*>(dest))[1] = static_cast<uint8>((value >> 8) & 0xFF);
          (reinterpret_cast<uint8*>(dest))[0] = static_cast<uint8>(value & 0xFF);
#endif
          break;
        case 4:
          (reinterpret_cast<uint32*>(dest))[0] = static_cast<uint32>(value);
          break;
      }
    }

    /**
     * @brief Read a n*8 bits integer value to memory in native endian.
     */
    static uint32
    intRead(const void *src, int n) {
      uint8* pData = nullptr;

      switch (n)
      {
        case 1:
          return (reinterpret_cast<uint8*>(const_cast<void*>(src)))[0];
        case 2:
          return (reinterpret_cast<uint16*>(const_cast<void*>(src)))[0];
        case 3:
#if GE_ENDIAN == GE_ENDIAN_BIG
          pData = reinterpret_cast<uint8*>(const_cast<void*>(src));
          return (static_cast<uint32>(pData[0] << 16) |
                  static_cast<uint32>(pData[1] << 8) |
                  static_cast<uint32>(pData[2]));
#else
          pData = reinterpret_cast<uint8*>(const_cast<void*>(src));
          return (static_cast<uint32>(pData[0]) | 
                  static_cast<uint32>(pData[1] << 8) | 
                  static_cast<uint32>(pData[2] << 16));
#endif
        case 4:
          return (reinterpret_cast<uint32*>(const_cast<void*>(src)))[0];
      }
      
      return 0; //This should never happen, exception?, assert?
    }

    /**
     * @brief Convert a float32 to a float16 (NV_half_float).
     */
    static uint16
    floatToHalf(float i) {
      Float16 half(i);
      return half.encoded;
    }

    /**
     * @brief Converts float in uint32 format to a a half in uint16 format.
     */
    static uint16
    floatToHalfI(uint32 i) {
      Float32 realFloat;
      realFloat.integerValue = i;
      return Float16(realFloat.floatValue).encoded;
    }

    /**
     * @brief Convert a Float16 (NV_half_float) to a Float32.
     */
    static float
    halfToFloat(uint16 y) {
      Float16 half;
      half.encoded = y;
      return half.getFloat();
    }

    /**
     * @brief Converts a half in UINT16 format to a float in uint32 format.
     */
    static uint32
    halfToFloatI(uint16 y) {
      Float16 half;
      half.encoded = y;
      return Float32(half.getFloat()).integerValue;
    }

    /**
     * @brief Converts a 32-bit float to a 10-bit float according to OpenGL
     *        packed_float extension.
     */
    static uint32
    floatToFloat10(float v) {
      Float10 float10(v);
      return float10.encoded;
    }

    /**
     * @brief Converts a 32-bit float to a 11-bit float according to OpenGL
     *        packed_float extension.
     */
    static uint32
    floatToFloat11(float v) {
      Float11 float11(v);
      return float11.encoded;
    }

    /**
     * @brief Converts a 10-bit float to a 32-bit float according to OpenGL
     *        packed_float extension.
     */
    static float
    float10ToFloat(uint32 v) {
      Float10 float10;
      float10.encoded = v;
      return float10.getFloat();
    }

    /**
     * @brief Converts a 11-bit float to a 32-bit float according to OpenGL
     *        packed_float extension.
     */
    static float
    float11ToFloat(uint32 v) {
      Float11 float11;
      float11.encoded = v;
      return float11.getFloat();
    }

    /**
     * @brief Converts a float in range [-1,1] into an unsigned 8-bit integer.
     */
    static uint8
    quantize8BitSigned(float v) {
      return quantize8BitUnsigned(v * 0.5f + 0.5f);
    }

    /**
     * @brief Converts a float in range [0,1] into an unsigned 8-bit integer.
     */
    static uint8
    quantize8BitUnsigned(float v) {
      return static_cast<uint8>(v * 255.999f);
    }
  };
}
