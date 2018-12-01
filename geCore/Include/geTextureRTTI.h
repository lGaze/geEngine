/*****************************************************************************/
/**
 * @file    geTextureRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/19
 * @brief   RTTI Objects for geTexture.
 *
 * RTTI Objects for geTexture.
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
#include "geTexture.h"
#include "geCoreThread.h"
#include "geRenderAPI.h"
#include "geTextureManager.h"
#include "gePixelData.h"

#include <geRTTIType.h>
#include <geMath.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT TextureRTTI
    : public RTTIType<Texture, Resource, TextureRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(m_size, 0)
      GE_RTTI_MEMBER_PLAIN_NAMED(height, m_properties.m_desc.height, 1)
      GE_RTTI_MEMBER_PLAIN_NAMED(width, m_properties.m_desc.width, 2)
      GE_RTTI_MEMBER_PLAIN_NAMED(depth, m_properties.m_desc.depth, 3)
      GE_RTTI_MEMBER_PLAIN_NAMED(numMips, m_properties.m_desc.numMips, 4)
      GE_RTTI_MEMBER_PLAIN_NAMED(hwGamma, m_properties.m_desc.hwGamma, 5)
      GE_RTTI_MEMBER_PLAIN_NAMED(numSamples, m_properties.m_desc.numSamples, 6)
      GE_RTTI_MEMBER_PLAIN_NAMED(type, m_properties.m_desc.type, 7)
      GE_RTTI_MEMBER_PLAIN_NAMED(format, m_properties.m_desc.format, 8)
    GE_END_RTTI_MEMBERS

    int32&
    getUsage(Texture* obj) {
      return obj->m_properties.m_desc.usage;
    }

    void
    setUsage(Texture* obj, int32& val) {
      //Render target and depth stencil texture formats are for in-memory use
      //only and don't make sense when serialized
      if ((val & (TU_DEPTHSTENCIL | TU_RENDERTARGET)) != 0) {
        obj->m_properties.m_desc.usage &= ~(TU_DEPTHSTENCIL | TU_RENDERTARGET);
        obj->m_properties.m_desc.usage |= TU_STATIC;
      }
      else {
        obj->m_properties.m_desc.usage = val;
      }
    }

    SPtr<PixelData>
    getPixelData(Texture* obj, uint32 idx) {
      uint32 face = Math::floor(idx /
                      static_cast<float>(obj->m_properties.getNumMipmaps() + 1));
      uint32 mipmap = idx % (obj->m_properties.getNumMipmaps() + 1);
      SPtr<PixelData> pixelData = obj->m_properties.allocBuffer(face, mipmap);
      obj->readData(pixelData, face, mipmap);
      g_coreThread().submitAll(true);

      return pixelData;
    }

    void
    setPixelData(Texture* /*obj*/, uint32 idx, SPtr<PixelData> data) {
      m_pixelData[idx] = data;
    }

    uint32
    getPixelDataArraySize(Texture* obj) {
      return obj->m_properties.getNumFaces() *
             (obj->m_properties.getNumMipmaps() + 1);
    }

    void
    setPixelDataArraySize(Texture* /*obj*/, uint32 size) {
      m_pixelData.resize(size);
    }

   public:
    TextureRTTI() {
      addPlainField("m_usage", 9, &TextureRTTI::getUsage, &TextureRTTI::setUsage);
      addReflectablePtrArrayField("m_pixelData",
                                  10,
                                  &TextureRTTI::getPixelData,
                                  &TextureRTTI::getPixelDataArraySize,
                                  &TextureRTTI::setPixelData,
                                  &TextureRTTI::setPixelDataArraySize,
                                  RTTI_FIELD_FLAG::kSkipInReferenceSearch);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto texture = static_cast<Texture*>(obj);
      TextureProperties& texProps = texture->m_properties;

      //Update pixel format if needed as it's possible the original texture was
      //saved using some other render API that has an unsupported format.
      PixelFormat originalFormat = texProps.getFormat();
      PixelFormat validFormat =
        TextureManager::instance().getNativeFormat(texProps.getTextureType(),
                                                   texProps.getFormat(),
                                                   texProps.getUsage(),
                                                   texProps.isHardwareGammaEnabled());

      if (originalFormat != validFormat) {
        texProps.m_desc.format = validFormat;

        for (auto& i : m_pixelData) {
          SPtr<PixelData> newData = PixelData::create(i->getWidth(),
                                                      i->getHeight(),
                                                      i->getDepth(),
                                                      validFormat);

          PixelUtil::bulkPixelConversion(*i, *newData);
          i = newData;
        }
      }

      //A bit clumsy initializing with already set values, but I feel its
      //better than complicating things and storing the values in m_rttiData.
      texture->initialize();

      for (SIZE_T i = 0; i < m_pixelData.size(); ++i) {
        uint32 face = Math::floor(i / static_cast<float>(texProps.getNumMipmaps() + 1));
        uint32 mipmap = i % (texProps.getNumMipmaps() + 1);
        texture->writeData(m_pixelData[i], face, mipmap, false);
      }
    }

    const String&
    getRTTIName() override {
      static String name = "Texture";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_Texture;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return TextureManager::instance()._createEmpty();
    }

   private:
    Vector<SPtr<PixelData>> m_pixelData;
  };
}
