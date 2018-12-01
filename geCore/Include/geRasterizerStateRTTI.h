/*****************************************************************************/
/**
 * @file    geRasterizerStateRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/09
 * @brief   RTTI Objects for geRasterizerState.
 *
 * RTTI Objects for geRasterizerState.
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
#include "geRasterizerState.h"
#include "geRenderStateManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT RasterizerStateRTTI
    : public RTTIType<RasterizerState, IReflectable, RasterizerStateRTTI>
  {
   private:
    RASTERIZER_STATE_DESC&
    getData(RasterizerState* obj) {
      return obj->m_properties.m_data;
    }

    void
    setData(RasterizerState* obj, RASTERIZER_STATE_DESC& val) {
      obj->m_properties.m_data = val;
    }

   public:
    RasterizerStateRTTI() {
      addPlainField("m_data",
                    0,
                    &RasterizerStateRTTI::getData,
                    &RasterizerStateRTTI::setData);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto rasterizerState = static_cast<RasterizerState*>(obj);
      rasterizerState->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "RasterizerState";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_RasterizerState;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return RenderStateManager::instance().
               _createRasterizerStatePtr(RASTERIZER_STATE_DESC());
    }
  };
}
