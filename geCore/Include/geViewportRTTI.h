/*****************************************************************************/
/**
 * @file    geViewportRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/12
 * @brief   RTTI Objects for geViewport.
 *
 * RTTI Objects for geViewport.
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
#include "geViewport.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT ViewportRTTI
    : public RTTIType<Viewport, IReflectable, ViewportRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(m_normArea, 0)
      GE_RTTI_MEMBER_PLAIN(m_clearColorValue, 1)
      GE_RTTI_MEMBER_PLAIN(m_clearDepthValue, 2)
      GE_RTTI_MEMBER_PLAIN(m_clearStencilValue, 3)
      GE_RTTI_MEMBER_PLAIN(m_clearFlags, 4)
    GE_END_RTTI_MEMBERS

   public:
    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto viewport = static_cast<Viewport*>(obj);
      viewport->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "Viewport";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_Viewport;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return Viewport::createEmpty();
    }
  };
}
