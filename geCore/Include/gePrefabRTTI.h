/*****************************************************************************/
/**
 * @file    gePrefabRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/04
 * @brief   RTTI Objects for gePrefab.
 *
 * RTTI Objects for gePrefab.
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
#include "gePrefab.h"
#include "geSceneObject.h"
#include "geUtility.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT PrefabRTTI
    : public RTTIType<Prefab, Resource, PrefabRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(m_hash, 1)
      GE_RTTI_MEMBER_PLAIN(m_uuid, 2)
      GE_RTTI_MEMBER_PLAIN(m_isScene, 3)
    GE_END_RTTI_MEMBERS

    SPtr<SceneObject>
    getSceneObject(Prefab* obj) {
      return obj->m_root.getInternalPtr();
    }

    void
    setSceneObject(Prefab* obj, SPtr<SceneObject> value) {
      obj->m_root = value->getHandle();
    }

   public:
    PrefabRTTI() {
      addReflectablePtrField("m_root",
                             0,
                             &PrefabRTTI::getSceneObject,
                             &PrefabRTTI::setSceneObject);
    }

    void
    onDeserializationStarted(IReflectable* /*ptr*/,
                             SerializationContext* context) override {
      GE_ASSERT(nullptr != context &&
                rtti_is_of_type<CoreSerializationContext>(context));
      auto coreContext = static_cast<CoreSerializationContext*>(context);

      //Make sure external IDs are broken because we do some ID matching when
      //dealing with prefabs and keeping the invalid external references could
      //cause it to match invalid objects in case they end up having the same
      //ID.
      GE_ASSERT(!coreContext->goState);
      coreContext->goState = ge_shared_ptr_new<GameObjectDeserializationState>
                               (GODM::kUseNewIds | GODM::kBreakExternal);
    }

    const String&
    getRTTIName() override {
      static String name = "Prefab";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_Prefab;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return Prefab::createEmpty();
    }
  };
}
