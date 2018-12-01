/*****************************************************************************/
/**
 * @file    geResourceHandleRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/29
 * @brief   RTTI Objects for geResourceHandle.
 *
 * RTTI Objects for geResourceHandle.
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
#include "geResources.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  using std::memory_order_relaxed;

  class GE_CORE_EXPORT ResourceHandleRTTI
    : public RTTIType<TResourceHandleBase<false>, IReflectable, ResourceHandleRTTI>
  {
   private:
    UUID&
    getUUID(TResourceHandleBase<false>* obj) {
      return nullptr != obj->m_data ? obj->m_data->m_uuid : UUID::EMPTY;
    }

    void
    setUUID(TResourceHandleBase<false>* obj, UUID& uuid) {
      obj->m_data->m_uuid = uuid;
    }

   public:
    ResourceHandleRTTI() {
      addPlainField("m_uuid",
                    0,
                    &ResourceHandleRTTI::getUUID,
                    &ResourceHandleRTTI::setUUID);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto resourceHandle = static_cast<TResourceHandleBase<false>*>(obj);

      if (resourceHandle->m_data && !resourceHandle->m_data->m_uuid.empty()) {
        HResource loadedResource =
          g_resources()._getResourceHandle(resourceHandle->m_data->m_uuid);

        resourceHandle->releaseRef();
        resourceHandle->m_data = loadedResource.m_data;
        resourceHandle->addRef();
      }
    }

    const String&
    getRTTIName() override {
      static String name = "ResourceHandleBase";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_ResourceHandle;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      auto obj = ge_shared_ptr<TResourceHandleBase<false>>(
                   new (ge_alloc<TResourceHandleBase<false>>())
                     TResourceHandleBase<false>());
      obj->m_data = ge_shared_ptr_new<ResourceHandleData>();
      obj->m_data->m_refCount.fetch_add(1, memory_order_relaxed);

      return obj;
    }
  };

  class GE_CORE_EXPORT WeakResourceHandleRTTI
    : public RTTIType<TResourceHandleBase<true>, IReflectable, WeakResourceHandleRTTI>
  {
   private:
    UUID&
    getUUID(TResourceHandleBase<true>* obj) {
      return nullptr != obj->m_data ? obj->m_data->m_uuid : UUID::EMPTY;
    }

    void
    setUUID(TResourceHandleBase<true>* obj, UUID& uuid) {
      obj->m_data->m_uuid = uuid;
    }

   public:
    WeakResourceHandleRTTI() {
      addPlainField("m_uuid",
                    0,
                    &WeakResourceHandleRTTI::getUUID,
                    &WeakResourceHandleRTTI::setUUID);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto resourceHandle = static_cast<TResourceHandleBase<true>*>(obj);

      if (resourceHandle->m_data && !resourceHandle->m_data->m_uuid.empty()) {
        HResource loadedResource =
          g_resources()._getResourceHandle(resourceHandle->m_data->m_uuid);
        resourceHandle->m_data = loadedResource.m_data;
      }
    }

    const String&
    getRTTIName() override {
      static String name = "WeakResourceHandleBase";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_WeakResourceHandle;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      auto obj = ge_shared_ptr<TResourceHandleBase<true>>(
                   new (ge_alloc<TResourceHandleBase<true>>())
                     TResourceHandleBase<true>());
      obj->m_data = ge_shared_ptr_new<ResourceHandleData>();

      return obj;
    }
  };
}
