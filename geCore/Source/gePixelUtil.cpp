/*****************************************************************************/
/**
 * @file    gePixelUtil.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/12
 * @brief   Utility methods for converting and managing pixel data and formats.
 *
 * Utility methods for converting and managing pixel data and formats.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePixelUtil.h"
#include "geTexture.h"

#include <geBitwise.h>
#include <geColor.h>
#include <geMath.h>
#include <geException.h>

#include <nvtt/nvtt.h>

namespace geEngineSDK {
  using std::min;
  using std::array;
  using std::sort;

  /**
   * @brief Performs pixel data resampling using the point filter
   *        (nearest neighbor).
   *        Does not perform format conversions.
   * @tparam  elementSize  Size of a single pixel in bytes.
   */
  template<uint32 elementSize>
  struct NearestResampler
  {
    static void
    scale(const PixelData& source, const PixelData& dest) {
      uint8* sourceData = source.getData();
      uint8* destPtr = dest.getData();

      //Get steps for traversing source data in 16 / 48 fixed point format
      uint64 stepX = (static_cast<uint64>(source.getWidth()) << 48) / dest.getWidth();
      uint64 stepY = (static_cast<uint64>(source.getHeight())<< 48) / dest.getHeight();
      uint64 stepZ = (static_cast<uint64>(source.getDepth()) << 48) / dest.getDepth();

      //Offset half a pixel to start at pixel center
      uint64 curZ = (stepZ >> 1) - 1;

      for (uint32 z = dest.getFront(); z < dest.getBack(); ++z, curZ += stepZ) {
        uint32 offsetZ = static_cast<uint32>(curZ >> 48) * source.getSlicePitch();

        //Offset half a pixel to start at pixel center
        uint64 curY = (stepY >> 1) - 1;

        for (uint32 y = dest.getTop(); y < dest.getBottom(); ++y, curY += stepY) {
          uint32 offsetY = static_cast<uint32>(curY >> 48) * source.getRowPitch();

          //Offset half a pixel to start at pixel center
          uint64 curX = (stepX >> 1) - 1;

          for (uint32 x = dest.getLeft(); x < dest.getRight(); ++x, curX += stepX) {
            uint32 offsetX = static_cast<uint32>(curX >> 48);
            uint32 offsetBytes = elementSize * (offsetX + offsetY + offsetZ);
            uint8* curSourcePtr = sourceData + offsetBytes;
            memcpy(destPtr, curSourcePtr, elementSize);
            destPtr += elementSize;
          }
          destPtr += elementSize * dest.getRowSkip();
        }
        destPtr += elementSize * dest.getSliceSkip();
      }
    }
  };

  /**
   * @brief Performs pixel data resampling using the box filter (linear).
   *        Performs format conversions.
   */
  struct LinearResampler
  {
    static void
    scale(const PixelData& source, const PixelData& dest) {
      uint32 sourceElemSize = PixelUtil::getNumElemBytes(source.getFormat());
      uint32 destElemSize = PixelUtil::getNumElemBytes(dest.getFormat());

      uint8* sourceData = source.getData();
      uint8* destPtr = dest.getData();

      //Get steps for traversing source data in 16/48 fixed point precision format
      uint64 stepX = (static_cast<uint64>(source.getWidth()) << 48) / dest.getWidth();
      uint64 stepY = (static_cast<uint64>(source.getHeight())<< 48) / dest.getHeight();
      uint64 stepZ = (static_cast<uint64>(source.getDepth()) << 48) / dest.getDepth();

      //Contains 16/16 fixed point precision format. Most significant 16 bits
      //will contain the coordinate in the source image, and the least
      //significant 16 bits will contain the fractional part of the coordinate
      //that will be used for determining the blend amount.
      uint32 temp = 0;

      //Offset half a pixel to start at pixel center
      uint64 curZ = (stepZ >> 1) - 1;

      for (uint32 z = dest.getFront(); z < dest.getBack(); ++z, curZ += stepZ) {
        temp = uint32(curZ >> 32);
        temp = (temp > 0x8000) ? temp - 0x8000 : 0;
        uint32 sampleCoordZ1 = temp >> 16;
        uint32 sampleCoordZ2 = min(sampleCoordZ1 + 1,
                                   static_cast<uint32>(source.getDepth() - 1));
        float sampleWeightZ = (temp & 0xFFFF) / 65536.0f;

        //Offset half a pixel to start at pixel center
        uint64 curY = (stepY >> 1) - 1;
        for (uint32 y = dest.getTop(); y < dest.getBottom(); ++y, curY += stepY) {
          temp = uint32(curY >> 32);
          temp = (temp > 0x8000) ? temp - 0x8000 : 0;
          uint32 sampleCoordY1 = temp >> 16;
          uint32 sampleCoordY2 = min(sampleCoordY1 + 1,
                                     static_cast<uint32>(source.getHeight() - 1));
          float sampleWeightY = (temp & 0xFFFF) / 65536.0f;

          //Offset half a pixel to start at pixel center
          uint64 curX = (stepX >> 1) - 1;
          for (uint32 x = dest.getLeft(); x < dest.getRight(); ++x, curX += stepX) {
            temp = uint32(curX >> 32);
            temp = (temp > 0x8000) ? temp - 0x8000 : 0;
            uint32 sampleCoordX1 = temp >> 16;
            uint32 sampleCoordX2 = min(sampleCoordX1 + 1,
                                       static_cast<uint32>(source.getWidth() - 1));
            float sampleWeightX = (temp & 0xFFFF) / 65536.0f;

            LinearColor x1y1z1, x2y1z1, x1y2z1, x2y2z1;
            LinearColor x1y1z2, x2y1z2, x1y2z2, x2y2z2;

#define GETSOURCEDATA(x, y, z)                                                \
            sourceData +                                                      \
            sourceElemSize *                                                  \
            ((x) + (y) * source.getRowPitch() + (z) * source.getSlicePitch())
            PixelUtil::unpackColor(&x1y1z1,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX1,
                                     sampleCoordY1,
                                     sampleCoordZ1));

            PixelUtil::unpackColor(&x2y1z1,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX2,
                                     sampleCoordY1,
                                     sampleCoordZ1));
            PixelUtil::unpackColor(&x1y2z1,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX1,
                                     sampleCoordY2,
                                     sampleCoordZ1));
            PixelUtil::unpackColor(&x2y2z1,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX2,
                                     sampleCoordY2,
                                     sampleCoordZ1));
            PixelUtil::unpackColor(&x1y1z2,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX1,
                                     sampleCoordY1,
                                     sampleCoordZ2));
            PixelUtil::unpackColor(&x2y1z2,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX2,
                                     sampleCoordY1,
                                     sampleCoordZ2));
            PixelUtil::unpackColor(&x1y2z2,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX1,
                                     sampleCoordY2,
                                     sampleCoordZ2));
            PixelUtil::unpackColor(&x2y2z2,
                                   source.getFormat(),
                                   GETSOURCEDATA(sampleCoordX2,
                                     sampleCoordY2,
                                     sampleCoordZ2));
