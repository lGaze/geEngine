/*****************************************************************************/
/**
 * @file    geSceneObjectRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/29
 * @brief   RTTI Objects for geSceneObject.
 *
 * RTTI Objects for geSceneObject.
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
#include "geSceneObject.h"
#include "geGameObjectHandle.h"
#include "geGameObjectManager.h"
#include "geComponent.h"
#include "geGameObjectRTTI.h"
#include "gePrefabDiff.h"
#include "geUtility.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  using std::static_pointer_cast;

  class GE_CORE_EXPORT SceneObjectRTTI
    : public RTTIType<SceneObject, GameObject, SceneObjectRTTI>
  {
   private:
    Transform&
    getTransform(SceneObject* obj) {
      return obj->m_worldTransform;
    }

    void
    setTransform(SceneObject* obj, Transform& value) {
      obj->m_worldTransform = value;
    }

    Transform&
    getLocalTransform(SceneObject* obj) {
      return obj->m_localTransform;
    }

    void
    setLocalTransform(SceneObject* obj, Transform& value) {
      obj->m_localTransform = value;
    }

    bool&
    getActive(SceneObject* obj) {
      return obj->m_activeSelf;
    }

    void
    setActive(SceneObject* obj, bool& value) {
      obj->m_activeSelf = value;
    }

    SPtr<SceneObject>
    getChild(SceneObject* obj, uint32 idx) {
      return obj->m_children[idx].getInternalPtr();
    }

    void
    setChild(SceneObject* /*obj*/, uint32 idx, SPtr<SceneObject> param) {
      //It's important that child indices remain the same after deserialization
      //as some systems (like SO record / restore) depend on it
      if (m_children.size() <= idx) {
        m_children.resize(idx + 1);
      }

      m_children[idx] = param;
    }

    uint32
    getNumChildren(SceneObject* obj) {
      return static_cast<uint32>(obj->m_children.size());
    }

    void
    setNumChildren(SceneObject* /*obj*/, uint32 /*size*/) {}

    //NOTE: These can only be set sequentially, specific array index is ignored
    SPtr<Component>
    getComponent(SceneObject* obj, uint32 idx) {
      return obj->m_components[idx].getInternalPtr();
    }

    void
    setComponent(SceneObject* /*obj*/, uint32 idx, SPtr<Component> param) {
      //It's important that child indices remain the same after deserialization
      //as some systems (like SO record / restore) depend on it
      if (m_components.size() <= idx) {
        m_components.resize(idx + 1);
      }

      m_components[idx] = param;
    }

    uint32
    getNumComponents(SceneObject* obj) {
      return static_cast<uint32>(obj->m_components.size());
    }

    void
    setNumComponents(SceneObject* /*obj*/, uint32 /*size*/) {}

    UUID&
    getPrefabLink(SceneObject* obj) {
      return obj->m_prefabLinkUUID;
    }

    void
    setPrefabLink(SceneObject* obj, UUID& value) {
      obj->m_prefabLinkUUID = value;
    }

    SPtr<PrefabDiff>
    getPrefabDiff(SceneObject* obj) {
      return obj->m_prefabDiff;
    }

    void
    setPrefabDiff(SceneObject* obj, SPtr<PrefabDiff> value) {
      obj->m_prefabDiff = value;
    }

    uint32&
    getFlags(SceneObject* obj) {
      return obj->m_flags;
    }

    void
    setFlags(SceneObject* obj, uint32& value) {
      obj->m_flags = value;
    }

    uint32&
    getPrefabHash(SceneObject* obj) {
      return obj->m_prefabHash;
    }

    void
    setPrefabHash(SceneObject* obj, uint32& value) {
      obj->m_prefabHash = value;
    }

    ObjectMobility&
    getMobility(SceneObject* obj) {
      return obj->m_mobility;
    }

    void
    setMobility(SceneObject* obj, ObjectMobility& value) {
      obj->m_mobility = value;
    }

   public:
    SceneObjectRTTI() {
      addReflectablePtrArrayField("m_children",
                                  0,
                                  &SceneObjectRTTI::getChild,
                                  &SceneObjectRTTI::getNumChildren,
                                  &SceneObjectRTTI::setChild,
                                  &SceneObjectRTTI::setNumChildren);

      addReflectablePtrArrayField("m_components",
                                  1,
                                  &SceneObjectRTTI::getComponent,
                                  &SceneObjectRTTI::getNumComponents,
                                  &SceneObjectRTTI::setComponent,
                                  &SceneObjectRTTI::setNumComponents);

      addPlainField("m_prefabLink",
                    2,
                    &SceneObjectRTTI::getPrefabLink,
                    &SceneObjectRTTI::setPrefabLink);

      addPlainField("m_flags",
                    3,
                    &SceneObjectRTTI::getFlags,
                    &SceneObjectRTTI::setFlags);

      addReflectablePtrField("m_prefabDiff",
                             4,
                             &SceneObjectRTTI::getPrefabDiff,
                             &SceneObjectRTTI::setPrefabDiff);

      addPlainField("m_prefabHash",
                    5,
                    &SceneObjectRTTI::getPrefabHash,
                    &SceneObjectRTTI::setPrefabHash);

      addPlainField("m_activeSelf",
                    9,
                    &SceneObjectRTTI::getActive,
                    &SceneObjectRTTI::setActive);

      addPlainField("m_mobility",
                    10,
                    &SceneObjectRTTI::getMobility,
                    &SceneObjectRTTI::setMobility);

      addReflectableField("m_worldTransform",
                          11,
                          &SceneObjectRTTI::getTransform,
                          &SceneObjectRTTI::setTransform);

      addReflectableField("m_localTransform",
                          12,
                          &SceneObjectRTTI::getLocalTransform,
                          &SceneObjectRTTI::setLocalTransform);
    }

    void
    onDeserializationStarted(IReflectable* obj,
                             SerializationContext* context) override {
      //If this is the root scene object we're deserializing, activate game
      //object deserialization so the system can resolve deserialized handles
      //to the newly created objects
      auto so = static_cast<SceneObject*>(obj);

      //It's possible we're just accessing the game object fields, in which
      //case the process below is not needed (it's only required for new scene
      //objects).
      if (so->m_rttiData.empty()) {
        return;
      }

      if (nullptr == context ||
          !rtti_is_of_type<CoreSerializationContext>(context)) {
        return;
      }

      auto coreContext = static_cast<CoreSerializationContext*>(context);
      if (!coreContext->goDeserializationActive) {
        if (!coreContext->goState) {
          coreContext->goState = ge_shared_ptr_new<GameObjectDeserializationState>();
        }

        m_isDeserializationParent = true;
        coreContext->goDeserializationActive = true;
      }
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* context) override {
      auto so = static_cast<SceneObject*>(obj);

      //It's possible we're just accessing the game object fields, in which
      //case the process below is not needed (it's only required for new scene
      //objects).
      if (so->m_rttiData.empty()) {
        return;
      }

      GE_ASSERT(nullptr != context &&
                rtti_is_of_type<CoreSerializationContext>(context));
      auto coreContext = static_cast<CoreSerializationContext*>(context);

      auto& goDeserializationData = any_cast_ref<GODeserializationData>(so->m_rttiData);

      //Register the newly created SO with the GameObjectManager and provide it
      //with the original ID so that deserialized handles pointing to this
      //object can be resolved.
      SPtr<SceneObject> soPtr = static_pointer_cast<SceneObject>(goDeserializationData.ptr);
      
      HSceneObject soHandle = SceneObject::createInternal(soPtr);
      coreContext->goState->registerObject(goDeserializationData.originalId, soHandle);

      //We stored all components and children in a temporary structure because
      //they rely on the SceneObject being initialized with the
      //GameObjectManager. Now that it is, we add them.
      for (auto& component : m_components) {
        so->addComponentInternal(component);
      }

      for (auto& child : m_children) {
        if (nullptr != child) {
          child->_setParent(so->m_thisHandle, false);
        }
      }

      //If this is the deserialization parent, end deserialization (which
      //resolves all game object handles, if we provided valid IDs), and
      //instantiate (i.e. activate) the deserialized hierarchy.
      if (m_isDeserializationParent) {
        coreContext->goState->resolve();
        coreContext->goDeserializationActive = false;

        bool parentActive = true;
        if (nullptr != so->getParent()) {
          parentActive = so->getParent()->getActive();
        }

        so->setActiveHierarchy(parentActive, false);

        if ((so->m_flags & SCENE_OBJECT_FLAGS::kDontInstantiate) == 0) {
          so->_instantiate();
        }
      }

      so->m_rttiData = nullptr;
    }

    const String&
    getRTTIName() override {
      static String name = "SceneObject";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_SceneObject;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      auto sceneObject = SPtr<SceneObject>(new (ge_alloc<SceneObject>())
                                           SceneObject("",
                                                       SCENE_OBJECT_FLAGS::kDontInstantiate),
                                           &ge_delete<SceneObject>, StdAlloc<SceneObject>());
      sceneObject->m_rttiData = sceneObject;
      return sceneObject;
    }
    
   private:
    bool m_isDeserializationParent = false;
    Vector<SPtr<SceneObject>> m_children;
    Vector<SPtr<Component>> m_components;
  };
}
