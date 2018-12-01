/*****************************************************************************/
/**
 * @file    geGameObjectRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   RTTI Objects for geGameObject.
 *
 * RTTI Objects for geGameObject.
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
#include "geGameObject.h"
#include "geSceneObject.h"
#include "geGameObjectManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  /**
   * @brief Provides temporary storage for data used during GameObject
   *        deserialization.
   */
  struct GODeserializationData
  {
    GODeserializationData()
      : isDeserializationParent(false),
        originalId(0)
    {}

    SPtr<GameObject> ptr;
    bool isDeserializationParent;
    uint64 originalId;
    Any moreData;
  };

  class GE_CORE_EXPORT GameObjectRTTI
    : public RTTIType<GameObject, IReflectable, GameObjectRTTI>
  {
   private:
    String&
    getName(GameObject* obj) {
      return obj->m_name;
    }

    void
    setName(GameObject* obj, String& name) {
      obj->m_name = name;
    }

    uint64&
    getInstanceID(GameObject* obj) {
      return obj->m_instanceData->instanceId;
    }

    void
    setInstanceID(GameObject* obj, uint64& instanceId) {
      //We record the ID for later use. Any child RTTI of GameObject must call
      //GameObjectManager::registerObject with this ID, so we know how to map
      //deserialized GO handles to live objects, otherwise the handle
      //references will get broken.
      auto go = static_cast<GameObject*>(obj);
      auto& deserializationData = any_cast_ref<GODeserializationData>(go->m_rttiData);

      deserializationData.originalId = instanceId;
    }

    uint32&
    getLinkId(GameObject* obj) {
      return obj->m_linkId;
    }

    void
    setLinkId(GameObject* obj, uint32& linkId) {
      obj->m_linkId = linkId;
    }

   public:
    /**
     * @brief Helper method used for creating Component objects used during
     *        deserialization.
     */
    template<typename T>
    static SPtr<T>
    createGameObject() {
      SPtr<T> component = SceneObject::createEmptyComponent<T>();
      component->m_rttiData = component;
      return component;
    }

   public:
    GameObjectRTTI() {
      addPlainField("m_instanceID",
                    0,
                    &GameObjectRTTI::getInstanceID,
                    &GameObjectRTTI::setInstanceID);
      addPlainField("m_name",
                    1,
                    &GameObjectRTTI::getName,
                    &GameObjectRTTI::setName);
      addPlainField("m_linkId",
                    2,
                    &GameObjectRTTI::getLinkId,
                    &GameObjectRTTI::setLinkId);
    }

    void
    onDeserializationStarted(IReflectable* obj,
                             SerializationContext* /*context*/) override {
      auto gameObject = static_cast<GameObject*>(obj);

      //It's possible we're just accessing the game object fields, in which
      //case the process below is not needed (it's only required for new game
      //objects).
      if (gameObject->m_rttiData.empty()) {
        return;
      }

      SPtr<GameObject> gameObjectPtr = any_cast<SPtr<GameObject>>(gameObject->m_rttiData);

      //Every GameObject must store GODeserializationData in its RTTI data
      //field during deserialization
      gameObject->m_rttiData = GODeserializationData();
      auto& deserializationData = any_cast_ref<GODeserializationData>(gameObject->m_rttiData);

      //Store shared pointer since the system only provides us with raw ones
      deserializationData.ptr = gameObjectPtr;
    }

    const String&
    getRTTIName() override {
      static String name = "GameObject";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_GameObject;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      GE_EXCEPT(InternalErrorException,
                "Cannot instantiate an abstract class.");
      //return nullptr; //Unreachable code
    }
  };
}
