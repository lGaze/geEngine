/*****************************************************************************/
/**
 * @file    geBlendStateRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/02
 * @brief   RTTI Objects for geBlendState.
 *
 * RTTI Objects for geBlendState.
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
#include "geBlendState.h"
#include "geRenderStateManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT BlendStateRTTI
    : public RTTIType<BlendState, IReflectable, BlendStateRTTI>
  {
   private:
    BLEND_STATE_DESC&
    getData(BlendState* obj) {
      return obj->m_properties.m_data;
    }

    void
    setData(BlendState* obj, BLEND_STATE_DESC& val) {
      obj->m_properties.m_data = val;
    }

   public:
    BlendStateRTTI() {
      addPlainField("m_data",
                    0,
                    &BlendStateRTTI::getData,
                    &BlendStateRTTI::setData);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto blendState = static_cast<BlendState*>(obj);
      blendState->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "BlendState";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_BlendState;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return RenderStateManager::instance()._createBlendStatePtr(BLEND_STATE_DESC());
    }
  };
}
