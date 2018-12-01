/*****************************************************************************/
/**
 * @file    geDepthStencilStateRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/09
 * @brief   RTTI Objects for geDepthStencilState.
 *
 * RTTI Objects for geDepthStencilState.
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
#include "geDepthStencilState.h"
#include "geRenderStateManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT DepthStencilStateRTTI
    : public RTTIType<DepthStencilState, IReflectable, DepthStencilStateRTTI>
  {
   private:
    DEPTH_STENCIL_STATE_DESC&
    getData(DepthStencilState* obj) {
      return obj->m_properties.m_data;
    }

    void
    setData(DepthStencilState* obj, DEPTH_STENCIL_STATE_DESC& val) {
      obj->m_properties.m_data = val;
    }

   public:
    DepthStencilStateRTTI() {
      addPlainField("m_data",
                    0,
                    &DepthStencilStateRTTI::getData,
                    &DepthStencilStateRTTI::setData);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto depthStencilState = static_cast<DepthStencilState*>(obj);
      depthStencilState->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "DepthStencilState";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_DepthStencilState;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return RenderStateManager::instance().
               _createDepthStencilStatePtr(DEPTH_STENCIL_STATE_DESC());
    }
  };
}