#undef GETSOURCEDATA

            LinearColor accum =
              x1y1z1 * ((1.0f - sampleWeightX) * (1.0f - sampleWeightY) *
                        (1.0f - sampleWeightZ)) +
              x2y1z1 * (sampleWeightX * (1.0f - sampleWeightY) *
                        (1.0f - sampleWeightZ)) +
              x1y2z1 * ((1.0f - sampleWeightX) * sampleWeightY *
                        (1.0f - sampleWeightZ)) +
              x2y2z1 * (sampleWeightX * sampleWeightY *
                        (1.0f - sampleWeightZ)) +
              x1y1z2 * ((1.0f - sampleWeightX) * (1.0f - sampleWeightY) *
                        sampleWeightZ) +
              x2y1z2 * (sampleWeightX * (1.0f - sampleWeightY) *
                        sampleWeightZ) +
              x1y2z2 * ((1.0f - sampleWeightX) * sampleWeightY *
                        sampleWeightZ) +
              x2y2z2 * (sampleWeightX * sampleWeightY * sampleWeightZ);

            PixelUtil::packColor(accum, dest.getFormat(), destPtr);
            destPtr += destElemSize;
          }
          destPtr += destElemSize * dest.getRowSkip();
        }
        destPtr += destElemSize * dest.getSliceSkip();
      }
    }
  };

  /**
   * @brief Performs pixel data resampling using the box filter (linear).
   *        Only handles float RGB or RGBA pixel data (32 bits per channel).
   */
  struct LinearResampler_Float32
  {
    static void scale(const PixelData& source, const PixelData& dest)
    {
      uint32 numSourceChannels = PixelUtil::getNumElemBytes(source.getFormat());
      numSourceChannels /= sizeof(float);

      uint32 numDestChannels = PixelUtil::getNumElemBytes(dest.getFormat());
      numDestChannels /= sizeof(float);

      float* sourceData = reinterpret_cast<float*>(source.getData());
      float* destPtr = reinterpret_cast<float*>(dest.getData());

      //Get steps for traversing source data in 16/48 fixed point precision format
      uint64 stepX = (static_cast<uint64>(source.getWidth()) << 48) / dest.getWidth();
      uint64 stepY = (static_cast<uint64>(source.getHeight())<< 48) / dest.getHeight();
      uint64 stepZ = (static_cast<uint64>(source.getDepth()) << 48) / dest.getDepth();

      //Contains 16/16 fixed point precision format. Most significant 16 bits
      //will contain the coordinate in the source image, and the least
      //significant 16 bits will contain the fractional part of the coordinate
      //that will be used for determining the blend amount.
      uint32 temp = 0;

      //Offset half a pixel to start at pixel center
      uint64 curZ = (stepZ >> 1) - 1;
      for (uint32 z = dest.getFront(); z < dest.getBack(); ++z, curZ += stepZ) {
        temp = uint32(curZ >> 32);
        temp = (temp > 0x8000) ? temp - 0x8000 : 0;
        uint32 sampleCoordZ1 = temp >> 16;
        uint32 sampleCoordZ2 = min(sampleCoordZ1 + 1,
                                   static_cast<uint32>(source.getDepth() - 1));
        float sampleWeightZ = (temp & 0xFFFF) / 65536.0f;

        //Offset half a pixel to start at pixel center
        uint64 curY = (stepY >> 1) - 1;
        for (uint32 y = dest.getTop(); y < dest.getBottom(); ++y, curY += stepY) {
          temp = uint32(curY >> 32);
          temp = (temp > 0x8000) ? temp - 0x8000 : 0;
          uint32 sampleCoordY1 = temp >> 16;
          uint32 sampleCoordY2 = min(sampleCoordY1 + 1,
                                     static_cast<uint32>(source.getHeight() - 1));
          float sampleWeightY = (temp & 0xFFFF) / 65536.0f;

          //Offset half a pixel to start at pixel center
          uint64 curX = (stepX >> 1) - 1;
          for (uint32 x = dest.getLeft(); x < dest.getRight(); ++x, curX += stepX) {
            temp = uint32(curX >> 32);
            temp = (temp > 0x8000) ? temp - 0x8000 : 0;
            uint32 sampleCoordX1 = temp >> 16;
            uint32 sampleCoordX2 = min(sampleCoordX1 + 1,
                                       static_cast<uint32>(source.getWidth() - 1));
            float sampleWeightX = (temp & 0xFFFF) / 65536.0f;

            //Process R,G,B,A simultaneously for cache coherence?
            float accum[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

#define ACCUM3(x,y,z,factor)                                                  \
						{                                                                 \
              float f = factor;                                               \
						  uint32 offset = (x + y * source.getRowPitch() + z *             \
                               source.getSlicePitch()) * numSourceChannels;   \
						  accum[0] += sourceData[offset + 0] * f;                         \
              accum[1] += sourceData[offset + 1] * f;                         \
						  accum[2] += sourceData[offset + 2] * f;                         \
            }
#define ACCUM4(x,y,z,factor)                                                  \
						{                                                                 \
              float f = factor;                                               \
						  uint32 offset = (x + y * source.getRowPitch() + z *             \
                               source.getSlicePitch()) * numSourceChannels;   \
						  accum[0] += sourceData[offset + 0] * f;                         \
              accum[1] += sourceData[offset + 1] * f;                         \
						  accum[2] += sourceData[offset + 2] * f;                         \
              accum[3] += sourceData[offset + 3] * f;                         \
            }

            if (3 == numSourceChannels || 3 == numDestChannels) { //RGB
              ACCUM3(sampleCoordX1,
                     sampleCoordY1,
                     sampleCoordZ1,
                     (1.0f - sampleWeightX) *
                     (1.0f - sampleWeightY) *
                     (1.0f - sampleWeightZ));

              ACCUM3(sampleCoordX2,
                     sampleCoordY1,
                     sampleCoordZ1,
                     sampleWeightX *
                     (1.0f - sampleWeightY) *
                     (1.0f - sampleWeightZ));

              ACCUM3(sampleCoordX1,
                     sampleCoordY2,
                     sampleCoordZ1,
                     (1.0f - sampleWeightX) *
                     sampleWeightY * (1.0f - sampleWeightZ));

              ACCUM3(sampleCoordX2,
                     sampleCoordY2,
                     sampleCoordZ1,
                     sampleWeightX * sampleWeightY * (1.0f - sampleWeightZ));

              ACCUM3(sampleCoordX1,
                     sampleCoordY1,
                     sampleCoordZ2,
                     (1.0f - sampleWeightX) *
                     (1.0f - sampleWeightY) *
                     sampleWeightZ);

              ACCUM3(sampleCoordX2,
                     sampleCoordY1,
                     sampleCoordZ2,
                     sampleWeightX *
                     (1.0f - sampleWeightY) *
                     sampleWeightZ);

              ACCUM3(sampleCoordX1,
                     sampleCoordY2,
                     sampleCoordZ2,
                     (1.0f - sampleWeightX) *
                     sampleWeightY *
                     sampleWeightZ);

              ACCUM3(sampleCoordX2,
                     sampleCoordY2,
                     sampleCoordZ2,
                     sampleWeightX *
                     sampleWeightY *
                     sampleWeightZ);
              accum[3] = 1.0f;
            }
            else {  //RGBA
              ACCUM4(sampleCoordX1,
                     sampleCoordY1,
                     sampleCoordZ1,
                     (1.0f - sampleWeightX) *
                     (1.0f - sampleWeightY) *
                     (1.0f - sampleWeightZ));

              ACCUM4(sampleCoordX2,
                     sampleCoordY1,
                     sampleCoordZ1,
                     sampleWeightX *
                     (1.0f - sampleWeightY) *
                     (1.0f - sampleWeightZ));

              ACCUM4(sampleCoordX1,
                     sampleCoordY2,
                     sampleCoordZ1,
                     (1.0f - sampleWeightX) *
                     sampleWeightY *
                     (1.0f - sampleWeightZ));

              ACCUM4(sampleCoordX2,
                     sampleCoordY2,
                     sampleCoordZ1,
                     sampleWeightX *
                     sampleWeightY *
                     (1.0f - sampleWeightZ));

              ACCUM4(sampleCoordX1,
                     sampleCoordY1,
                     sampleCoordZ2,
                     (1.0f - sampleWeightX) *
                     (1.0f - sampleWeightY) *
                     sampleWeightZ);

              ACCUM4(sampleCoordX2,
                     sampleCoordY1,
                     sampleCoordZ2,
                     sampleWeightX *
                     (1.0f - sampleWeightY) *
                     sampleWeightZ);

              ACCUM4(sampleCoordX1,
                     sampleCoordY2,
                     sampleCoordZ2,
                     (1.0f - sampleWeightX) *
                     sampleWeightY *
                     sampleWeightZ);

              ACCUM4(sampleCoordX2,
                     sampleCoordY2,
                     sampleCoordZ2,
                     sampleWeightX *
                     sampleWeightY *
                     sampleWeightZ);
            }

            memcpy(destPtr, accum, sizeof(float) * numDestChannels);

#undef ACCUM3
#undef ACCUM4

            destPtr += numDestChannels;
          }
          destPtr += numDestChannels * dest.getRowSkip();
        }
        destPtr += numDestChannels * dest.getSliceSkip();
      }
    }
  };

  //Byte linear resampler, does not do any format conversions.
  //only handles pixel formats that use 1 byte per color channel.
  //2D only; punts 3D pixelboxes to default LinearResampler (slow).
  //templated on bytes-per-pixel to allow compiler optimizations, such
  //as unrolling loops and replacing multiplies with bit shifts

  /**
   * @brief Performs pixel data resampling using the box filter (linear).
   *        Only handles pixel formats with one byte per channel.
   *        Does not perform format conversion.
   * @tparam  channels  Number of channels in the pixel format.
   */
  template<uint32 channels>
  struct LinearResampler_Byte
  {
    static void
    scale(const PixelData& source, const PixelData& dest) {
      //Only optimized for 2D
      if (source.getDepth() > 1 || dest.getDepth() > 1) {
        LinearResampler::scale(source, dest);
        return;
      }

      uint8* sourceData = reinterpret_cast<uint8*>(source.getData());
      uint8* destPtr = reinterpret_cast<uint8*>(dest.getData());

      //Get steps for traversing source data in 16/48 fixed point precision format
      uint64 stepX = (static_cast<uint64>(source.getWidth()) << 48) / dest.getWidth();
      uint64 stepY = (static_cast<uint64>(source.getHeight())<< 48) / dest.getHeight();

      //Contains 16/16 fixed point precision format. Most significant
      //16 bits will contain the coordinate in the source image, and the
      //least significant 16 bits will contain the fractional part of the
      //coordinate that will be used for determining the blend amount.
      uint32 temp;

      //Offset half a pixel to start at pixel center
      uint64 curY = (stepY >> 1) - 1;
      for (uint32 y = dest.getTop(); y < dest.getBottom(); ++y, curY += stepY) {
        temp = uint32(curY >> 36);
        temp = (temp > 0x800) ? temp - 0x800 : 0;
        uint32 sampleWeightY = temp & 0xFFF;
        uint32 sampleCoordY1 = temp >> 12;
        uint32 sampleCoordY2 = min(sampleCoordY1 + 1,
                                   static_cast<uint32>(source.getBottom() -
                                                       source.getTop() - 1));

        uint32 sampleY1Offset = sampleCoordY1 * source.getRowPitch();
        uint32 sampleY2Offset = sampleCoordY2 * source.getRowPitch();

        //Offset half a pixel to start at pixel center
        uint64 curX = (stepX >> 1) - 1;
        for (uint32 x = dest.getLeft(); x < dest.getRight(); ++x, curX += stepX) {
          temp = uint32(curX >> 36);
          temp = (temp > 0x800) ? temp - 0x800 : 0;
          uint32 sampleWeightX = temp & 0xFFF;
          uint32 sampleCoordX1 = temp >> 12;
          uint32 sampleCoordX2 = min(sampleCoordX1 + 1,
                                     static_cast<uint32>(source.getRight() -
                                                         source.getLeft() - 1));

          uint32 sxfsyf = sampleWeightX * sampleWeightY;
          for (uint32 k = 0; k < channels; ++k) {
            uint32 accum =
              sourceData[(sampleCoordX1 + sampleY1Offset) * channels + k] *
              (0x1000000 - (sampleWeightX << 12) - (sampleWeightY << 12) + sxfsyf) +
              sourceData[(sampleCoordX2 + sampleY1Offset) * channels + k] *
              ((sampleWeightX << 12) - sxfsyf) +
              sourceData[(sampleCoordX1 + sampleY2Offset) * channels + k] *
              ((sampleWeightY << 12) - sxfsyf) +
              sourceData[(sampleCoordX2 + sampleY2Offset) * channels + k] * sxfsyf;

            //Round up to byte size
            *destPtr = static_cast<uint8>((accum + 0x800000) >> 24);
            ++destPtr;
          }
        }
        destPtr += channels * dest.getRowSkip();
      }
    }
  };

  /**
   * @brief Data describing a pixel format.
   */
  struct PixelFormatDescription
  {
    /**
     * Name of the format.
     */
    const ANSICHAR* name;

    /**
     * Number of bytes one element (color value) uses.
     */
    uint8 elemBytes;

    /**
     * PixelFormatFlags set by the pixel format.
     */
    uint32 flags;

    /**
     * Data type of a single element of the format.
     */
    PIXEL_COMPONENT_TYPE::E componentType;

    /**
     * Number of elements in the format.
     */
    uint8 componentCount;

    /**
     * Number of bits per element in the format.
     */
    uint8 rbits, gbits, bbits, abits;

    /**
     * Masks used by packers/unpackers.
     */
    uint32 rmask, gmask, bmask, amask;

    /**
     * Shifts used by packers/unpackers.
     */
    uint8 rshift, gshift, bshift, ashift;
  };

#if GE_COMPILER == GE_COMPILER_MSVC
# pragma region Pixel_Formats
#endif

  /**
   * @brief A list of all available pixel formats.
   */
  PixelFormatDescription _pixelFormats[PixelFormat::kCOUNT] = {
    {
      "UNKNOWN",
      0,                                      /* Bytes per element */
      0,                                      /* Flags */
      PIXEL_COMPONENT_TYPE::kBYTE, 0,         /* Component type and count */
      0, 0, 0, 0,                             /* rbits, gbits, bbits, abits */
      0, 0, 0, 0,                             /* Masks */
      0, 0, 0, 0,                             /* Shifts */
    },

    {
      "R8",
      1,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 1,
      8, 0, 0, 0,
      0x000000FF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG8",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 2,
      8, 8, 0, 0,
      0x000000FF, 0x0000FF00, 0, 0,
      0, 8, 0, 0,
    },

    {
      "RGB8",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 3,
      8, 8, 8, 0,
      0x000000FF, 0x0000FF00, 0x00FF0000, 0,
      0, 8, 16, 0,
    },

    {
      "BGR8",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 3,
      8, 8, 8, 0,
      0x00FF0000, 0x0000FF00, 0x000000FF, 0,
      16, 8, 0, 0,
    },

    {}, //Deleted format
    {}, //Deleted format

    {
      "BGRA8",
      4,
      PIXEL_FORMAT_FLAGS::kHASALPHA |
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      8, 8, 8, 8,
      0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
      16, 8, 0, 24,
    },

    {
      "RGBA8",
      4,
      PIXEL_FORMAT_FLAGS::kHASALPHA |
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      8, 8, 8, 8,
      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
      0, 8, 16, 24,
    },
  
    {}, // Deleted format
    {}, // Deleted format
    {}, // Deleted format
    {}, // Deleted format

    {
      "BC1",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 3, //No alpha
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },
    
    {
      "BC1a",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED,
      PIXEL_COMPONENT_TYPE::kBYTE, 3,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },
    
    {
      "BC2",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },
    
    {
      "BC3",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },
    
    {
      "BC4",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED,
      PIXEL_COMPONENT_TYPE::kBYTE, 1,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },
    
    {
      "BC5",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED,
      PIXEL_COMPONENT_TYPE::kBYTE, 2,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "BC6H",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED,
      PIXEL_COMPONENT_TYPE::kFLOAT16, 3,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "BC7",
      0,
      PIXEL_FORMAT_FLAGS::kCOMPRESSED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "R16F",
      2,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT16, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG16F",
      4,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT16, 2,
      16, 16, 0, 0,
      0x0000FFFF, 0xFFFF0000, 0, 0,
      0, 16, 0, 0,
    },

    {}, //Deleted format

    {
      "RGBA16F",
      8,
      PIXEL_FORMAT_FLAGS::kFLOAT |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kFLOAT16, 4,
      16, 16, 16, 16,
      0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000,
      0, 16, 0, 16,
    },

    {
      "R32F",
      4,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 1,
      32, 0, 0, 0,
      0xFFFFFFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG32F",
      8,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 2,
      32, 32, 0, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RGB32F",
      12,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 3,
      32, 32, 32, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0,
      0, 0, 0, 0,
    },

    {
      "RGBA32F",
      16,
      PIXEL_FORMAT_FLAGS::kFLOAT |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 4,
      32, 32, 32, 32,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
      0, 0, 0, 0,
    },

    {
      "D32_S8X24",
      8,
      PIXEL_FORMAT_FLAGS::kDEPTH |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 2,
      32, 8, 0, 0,
      0xFFFFFFFF, 0x000000FF, 0x00000000, 0x00000000,
      0, 0, 0, 0,
    },

    {
      "D24_S8",
      4,
      PIXEL_FORMAT_FLAGS::kDEPTH |
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kINT, 2,
      24, 8, 0, 0,
      0x00FFFFFF, 0x0FF0000, 0x00000000, 0x00000000,
      0, 24, 0, 0,
    },

    {
      "D32",
      4,
      PIXEL_FORMAT_FLAGS::kDEPTH |
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kFLOAT32, 1,
      32, 0, 0, 0,
      0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
      0, 0, 0, 0,
    },

    {
      "D16",
      2,
      PIXEL_FORMAT_FLAGS::kDEPTH |
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kSHORT, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0x00000000, 0x00000000, 0x00000000,
      0, 0, 0, 0,
    },

    {
      "RG11B10F",
      4,
      PIXEL_FORMAT_FLAGS::kFLOAT,
      PIXEL_COMPONENT_TYPE::kPACKED_R11G11B10, 1,
      11, 11, 10, 0,
      0x000007FF, 0x003FF800, 0xFFC00000, 0,
      0, 11, 22, 0,
    },

    {
      "RGB10A2",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kPACKED_R10G10B10A2, 1,
      10, 10, 10, 2,
      0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000,
      0, 10, 20, 30,
    },

    {
      "R8I",
      1,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kBYTE, 1,
      8, 0, 0, 0,
      0x000000FF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG8I",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kBYTE, 2,
      8, 8, 0, 0,
      0x000000FF, 0x0000FF00, 0, 0,
      0, 8, 0, 0,
    },

    {
      "RGBA8I",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      8, 8, 8, 8,
      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
      0, 8, 16, 24,
    },

    {
      "R8U",
      1,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kBYTE, 1,
      8, 0, 0, 0,
      0x000000FF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG8U",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kBYTE, 2,
      8, 8, 0, 0,
      0x000000FF, 0x0000FF00, 0, 0,
      0, 8, 0, 0,
    },

    {
      "RGBA8U",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      8, 8, 8, 8,
      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
      0, 8, 16, 24,
    },

    {
      "R8S",
      1,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kBYTE, 1,
      8, 0, 0, 0,
      0x000000FF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG8S",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kBYTE, 2,
      8, 8, 0, 0,
      0x000000FF, 0x0000FF00, 0, 0,
      0, 8, 0, 0,
    },

    {
      "RGBA8S",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kBYTE, 4,
      8, 8, 8, 8,
      0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000,
      0, 8, 16, 24,
    },
    {
      "R16I",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kSHORT, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG16I",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kSHORT, 2,
      16, 16, 0, 0,
      0x0000FFFF, 0xFFFF0000, 0, 0,
      0, 16, 0, 0,
    },

    {
      "RGBA16I",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kSHORT, 4,
      16, 16, 16, 16,
      0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000,
      0, 16, 0, 16,
    },

    {
      "R16U",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kSHORT, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG16U",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kSHORT, 2,
      16, 16, 0, 0,
      0x0000FFFF, 0xFFFF0000, 0, 0,
      0, 16, 0, 0,
    },

    {
      "RGBA16U",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kSHORT, 4,
      16, 16, 16, 16,
      0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000,
      0, 16, 0, 16,
    },

    {
      "R32I",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kINT, 1,
      32, 0, 0, 0,
      0xFFFFFFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG32I",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kINT, 2,
      32, 32, 0, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RGB32I",
      12,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kINT, 3,
      32, 32, 32, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0,
      0, 0, 0, 0,
    },

    {
      "RGBA32I",
      16,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kSIGNED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kINT, 4,
      32, 32, 32, 32,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
      0, 0, 0, 0
    },

    {
      "R32U",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kINT, 1,
      32, 0, 0, 0,
      0xFFFFFFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG32U",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kINT, 2,
      32, 32, 0, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RGB32U",
      12,
      PIXEL_FORMAT_FLAGS::kINTEGER,
      PIXEL_COMPONENT_TYPE::kINT, 3,
      32, 32, 32, 0,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0,
      0, 0, 0, 0,
    },

    { "RGBA32U",
      16,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kINT, 4,
      32, 32, 32, 32,
      0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
      0, 0, 0, 0
    },

    {
      "R16S",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kSHORT, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0, 0, 0,
      0, 0, 0, 0,
    },

    {
      "RG16S",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED,
      PIXEL_COMPONENT_TYPE::kSHORT, 2,
      16, 16, 0, 0,
      0x0000FFFF, 0xFFFF0000, 0, 0,
      0, 16, 0, 0,
    },

    {
      "RGBA16S",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kSIGNED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kSHORT, 4,
      16, 16, 16, 16,
      0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000,
      0, 16, 0, 16,
    },

    {
      "R16",
      2,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kSHORT, 1,
      16, 0, 0, 0,
      0x0000FFFF, 0, 0, 0,
      0, 0, 0, 0
    },

    {
      "RG16",
      4,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED,
      PIXEL_COMPONENT_TYPE::kSHORT, 2,
      16, 16, 0, 0,
      0x0000FFFF, 0xFFFF0000, 0, 0,
      0, 16, 0, 0
    },

    {
      "RGBA16",
      8,
      PIXEL_FORMAT_FLAGS::kINTEGER |
      PIXEL_FORMAT_FLAGS::kNORMALIZED |
      PIXEL_FORMAT_FLAGS::kHASALPHA,
      PIXEL_COMPONENT_TYPE::kSHORT, 4,
      16, 16, 16, 16,
      0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000,
      0, 16, 0, 16
    },
  };

#if GE_COMPILER == GE_COMPILER_MSVC
# pragma endregion Pixel_Formats
#endif

  static inline const PixelFormatDescription&
  getDescriptionFor(const PixelFormat fmt) {
    const int ord = (int)fmt;
    GE_ASSERT(0 <= ord && ord < PixelFormat::kCOUNT);
    return _pixelFormats[ord];
  }

  /**
   * @brief Handles compression output from NVTT library for a single image.
   */
  struct NVTTCompressOutputHandler : public nvtt::OutputHandler
  {
    NVTTCompressOutputHandler(uint8* buffer, uint32 sizeBytes)
      : buffer(buffer),
        bufferWritePos(buffer),
        bufferEnd(buffer + sizeBytes)
    {}

    void
    beginImage(int /*size*/,
               int /*width*/,
               int /*height*/,
               int /*depth*/,
               int /*face*/,
               int /*miplevel*/) override
    {}

    bool
    writeData(const void* data, int size) override {
      GE_ASSERT((bufferWritePos + size) <= bufferEnd);
      memcpy(bufferWritePos, data, size);
      bufferWritePos += size;
      return true;
    }

    void
    endImage() override {}

    uint8* buffer;
    uint8* bufferWritePos;
    uint8* bufferEnd;
  };

  /**
   * @brief Handles output from NVTT library for a mip-map chain.
   */
  struct NVTTMipmapOutputHandler : public nvtt::OutputHandler
  {
    NVTTMipmapOutputHandler(const Vector<SPtr<PixelData>>& buffers)
      : buffers(buffers),
        bufferWritePos(nullptr),
        bufferEnd(nullptr)
    {}

    void
    beginImage(int size,
               int /*width*/,
               int /*height*/,
               int /*depth*/,
               int /*face*/,
               int miplevel) override {
      GE_UNREFERENCED_PARAMETER(size);
      GE_ASSERT(0 <= miplevel && static_cast<int>(buffers.size()) > miplevel);
      GE_ASSERT(static_cast<uint32>(size) == buffers[miplevel]->getConsecutiveSize());

      activeBuffer = buffers[miplevel];

      bufferWritePos = activeBuffer->getData();
      bufferEnd = bufferWritePos + activeBuffer->getConsecutiveSize();
    }

    bool
    writeData(const void* data, int size) override {
      GE_ASSERT((bufferWritePos + size) <= bufferEnd);
      memcpy(bufferWritePos, data, size);
      bufferWritePos += size;
      return true;
    }

    void
    endImage() override {}

    Vector<SPtr<PixelData>> buffers;
    SPtr<PixelData> activeBuffer;
    uint8* bufferWritePos;
    uint8* bufferEnd;
  };

  nvtt::Format
  toNVTTFormat(PixelFormat format) {
    switch (format)
    {
      case PixelFormat::kBC1:
        return nvtt::Format_BC1;
      case PixelFormat::kBC1a:
        return nvtt::Format_BC1a;
      case PixelFormat::kBC2:
        return nvtt::Format_BC2;
      case PixelFormat::kBC3:
        return nvtt::Format_BC3;
      case PixelFormat::kBC4:
        return nvtt::Format_BC4;
      case PixelFormat::kBC5:
        return nvtt::Format_BC5;
      case PixelFormat::kBC6H:
        return nvtt::Format_BC6;
      case PixelFormat::kBC7:
        return nvtt::Format_BC7;
    }

    //Unsupported format
    return nvtt::Format_BC3;
  }

  nvtt::Quality
  toNVTTQuality(COMPRESSION_QUALITY::E quality) {
    switch (quality)
    {
      case COMPRESSION_QUALITY::kFastest:
        return nvtt::Quality_Fastest;
      case COMPRESSION_QUALITY::kHighest:
        return nvtt::Quality_Highest;
      case COMPRESSION_QUALITY::kNormal:
        return nvtt::Quality_Normal;
      case COMPRESSION_QUALITY::kProduction:
        return nvtt::Quality_Normal;
    }

    //Unknown quality level
    return nvtt::Quality_Normal;
  }

  nvtt::AlphaMode
  toNVTTAlphaMode(ALPHA_MODE::E alphaMode) {
    switch (alphaMode)
    {
      case ALPHA_MODE::kNone:
        return nvtt::AlphaMode_None;
      case ALPHA_MODE::kPremultiplied:
        return nvtt::AlphaMode_Premultiplied;
      case ALPHA_MODE::kTransparency:
        return nvtt::AlphaMode_Transparency;
    }

    //Unknown alpha mode
    return nvtt::AlphaMode_None;
  }

  nvtt::WrapMode
  toNVTTWrapMode(MIPMAP_WRAP_MODE::E wrapMode) {
    switch (wrapMode)
    {
      case MIPMAP_WRAP_MODE::kClamp:
        return nvtt::WrapMode_Clamp;
      case MIPMAP_WRAP_MODE::kMirror:
        return nvtt::WrapMode_Mirror;
      case MIPMAP_WRAP_MODE::kRepeat:
        return nvtt::WrapMode_Repeat;
    }

    //Unknown alpha mode
    return nvtt::WrapMode_Mirror;
  }

  uint32
  PixelUtil::getNumElemBytes(PixelFormat format) {
    return static_cast<uint32>(getDescriptionFor(format).elemBytes);
  }

  uint32
  PixelUtil::getMemorySize(uint32 width,
                           uint32 height,
                           uint32 depth,
                           PixelFormat format) {
    if (isCompressed(format)) {
      switch (format)
      {
        //BC formats work by dividing the image into 4x4 blocks, then encoding
        //each 4x4 block with a certain number of bytes.
        case PixelFormat::kBC1:
        case PixelFormat::kBC1a:
        case PixelFormat::kBC4:
          return ((width + 3) / 4) * ((height + 3) / 4) * 8 * depth;
        case PixelFormat::kBC2:
        case PixelFormat::kBC3:
        case PixelFormat::kBC5:
        case PixelFormat::kBC6H:
        case PixelFormat::kBC7:
          return ((width + 3) / 4) * ((height + 3) / 4) * 16 * depth;

        default:
          GE_EXCEPT(InvalidParametersException,
                    "Invalid compressed pixel format");
          //return 0; //Unreachable code
      }
    }

    return width * height * depth * getNumElemBytes(format);
  }

  void
  PixelUtil::getPitch(uint32 width,
                      uint32 height,
                      uint32 /*depth*/,
                      PixelFormat format,
                      uint32& rowPitch,
                      uint32& depthPitch) {
    if (isCompressed(format)) {
      switch (format)
      {
        //BC formats work by dividing the image into 4x4 blocks, then encoding
        //each 4x4 block with a certain number of bytes.
        case PixelFormat::kBC1:
        case PixelFormat::kBC1a:
        case PixelFormat::kBC4:
        case PixelFormat::kBC2:
        case PixelFormat::kBC3:
        case PixelFormat::kBC5:
        case PixelFormat::kBC6H:
        case PixelFormat::kBC7:
          rowPitch = div(width + 3, 4).quot * 4;
          depthPitch = div(height + 3, 4).quot * 4 * rowPitch;
          return;

        default:
          GE_EXCEPT(InvalidParametersException, "Invalid compressed pixel format");
          return;
      }
    }

    rowPitch = width;
    depthPitch = width * height;
  }

  void
  PixelUtil::getSizeForMipLevel(uint32 width,
                                uint32 height,
                                uint32 depth,
                                uint32 mipLevel,
                                uint32& mipWidth,
                                uint32& mipHeight,
                                uint32& mipDepth) {
    mipWidth = width;
    mipHeight = height;
    mipDepth = depth;

    for (uint32 i = 0; i < mipLevel; ++i) {
      if (1 != mipWidth) {
        mipWidth = mipWidth >> 1;
      }
      if (1 != mipHeight) {
        mipHeight = mipHeight >> 1;
      }
      if (1 != mipDepth) {
        mipDepth = mipDepth >> 1;
      }
    }
  }

  uint32
  PixelUtil::getNumElemBits(PixelFormat format) {
    return static_cast<uint32>(getDescriptionFor(format).elemBytes * 8);
  }

  uint32
  PixelUtil::getFlags(PixelFormat format) {
    return getDescriptionFor(format).flags;
  }

  bool
  PixelUtil::hasAlpha(PixelFormat format) {
    return (PixelUtil::getFlags(format) & PIXEL_FORMAT_FLAGS::kHASALPHA) > 0;
  }

  bool
  PixelUtil::isFloatingPoint(PixelFormat format) {
    return (PixelUtil::getFlags(format) & PIXEL_FORMAT_FLAGS::kFLOAT) > 0;
  }

  bool
  PixelUtil::isCompressed(PixelFormat format) {
    return (PixelUtil::getFlags(format) & PIXEL_FORMAT_FLAGS::kCOMPRESSED) > 0;
  }

  bool
  PixelUtil::isNormalized(PixelFormat format) {
    return (PixelUtil::getFlags(format) & PIXEL_FORMAT_FLAGS::kNORMALIZED) > 0;
  }

  bool
  PixelUtil::isDepth(PixelFormat format) {
    return (PixelUtil::getFlags(format) & PIXEL_FORMAT_FLAGS::kDEPTH) > 0;
  }

  bool
  PixelUtil::checkFormat(PixelFormat& format,
                         TextureType texType,
                         int32 usage) {
    //First check just the usage since it's the most limiting factor

    //Depth-stencil only supports depth formats
    if ((usage & TU_DEPTHSTENCIL) != 0) {
      if (isDepth(format)) {
        return true;
      }

      format = PixelFormat::kD32_S8X24;
      return false;
    }

    //Render targets support everything but compressed & depth-stencil formats
    if ((usage & TU_RENDERTARGET) != 0) {
      if (!isDepth(format) && !isCompressed(format)) {
        return true;
      }

      format = PixelFormat::kRGBA8;
      return false;
    }

    //Load-store textures support everything but compressed & depth-stencil
    //formats
    if ((usage & TU_LOADSTORE) != 0) {
      if (!isDepth(format) && !isCompressed(format)) {
        return true;
      }

      format = PixelFormat::kRGBA8;
      return false;
    }

    //Sampled texture support depends on texture type
    switch (texType)
    {
    case TEX_TYPE_1D:
      {
        //1D textures support anything but depth & compressed formats
        if (!isDepth(format) && !isCompressed(format)) {
          return true;
        }
        format = PixelFormat::kRGBA8;
        return false;
      }
    case TEX_TYPE_3D:
      {
        // 3D textures support anything but depth & compressed formats
        if (!isDepth(format))
          return true;
        format = PixelFormat::kRGBA8;
        return false;
      }
    default:  //2D & cube
      {
        //2D/cube textures support anything but depth formats
        if (!isDepth(format)) {
          return true;
        }
        format = PixelFormat::kRGBA8;
        return false;
      }
    }
  }

  bool
  PixelUtil::isValidExtent(uint32 width,
                           uint32 height,
                           uint32 depth,
                           PixelFormat format) {
    if (isCompressed(format)) {
      switch (format)
      {
        case PixelFormat::kBC1:
        case PixelFormat::kBC2:
        case PixelFormat::kBC1a:
        case PixelFormat::kBC3:
        case PixelFormat::kBC4:
        case PixelFormat::kBC5:
        case PixelFormat::kBC6H:
        case PixelFormat::kBC7:
          return ((width & 3) == 0 && (height & 3) == 0 && depth == 1);
        default:
          return true;
      }
    }

    return true;
  }

  void
  PixelUtil::getBitDepths(PixelFormat format, int32(&rgba)[4]) {
    const PixelFormatDescription& des = getDescriptionFor(format);
    rgba[0] = des.rbits;
    rgba[1] = des.gbits;
    rgba[2] = des.bbits;
    rgba[3] = des.abits;
  }

  void
  PixelUtil::getBitMasks(PixelFormat format, uint32(&rgba)[4]) {
    const PixelFormatDescription& des = getDescriptionFor(format);
    rgba[0] = des.rmask;
    rgba[1] = des.gmask;
    rgba[2] = des.bmask;
    rgba[3] = des.amask;
  }

  void
  PixelUtil::getBitShifts(PixelFormat format, uint8(&rgba)[4]) {
    const PixelFormatDescription& des = getDescriptionFor(format);
    rgba[0] = des.rshift;
    rgba[1] = des.gshift;
    rgba[2] = des.bshift;
    rgba[3] = des.ashift;
  }

  String
  PixelUtil::getFormatName(PixelFormat srcformat) {
    return getDescriptionFor(srcformat).name;
  }

  bool
  PixelUtil::isAccessible(PixelFormat srcformat) {
    if (PixelFormat::kUNKNOWN == srcformat) {
      return false;
    }

    uint32 flags = getFlags(srcformat);
    return !( (flags & PIXEL_FORMAT_FLAGS::kCOMPRESSED) ||
              (flags & PIXEL_FORMAT_FLAGS::kDEPTH) );
  }

  PIXEL_COMPONENT_TYPE::E
  PixelUtil::getElementType(PixelFormat format) {
    const PixelFormatDescription& des = getDescriptionFor(format);
    return des.componentType;
  }

  uint32
  PixelUtil::getNumElements(PixelFormat format) {
    const PixelFormatDescription& des = getDescriptionFor(format);
    return static_cast<uint32>(des.componentCount);
  }

  uint32
  PixelUtil::getMaxMipmaps(uint32 width,
                           uint32 height,
                           uint32 depth,
                           PixelFormat /*format*/) {
    uint32 count = 0;
    if ((width > 0) && (height > 0)) {
      while (!(1 == width && 1 == height && 1 == depth)) {
        if (width > 1) {
          width = width >> 1;
        }
        if (height > 1) {
          height = height >> 1;
        }
        if (depth > 1) {
          depth = depth >> 1;
        }
        ++count;
      }
    }

    return count;
  }

  void
  PixelUtil::packColor(const LinearColor& color,
                       PixelFormat format,
                       void* dest) {
    packColor(color.r, color.g, color.b, color.a, format, dest);
  }

  void
  PixelUtil::packColor(uint8 r,
                       uint8 g,
                       uint8 b,
                       uint8 a,
                       PixelFormat format,
                       void* dest) {
    const PixelFormatDescription &des = getDescriptionFor(format);

    if (des.flags & PIXEL_FORMAT_FLAGS::kINTEGER) {
      //Shortcut for integer formats packing
      uint32 value = ((Bitwise::fixedToFixed(r, 8, des.rbits) << des.rshift) & des.rmask) |
                     ((Bitwise::fixedToFixed(g, 8, des.gbits) << des.gshift) & des.gmask) |
                     ((Bitwise::fixedToFixed(b, 8, des.bbits) << des.bshift) & des.bmask) |
                     ((Bitwise::fixedToFixed(a, 8, des.abits) << des.ashift) & des.amask);

      //And write to memory
      Bitwise::intWrite(dest,
                        static_cast<int32>(des.elemBytes),
                        value);
    }

    //Convert to float
    packColor(static_cast<float>(r) / 255.0f,
              static_cast<float>(g) / 255.0f,
              static_cast<float>(b) / 255.0f,
              static_cast<float>(a) / 255.0f,
              format,
              dest);
  }

  void
  PixelUtil::packColor(float r,
                       float g,
                       float b,
                       float a,
                       const PixelFormat format,
                       void* dest) {
    //Special cases
    if (PixelFormat::kRG11B10F == format) {
      uint32 value;
      value =  Bitwise::floatToFloat11(r);
      value |= Bitwise::floatToFloat11(g) << 11;
      value |= Bitwise::floatToFloat10(b) << 22;

      reinterpret_cast<uint32*>(dest)[0] = value;
      return;
    }

    if (PixelFormat::kRGB10A2 == format) {
      LOGERR("packColor() not implemented for format \"" +
             getFormatName(PixelFormat::kRGB10A2) + "\".");
      return;
    }

    //All other formats handled in a generic way
    const PixelFormatDescription& des = getDescriptionFor(format);
    GE_ASSERT(4 >= des.componentCount);

    float inputs[] = { r, g, b, a };
    uint8 bits[] = { des.rbits, des.gbits, des.bbits, des.abits };
    uint32 masks[] = { des.rmask, des.gmask, des.bmask, des.amask };
    uint8 shifts[] = { des.rshift, des.gshift, des.bshift, des.ashift };

    memset(dest, 0, des.elemBytes);

    uint32 curBit = 0;
    uint32 prevDword = 0;
    uint32 dwordValue = 0;
    for (uint32 i = 0; i < des.componentCount; ++i) {
      uint32 curDword = curBit >> 5;  //Division by 32

      //New dword reached, write current one and advance
      if (curDword > prevDword) {
        uint32* curDst = reinterpret_cast<uint32*>(dest) + prevDword;
        Bitwise::intWrite(curDst, 4, dwordValue);

        dwordValue = 0;
        prevDword = curDword;
      }

      if (des.flags & PIXEL_FORMAT_FLAGS::kINTEGER) {
        if (des.flags & PIXEL_FORMAT_FLAGS::kNORMALIZED) {
          if (des.flags & PIXEL_FORMAT_FLAGS::kSIGNED) {
            dwordValue |= (Bitwise::snormToUint(inputs[i], bits[i]) << shifts[i]) & masks[i];
          }
          else {
            dwordValue |= (Bitwise::unormToUint(inputs[i], bits[i]) << shifts[i]) & masks[i];
          }
        }
        else {
          //NOTE: Casting integer to float. A better option would be to have a
          //separate unpackColor that has integer output parameters.
          dwordValue |= (static_cast<uint32>(inputs[i]) << shifts[i]) & masks[i];
        }
      }
      else if (des.flags & PIXEL_FORMAT_FLAGS::kFLOAT) {
        //NOTE: Not handling unsigned floats
        if (des.componentType == PIXEL_COMPONENT_TYPE::kFLOAT16) {
          dwordValue |= (Bitwise::floatToHalf(inputs[i]) << shifts[i]) & masks[i];
        }
        else {
          dwordValue |= *reinterpret_cast<uint32*>(&inputs[i]);
        }
      }
      else {
        LOGERR("packColor() not implemented for format \"" +
               getFormatName(format) + "\".");
        return;
      }

      curBit += bits[i];
    }

    //Write last dword
    uint32 numBytes = min((prevDword + 1) * 4,
                          static_cast<uint32>(des.elemBytes)) - (prevDword * 4);
    uint32* curDst = reinterpret_cast<uint32*>(dest) + prevDword;
    Bitwise::intWrite(curDst, numBytes, dwordValue);
  }

  void
  PixelUtil::unpackColor(LinearColor* color,
                         PixelFormat format,
                         const void* src) {
    unpackColor(&color->r, &color->g, &color->b, &color->a, format, src);
  }

  void
  PixelUtil::unpackColor(uint8* r,
                         uint8* g,
                         uint8* b,
                         uint8* a,
                         PixelFormat format,
                         const void* src) {
    const PixelFormatDescription &des = getDescriptionFor(format);

    if (des.flags & PIXEL_FORMAT_FLAGS::kINTEGER) {
      //Shortcut for integer formats unpacking
      const uint32 value = Bitwise::intRead(src, des.elemBytes);

      *r = static_cast<uint8>(Bitwise::fixedToFixed((value & des.rmask) >> des.rshift,
                                                    des.rbits,
                                                    8));
      *g = static_cast<uint8>(Bitwise::fixedToFixed((value & des.gmask) >> des.gshift,
                                                    des.gbits,
                                                    8));
      *b = static_cast<uint8>(Bitwise::fixedToFixed((value & des.bmask) >> des.bshift,
                                                    des.bbits,
                                                    8));

      if (des.flags & PIXEL_FORMAT_FLAGS::kHASALPHA) {
        *a = static_cast<uint8>(Bitwise::fixedToFixed((value & des.amask) >> des.ashift,
                                                      des.abits,
                                                      8));
      }
      else {
        *a = 255; //No alpha, default a component to full
      }
    }
    else {
      //Do the operation with the more generic floating point
      float rr, gg, bb, aa;
      unpackColor(&rr, &gg, &bb, &aa, format, src);
      *r = static_cast<uint8>(Bitwise::unormToUint(rr, 8));
      *g = static_cast<uint8>(Bitwise::unormToUint(gg, 8));
      *b = static_cast<uint8>(Bitwise::unormToUint(bb, 8));
      *a = static_cast<uint8>(Bitwise::unormToUint(aa, 8));
    }
  }

  void
  PixelUtil::unpackColor(float* r,
                         float* g,
                         float* b,
                         float* a,
                         PixelFormat format,
                         const void* src) {
    //Special cases
    if (PixelFormat::kRG11B10F == format) {
      uint32 value = reinterpret_cast<uint32*>(const_cast<void*>(src))[0];
      *r = Bitwise::float11ToFloat(value);
      *g = Bitwise::float11ToFloat(value >> 11);
      *b = Bitwise::float10ToFloat(value >> 22);
      return;
    }

    if (PixelFormat::kRGB10A2 == format) {
      LOGERR("unpackColor() not implemented for format \"" +
             getFormatName(PixelFormat::kRGB10A2) + "\".");
      return;
    }

    //All other formats handled in a generic way
    const PixelFormatDescription& des = getDescriptionFor(format);
    GE_ASSERT(4 >= des.componentCount);

    float* outputs[] = { r, g, b, a };
    uint8 bits[] = { des.rbits, des.gbits, des.bbits, des.abits };
    uint32 masks[] = { des.rmask, des.gmask, des.bmask, des.amask };
    uint8 shifts[] = { des.rshift, des.gshift, des.bshift, des.ashift };

    uint32 curBit = 0;
    for (uint32 i = 0; i < des.componentCount; ++i) {
      uint32 curDword = curBit >> 5;  //Division by 32
      uint32 numBytes = min((curDword + 1) * 4,
                            static_cast<uint32>(des.elemBytes)) - (curDword * 4);

      uint32* curSrc = reinterpret_cast<uint32*>(const_cast<void*>(src)) + curDword;
      uint32 value = Bitwise::intRead(curSrc, numBytes);
      if (des.flags & PIXEL_FORMAT_FLAGS::kINTEGER) {
        if (des.flags & PIXEL_FORMAT_FLAGS::kNORMALIZED) {
          if (des.flags & PIXEL_FORMAT_FLAGS::kSIGNED) {
            *outputs[i] = Bitwise::uintToSnorm((value & masks[i]) >> shifts[i], bits[i]);
          }
          else {
            *outputs[i] = Bitwise::uintToUnorm((value & masks[i]) >> shifts[i], bits[i]);
          }
        }
        else {
          //NOTE: Casting integer to float. A better option would be to have a
          //separate unpackColor that has integer output parameters.
          *outputs[i] = static_cast<float>((value & masks[i]) >> shifts[i]);
        }
      }
      else if (des.flags & PIXEL_FORMAT_FLAGS::kFLOAT) {
        //NOTE: Not handling unsigned floats
        if (des.componentType == PIXEL_COMPONENT_TYPE::kFLOAT16) {
          *outputs[i] =
            Bitwise::halfToFloat(static_cast<uint16>((value & masks[i]) >> shifts[i]));
        }
        else {
          *outputs[i] = *reinterpret_cast<float*>(&value);
        }
      }
      else {
        LOGERR("unpackColor() not implemented for format \"" +
               getFormatName(format) + "\".");
        return;
      }

      curBit += bits[i];
    }

    //Fill empty components
    for (uint32 i = des.componentCount; i < 3; ++i) {
      *outputs[i] = 0.0f;
    }

    if (4 > des.componentCount) {
      *outputs[3] = 1.0f;
    }
  }

  void
  PixelUtil::packDepth(float /*depth*/,
                       const PixelFormat format,
                       void* /*dest*/) {
    if (!isDepth(format)) {
      LOGERR("Cannot convert depth to " +
             getFormatName(format) + ": it is not a depth format");
      return;
    }

    LOGERR("Method is not implemented");
    //TODO: Implement depth packing
  }

  float
  PixelUtil::unpackDepth(PixelFormat format, void* src) {
    if (!isDepth(format)) {
      LOGERR("Cannot unpack from " +
             getFormatName(format) + ": it is not a depth format");
      return 0;
    }

    uint32* color = reinterpret_cast<uint32*>(src);
    uint32 masked = 0;
    switch (format)
    {
      case PixelFormat::kD24S8:
        return  static_cast<float>(*color & 0x00FFFFFF) / 16777216.f;
        break;
      case PixelFormat::kD16:
        return static_cast<float>(*color & 0xFFFF) / 65536.f;
        break;
      case PixelFormat::kD32:
        masked = *color & 0xFFFFFFFF;
        return *reinterpret_cast<float*>(&masked);
        break;
      case PixelFormat::kD32_S8X24:
        masked = *color & 0xFFFFFFFF;
        return *reinterpret_cast<float*>(&masked);
        break;
      default:
        LOGERR("Cannot unpack from " + getFormatName(format));
        break;
    }

    return 0;
  }

  void
  PixelUtil::bulkPixelConversion(const PixelData &src, PixelData &dst) {
    GE_ASSERT(src.getWidth() == dst.getWidth() &&
              src.getHeight() == dst.getHeight() &&
              src.getDepth() == dst.getDepth());

    //Check for compressed formats, we don't support decompression
    if (PixelUtil::isCompressed(src.getFormat())) {
      if (src.getFormat() == dst.getFormat()) {
        memcpy(dst.getData(), src.getData(), src.getConsecutiveSize());
        return;
      }
      else {
        LOGERR("bulkPixelConversion() cannot be used to compress or "
               "decompress images");
        return;
      }
    }

    //Check for compression
    if (PixelUtil::isCompressed(dst.getFormat())) {
      if (src.getFormat() == dst.getFormat()) {
        memcpy(dst.getData(), src.getData(), src.getConsecutiveSize());
        return;
      }
      else {
        CompressionOptions co;
        co.format = dst.getFormat();
        compress(src, dst, co);
        return;
      }
    }

    //The easy case
    if (src.getFormat() == dst.getFormat()) {
      //Everything consecutive?
      if (src.isConsecutive() && dst.isConsecutive()) {
        memcpy(dst.getData(), src.getData(), src.getConsecutiveSize());
        return;
      }

      const uint32 srcPixelSize = PixelUtil::getNumElemBytes(src.getFormat());
      const uint32 dstPixelSize = PixelUtil::getNumElemBytes(dst.getFormat());
      uint8* srcptr = src.getData() + (src.getLeft() + src.getTop() * src.getRowPitch() +
                                       src.getFront() * src.getSlicePitch()) * srcPixelSize;
      uint8* dstptr = dst.getData() + (dst.getLeft() + dst.getTop() * dst.getRowPitch() +
                                       dst.getFront() * dst.getSlicePitch()) * dstPixelSize;

      //Calculate pitches + skips in bytes
      const uint32 srcRowPitchBytes = src.getRowPitch() * srcPixelSize;
      const uint32 srcSliceSkipBytes = src.getSliceSkip() * srcPixelSize;

      const uint32 dstRowPitchBytes = dst.getRowPitch() * dstPixelSize;
      const uint32 dstSliceSkipBytes = dst.getSliceSkip() * dstPixelSize;

      //Otherwise, copy per row
      const uint32 rowSize = src.getWidth() * srcPixelSize;
      for (uint32 z = src.getFront(); z < src.getBack(); ++z) {
        for (uint32 y = src.getTop(); y < src.getBottom(); ++y) {
          memcpy(dstptr, srcptr, rowSize);
          srcptr += srcRowPitchBytes;
          dstptr += dstRowPitchBytes;
        }
        srcptr += srcSliceSkipBytes;
        dstptr += dstSliceSkipBytes;
      }
      return;
    }

    uint32 srcPixelSize = PixelUtil::getNumElemBytes(src.getFormat());
    uint32 dstPixelSize = PixelUtil::getNumElemBytes(dst.getFormat());
    uint8 *srcptr = src.getData() + (src.getLeft() + src.getTop() * src.getRowPitch() +
                                     src.getFront() * src.getSlicePitch()) * srcPixelSize;
    uint8 *dstptr = dst.getData() + (dst.getLeft() + dst.getTop() * dst.getRowPitch() +
                                     dst.getFront() * dst.getSlicePitch()) * dstPixelSize;

    //Calculate pitches + skips in bytes
    uint32 srcRowSkipBytes = src.getRowSkip() * srcPixelSize;
    uint32 srcSliceSkipBytes = src.getSliceSkip() * srcPixelSize;
    uint32 dstRowSkipBytes = dst.getRowSkip() * dstPixelSize;
    uint32 dstSliceSkipBytes = dst.getSliceSkip() * dstPixelSize;

    //The brute force fallback
    float r, g, b, a;
    for (uint32 z = src.getFront(); z < src.getBack(); ++z) {
      for (uint32 y = src.getTop(); y < src.getBottom(); ++y) {
        for (uint32 x = src.getLeft(); x < src.getRight(); ++x) {
          unpackColor(&r, &g, &b, &a, src.getFormat(), srcptr);
          packColor(r, g, b, a, dst.getFormat(), dstptr);
          srcptr += srcPixelSize;
          dstptr += dstPixelSize;
        }
        srcptr += srcRowSkipBytes;
        dstptr += dstRowSkipBytes;
      }
      srcptr += srcSliceSkipBytes;
      dstptr += dstSliceSkipBytes;
    }
  }

  void
  PixelUtil::flipComponentOrder(PixelData& data) {
    if (isCompressed(data.getFormat())) {
      LOGERR("flipComponentOrder() not supported on compressed images.");
      return;
    }

    const PixelFormatDescription& pfd = getDescriptionFor(data.getFormat());
    if (4 < pfd.elemBytes) {
      LOGERR("flipComponentOrder() only supported on 4 byte or smaller pixel "
             "formats.");
      return;
    }

    if (1 >= pfd.componentCount) {  //Nothing to flip
      return;
    }

    bool bitCountMismatch = false;
    if (pfd.rbits != pfd.gbits) {
      bitCountMismatch = true;
    }

    if (pfd.componentCount > 2 && pfd.rbits != pfd.bbits) {
      bitCountMismatch = true;
    }

    if (pfd.componentCount > 3 && pfd.rbits != pfd.abits) {
      bitCountMismatch = true;
    }

    if (bitCountMismatch) {
      LOGERR("flipComponentOrder() not supported for formats that don't have "
             "the same number of bytes for all components.");
      return;
    }

    struct CompData
    {
      uint32 mask;
      uint8 shift;
    };

    array<CompData, 4> compData = {
      {
        { pfd.rmask, pfd.rshift },
        { pfd.gmask, pfd.gshift },
        { pfd.bmask, pfd.bshift },
        { pfd.amask, pfd.ashift }
      }
    };

    //Ensure unused components are at the end, after sort
    if (4 > pfd.componentCount) {
      compData[3].shift = 0xFF;
    }

    if (3 > pfd.componentCount) {
      compData[2].shift = 0xFF;
    }

    sort(compData.begin(), compData.end(),
    [&](const CompData& lhs, const CompData& rhs) {
      return lhs.shift < rhs.shift;
    });

    uint8* dataPtr = data.getData();

    uint32 pixelSize = pfd.elemBytes;
    uint32 rowSkipBytes = data.getRowSkip() * pixelSize;
    uint32 sliceSkipBytes = data.getSliceSkip() * pixelSize;

    for (uint32 z = 0; z < data.getDepth(); ++z) {
      for (uint32 y = 0; y < data.getHeight(); ++y) {
        for (uint32 x = 0; x < data.getWidth(); ++x) {
          if (2 == pfd.componentCount) {
            uint64 pixelData = 0;
            memcpy(&pixelData, dataPtr, pixelSize);

            uint64 output = 0;
            output |= (pixelData & compData[1].mask) >> compData[1].shift;
            output |= (pixelData & compData[0].mask) << compData[1].shift;

            memcpy(dataPtr, &output, pixelSize);
          }
          else if (3 == pfd.componentCount) {
            uint64 pixelData = 0;
            memcpy(&pixelData, dataPtr, pixelSize);

            uint64 output = 0;
            output |= (pixelData & compData[2].mask) >> compData[2].shift;
            output |= (pixelData & compData[0].mask) << compData[2].shift;

            memcpy(dataPtr, &output, pixelSize);
          }
          else if (4 == pfd.componentCount) {
            uint64 pixelData = 0;
            memcpy(&pixelData, dataPtr, pixelSize);

            uint64 output = 0;
            output |= (pixelData & compData[3].mask) >> compData[3].shift;
            output |= (pixelData & compData[0].mask) << compData[3].shift;

            output |= (pixelData & compData[2].mask) >>
                      (compData[2].shift - compData[1].shift);
            output |= (pixelData & compData[1].mask) <<
                      (compData[2].shift - compData[1].shift);

            memcpy(dataPtr, &output, pixelSize);
          }
          dataPtr += pixelSize;
        }
        dataPtr += rowSkipBytes;
      }
      dataPtr += sliceSkipBytes;
    }
  }

  void
  PixelUtil::scale(const PixelData& src, PixelData& scaled, FILTER::E filter) {
    GE_ASSERT(PixelUtil::isAccessible(src.getFormat()));
    GE_ASSERT(PixelUtil::isAccessible(scaled.getFormat()));

    PixelData temp;
    switch (filter)
    {
      default:
      case FILTER::kNEAREST:
        if (src.getFormat() == scaled.getFormat()) {
          //No intermediate buffer needed
          temp = scaled;
        }
        else {
          //Allocate temporary buffer of destination size in source format
          temp = PixelData(scaled.getWidth(),
                           scaled.getHeight(),
                           scaled.getDepth(),
                           src.getFormat());
          temp.allocateInternalBuffer();
        }

        //No conversion
        switch (PixelUtil::getNumElemBytes(src.getFormat()))
        {
          case 1: NearestResampler<1>::scale(src, temp); break;
          case 2: NearestResampler<2>::scale(src, temp); break;
          case 3: NearestResampler<3>::scale(src, temp); break;
          case 4: NearestResampler<4>::scale(src, temp); break;
          case 6: NearestResampler<6>::scale(src, temp); break;
          case 8: NearestResampler<8>::scale(src, temp); break;
          case 12: NearestResampler<12>::scale(src, temp); break;
          case 16: NearestResampler<16>::scale(src, temp); break;
          default:
            //Never reached
            GE_ASSERT(false);
        }

        if (temp.getData() != scaled.getData()) {
          //Blit temp buffer
          PixelUtil::bulkPixelConversion(temp, scaled);
          temp.freeInternalBuffer();
        }
        break;

      case FILTER::kLINEAR:
        switch (src.getFormat())
        {
          case PixelFormat::kRG8:
          case PixelFormat::kRGB8: case PixelFormat::kBGR8:
          case PixelFormat::kRGBA8: case PixelFormat::kBGRA8:
            if (src.getFormat() == scaled.getFormat()) {
              //No intermediate buffer needed
              temp = scaled;
            }
            else {
              //Allocate temp buffer of destination size in source format
              temp = PixelData(scaled.getWidth(),
                               scaled.getHeight(),
                               scaled.getDepth(),
                               src.getFormat());
              temp.allocateInternalBuffer();
            }

            //No conversion
            switch (PixelUtil::getNumElemBytes(src.getFormat()))
            {
              case 1: LinearResampler_Byte<1>::scale(src, temp); break;
              case 2: LinearResampler_Byte<2>::scale(src, temp); break;
              case 3: LinearResampler_Byte<3>::scale(src, temp); break;
              case 4: LinearResampler_Byte<4>::scale(src, temp); break;
              default:
                //Never reached
                GE_ASSERT(false);
            }

            if (temp.getData() != scaled.getData()) {
              //Blit temp buffer
              PixelUtil::bulkPixelConversion(temp, scaled);
              temp.freeInternalBuffer();
            }

            break;
          case PixelFormat::kRGB32F:
          case PixelFormat::kRGBA32F:
            if (scaled.getFormat() == PixelFormat::kRGB32F ||
                scaled.getFormat() == PixelFormat::kRGBA32F) {
              //float32 to float32, avoid unpack/repack overhead
              LinearResampler_Float32::scale(src, scaled);
              break;
            }
            //Else, fall through
          default:
            //Fallback case, slow but works
            LinearResampler::scale(src, scaled);
        }
        break;
    }
  }

  void
  PixelUtil::copy(const PixelData& src,
                  PixelData& dst,
                  uint32 offsetX,
                  uint32 offsetY,
                  uint32 offsetZ) {
    if (src.getFormat() != dst.getFormat()) {
      LOGERR("Source format is different from destination format for copy(). "
             "This operation cannot be used for a format conversion. "
             "Aborting copy.");
      return;
    }

    uint32 right = offsetX + dst.getWidth();
    uint32 bottom = offsetY + dst.getHeight();
    uint32 back = offsetZ + dst.getDepth();

    if (right > src.getWidth() || bottom > src.getHeight() || back > src.getDepth()) {
      LOGERR("Provided offset or destination size is too large and is "
             "referencing pixels that are out of bounds on the source texture."
             " Aborting copy().");
      return;
    }

    uint8* srcPtr = src.getData() + offsetZ * src.getSlicePitch();
    uint8* dstPtr = dst.getData();

    uint32 elemSize = getNumElemBytes(dst.getFormat());
    uint32 rowSize = dst.getWidth() * elemSize;

    for (uint32 z = 0; z < dst.getDepth(); ++z) {
      uint8* srcRowPtr = srcPtr + offsetY * src.getRowPitch() * elemSize;
      uint8* dstRowPtr = dstPtr;

      for (uint32 y = 0; y < dst.getHeight(); ++y) {
        memcpy(dstRowPtr, srcRowPtr + offsetX * elemSize, rowSize);
        srcRowPtr += src.getRowPitch() * elemSize;
        dstRowPtr += dst.getRowPitch() * elemSize;
      }

      srcPtr += src.getSlicePitch() * elemSize;
      dstPtr += dst.getSlicePitch() * elemSize;
    }
  }

  void
  PixelUtil::mirror(PixelData& pixelData, MirrorMode mode) {
    uint32 width = pixelData.getWidth();
    uint32 height = pixelData.getHeight();
    uint32 depth = pixelData.getDepth();
    uint32 elemSize = getNumElemBytes(pixelData.getFormat());

    if (mode.isSet(MIRROR_MODE_BITS::kZ)) {
      uint32 sliceSize = width * height * elemSize;
      auto sliceTemp = ge_stack_alloc<uint8>(sliceSize);

      uint8* dataPtr = pixelData.getData();
      uint32 halfDepth = depth / 2;
      for (uint32 z = 0; z < halfDepth; ++z) {
        uint32 srcZ = z * sliceSize;
        uint32 dstZ = (depth - z - 1) * sliceSize;

        memcpy(sliceTemp, &dataPtr[dstZ], sliceSize);
        memcpy(&dataPtr[dstZ], &dataPtr[srcZ], sliceSize);
        memcpy(&dataPtr[srcZ], sliceTemp, sliceSize);
      }

      //NOTE: If flipping Y or X as well I could do it here without an extra
      //set of memcpys
      ge_stack_free(sliceTemp);
    }

    if (mode.isSet(MIRROR_MODE_BITS::kY)) {
      uint32 rowSize = width * elemSize;
      auto rowTemp = ge_stack_alloc<uint8>(rowSize);

      uint8* slicePtr = pixelData.getData();
      for (uint32 z = 0; z < depth; ++z) {
        uint32 halfHeight = height >> 1;
        for (uint32 y = 0; y < halfHeight; ++y) {
          uint32 srcY = y * rowSize;
          uint32 dstY = (height - y - 1) * rowSize;

          memcpy(rowTemp, &slicePtr[dstY], rowSize);
          memcpy(&slicePtr[dstY], &slicePtr[srcY], rowSize);
          memcpy(&slicePtr[srcY], rowTemp, rowSize);
        }

        //NOTE: If flipping X as well I could do it here without an extra set
        //of memcpys
        slicePtr += pixelData.getSlicePitch() * elemSize;
      }

      ge_stack_free(rowTemp);
    }

    if (mode.isSet(MIRROR_MODE_BITS::kX)) {
      auto elemTemp = ge_stack_alloc<uint8>(elemSize);

      uint8* slicePtr = pixelData.getData();
      for (uint32 z = 0; z < depth; ++z) {
        uint8* rowPtr = slicePtr;
        for (uint32 y = 0; y < height; ++y) {
          uint32 halfWidth = width >> 1;
          for (uint32 x = 0; x < halfWidth; ++x) {
            uint32 srcX = x * elemSize;
            uint32 dstX = (width - x - 1) * elemSize;

            memcpy(elemTemp, &rowPtr[dstX], elemSize);
            memcpy(&rowPtr[dstX], &rowPtr[srcX], elemSize);
            memcpy(&rowPtr[srcX], elemTemp, elemSize);
          }
          rowPtr += pixelData.getRowPitch() * elemSize;
        }
        slicePtr += pixelData.getSlicePitch() * elemSize;
      }
      ge_stack_free(elemTemp);
    }
  }

  void
  PixelUtil::applyGamma(uint8* buffer, float gamma, uint32 size, uint8 bpp) {
    if (1.0f == gamma) {
      return;
    }

    uint32 stride = bpp >> 3;

    float r, g, b;

    for (SIZE_T i = 0, j = size / stride; i < j; ++i, buffer += stride) {
      r = static_cast<float>(buffer[0]);
      g = static_cast<float>(buffer[1]);
      b = static_cast<float>(buffer[2]);

      r *= gamma;
      g *= gamma;
      b *= gamma;

      float gammaScale = 1.0f;
      float tmp = 0.0f;

      if (r > 255.0f && (tmp = (255.0f / r)) < gammaScale) {
        gammaScale = tmp;
      }

      if (g > 255.0f && (tmp = (255.0f / g)) < gammaScale) {
        gammaScale = tmp;
      }

      if (b > 255.0f && (tmp = (255.0f / b)) < gammaScale) {
        gammaScale = tmp;
      }

      r *= gammaScale;
      g *= gammaScale;
      b *= gammaScale;

      buffer[0] = static_cast<uint8>(r);
      buffer[1] = static_cast<uint8>(g);
      buffer[2] = static_cast<uint8>(b);
    }
  }

  void
  PixelUtil::compress(const PixelData& src,
                      PixelData& dst,
                      const CompressionOptions& options) {
    if (!isCompressed(options.format)) {
      LOGERR("Compression failed. Destination format is not a valid "
             "compressed format.");
        return;
    }

    if (src.getDepth() != 1) {
      LOGERR("Compression failed. 3D texture compression not supported.");
      return;
    }

    if (isCompressed(src.getFormat()))
    {
      LOGERR("Compression failed. Source data cannot be compressed.");
      return;
    }

    PixelFormat interimFormat = options.format == PixelFormat::kBC6H ?
      PixelFormat::kRGBA32F :
      PixelFormat::kBGRA8;

    PixelData interimData(src.getWidth(), src.getHeight(), 1, interimFormat);
    interimData.allocateInternalBuffer();
    bulkPixelConversion(src, interimData);

    nvtt::InputOptions io;
    io.setTextureLayout(nvtt::TextureType_2D, src.getWidth(), src.getHeight());
    io.setMipmapGeneration(false);
    io.setAlphaMode(toNVTTAlphaMode(options.alphaMode));
    io.setNormalMap(options.isNormalMap);

    if (PixelFormat::kRGBA32F == interimFormat) {
      io.setFormat(nvtt::InputFormat_RGBA_32F);
    }
    else {
      io.setFormat(nvtt::InputFormat_BGRA_8UB);
    }

    if (options.isSRGB) {
      io.setGamma(2.2f, 2.2f);
    }
    else {
      io.setGamma(1.0f, 1.0f);
    }

    io.setMipmapData(interimData.getData(), src.getWidth(), src.getHeight());

    nvtt::CompressionOptions co;
    co.setFormat(toNVTTFormat(options.format));
    co.setQuality(toNVTTQuality(options.quality));

    NVTTCompressOutputHandler outputHandler(dst.getData(), dst.getConsecutiveSize());

    nvtt::OutputOptions oo;
    oo.setOutputHeader(false);
    oo.setOutputHandler(&outputHandler);

    nvtt::Compressor compressor;
    if (!compressor.process(io, co, oo)) {
      LOGERR("Compression failed. Internal error.");
      return;
    }
  }

  Vector<SPtr<PixelData>>
  PixelUtil::genMipmaps(const PixelData& src, const MipMapGenOptions& options) {
    Vector<SPtr<PixelData>> outputMipBuffers;

    if (src.getDepth() != 1) {
      LOGERR("Mipmap generation failed. 3D texture formats not supported.");
      return outputMipBuffers;
    }

    if (isCompressed(src.getFormat())) {
      LOGERR("Mipmap generation failed. Source data cannot be compressed.");
      return outputMipBuffers;
    }

    if (!Bitwise::isPow2(src.getWidth()) ||
        !Bitwise::isPow2(src.getHeight())) {
      LOGERR("Mipmap generation failed. "
             "Texture width & height must be powers of 2.");
      return outputMipBuffers;
    }

    PixelFormat interimFormat = isFloatingPoint(src.getFormat()) ?
                                                  PixelFormat::kRGBA32F :
                                                  PixelFormat::kBGRA8;

    PixelData interimData(src.getWidth(), src.getHeight(), 1, interimFormat);
    interimData.allocateInternalBuffer();
    bulkPixelConversion(src, interimData);

    if (PixelFormat::kRGBA32F != interimFormat) {
      flipComponentOrder(interimData);
    }

    nvtt::InputOptions io;
    io.setTextureLayout(nvtt::TextureType_2D, src.getWidth(), src.getHeight());
    io.setMipmapGeneration(true);
    io.setNormalMap(options.isNormalMap);
    io.setNormalizeMipmaps(options.normalizeMipmaps);
    io.setWrapMode(toNVTTWrapMode(options.wrapMode));

    if (PixelFormat::kRGBA32F == interimFormat) {
      io.setFormat(nvtt::InputFormat_RGBA_32F);
    }
    else {
      io.setFormat(nvtt::InputFormat_BGRA_8UB);
    }

    if (options.isSRGB) {
      io.setGamma(2.2f, 2.2f);
    }
    else {
      io.setGamma(1.0f, 1.0f);
    }

    io.setMipmapData(interimData.getData(), src.getWidth(), src.getHeight());

    nvtt::CompressionOptions co;
    co.setFormat(nvtt::Format_RGBA);

    if (PixelFormat::kRGBA32F == interimFormat) {
      co.setPixelType(nvtt::PixelType_Float);
      co.setPixelFormat(32, 32, 32, 32);
    }
    else {
      co.setPixelType(nvtt::PixelType_UnsignedNorm);
      co.setPixelFormat(32, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF);
    }

    uint32 numMips = getMaxMipmaps(src.getWidth(),
                                   src.getHeight(),
                                   1,
                                   src.getFormat());

    Vector<SPtr<PixelData>> rgbaMipBuffers;

    //NOTE: This can be done more effectively without creating so many temp
    //buffers and working with the original formats directly, but it would
    //complicate the code too much at the moment.
    uint32 curWidth = src.getWidth();
    uint32 curHeight = src.getHeight();
    for (uint32 i = 0; i < numMips; ++i) {
      rgbaMipBuffers.push_back(ge_shared_ptr_new<PixelData>(curWidth,
                                                            curHeight,
                                                            1,
                                                            interimFormat));
      rgbaMipBuffers.back()->allocateInternalBuffer();

      if (curWidth > 1) {
        curWidth = curWidth >> 1;
      }

      if (curHeight > 1) {
        curHeight = curHeight >> 1;
      }
    }

    rgbaMipBuffers.push_back(ge_shared_ptr_new<PixelData>(curWidth,
                                                          curHeight,
                                                          1,
                                                          interimFormat));
    rgbaMipBuffers.back()->allocateInternalBuffer();

    NVTTMipmapOutputHandler outputHandler(rgbaMipBuffers);

    nvtt::OutputOptions oo;
    oo.setOutputHeader(false);
    oo.setOutputHandler(&outputHandler);

    nvtt::Compressor compressor;
    if (!compressor.process(io, co, oo)) {
      LOGERR("Mipmap generation failed. Internal error.");
      return outputMipBuffers;
    }

    interimData.freeInternalBuffer();

    for (const auto& argbBuffer : rgbaMipBuffers) {
      SPtr<PixelData>
        outputBuffer = ge_shared_ptr_new<PixelData>(argbBuffer->getWidth(),
                                                    argbBuffer->getHeight(),
                                                    1,
                                                    src.getFormat());
      outputBuffer->allocateInternalBuffer();

      bulkPixelConversion(*argbBuffer, *outputBuffer);
      argbBuffer->freeInternalBuffer();

      outputMipBuffers.push_back(outputBuffer);
    }

    return outputMipBuffers;
  }
}
