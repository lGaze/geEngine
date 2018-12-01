/*****************************************************************************/
/**
 * @file    geResourceRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/29
 * @brief   RTTI Objects for geResource.
 *
 * RTTI Objects for geResource.
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
#include "geResource.h"
#include "geResourceMetaData.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT ResourceRTTI
    : public RTTIType<Resource, IReflectable, ResourceRTTI>
  {
   private:
    uint32&
    getSize(Resource* obj) {
      return obj->m_size;
    }

    void
    setSize(Resource* obj, uint32& size) {
      obj->m_size = size;
    }

    SPtr<ResourceMetaData>
    getMetaData(Resource* obj) {
      return obj->m_metaData;
    }

    void
    setMetaData(Resource* obj, SPtr<ResourceMetaData> value) {
      obj->m_metaData = value;
    }

   public:
    ResourceRTTI() {
      addPlainField("m_size",
                    0,
                    &ResourceRTTI::getSize,
                    &ResourceRTTI::setSize);
      addReflectablePtrField("m_metaData",
                             1,
                             &ResourceRTTI::getMetaData,
                             &ResourceRTTI::setMetaData);
    }

    void
    onDeserializationStarted(IReflectable* obj,
                             SerializationContext* context) override {
      auto resource = static_cast<Resource*>(obj);
      if (context) {
        resource->m_keepSourceData = (context->flags &
                                      SERIALIZATION_FLAGS::kKeepResourceSourceData) != 0;
      }
      else {
        resource->m_keepSourceData = false;
      }
    }

    const String&
    getRTTIName() override {
      static String name = "Resource";
      return name;
    }

    uint32
    getRTTIId() override {
      return 100; //This is meant to be any standard resource (constant?)
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      GE_EXCEPT(InternalErrorException,
                "Cannot instantiate an abstract class.");
      //return nullptr; //Unreachable code
    }
  };
}
