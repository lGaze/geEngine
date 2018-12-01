/*****************************************************************************/
/**
 * @file    geSamplerStateRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/09
 * @brief   RTTI Objects for geSamplerState.
 *
 * RTTI Objects for geSamplerState.
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
#include "geSamplerState.h"
#include "geRenderStateManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  GE_ALLOW_MEMCPY_SERIALIZATION(SAMPLER_STATE_DESC);

  class GE_CORE_EXPORT SamplerStateRTTI
    : public RTTIType<SamplerState, IReflectable, SamplerStateRTTI>
  {
   private:
    SAMPLER_STATE_DESC&
    getData(SamplerState* obj) {
      return obj->m_properties.m_data;
    }

    void
    setData(SamplerState* obj, SAMPLER_STATE_DESC& val) {
      obj->m_properties.m_data = val;
    }

   public:
    SamplerStateRTTI() {
      addPlainField("m_data",
                    0,
                    &SamplerStateRTTI::getData,
                    &SamplerStateRTTI::setData);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto samplerState = static_cast<SamplerState*>(obj);
      samplerState->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "SamplerState";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_SamplerState;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return RenderStateManager::instance()._createSamplerStatePtr(SAMPLER_STATE_DESC());
    }
  };
}
