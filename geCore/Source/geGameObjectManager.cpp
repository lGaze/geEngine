/*****************************************************************************/
/**
 * @file    geGameObjectManager.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   Tracks GameObject creation and destructions.
 *
 * Tracks GameObject creation and destructions. Also resolves GameObject
 * references from GameObject handles.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geGameObjectManager.h"
#include "geGameObject.h"

namespace geEngineSDK {
  using std::memory_order_relaxed;

  GameObjectManager::~GameObjectManager() {
    destroyQueuedObjects();
  }

  GameObjectHandleBase
  GameObjectManager::getObject(uint64 id) const {
    Lock lock(m_mutex);
    
    const auto iterFind = m_objects.find(id);

    if (m_objects.end() != iterFind) {
      return iterFind->second;
    }

    return nullptr;
  }

  bool
  GameObjectManager::tryGetObject(uint64 id, GameObjectHandleBase& object) const {
    Lock lock(m_mutex);

    const auto iterFind = m_objects.find(id);

    if (m_objects.end() != iterFind) {
      object = iterFind->second;
      return true;
    }

    return false;
  }

  bool
  GameObjectManager::objectExists(uint64 id) const {
    Lock lock(m_mutex);
    return m_objects.find(id) != m_objects.end();
  }

  void
  GameObjectManager::remapId(uint64 oldId, uint64 newId) {
    if (oldId == newId) {
      return;
    }

    Lock lock(m_mutex);
    m_objects[newId] = m_objects[oldId];
    m_objects.erase(oldId);
  }

  uint64
  GameObjectManager::reserveId() {
    return m_nextAvailableID.fetch_add(1, memory_order_relaxed);
  }

  void
  GameObjectManager::queueForDestroy(const GameObjectHandleBase& object) {
    if (object.isDestroyed()) {
      return;
    }

    uint64 instanceId = object->getInstanceId();
    m_queuedForDestroy[instanceId] = object;
  }

  void
  GameObjectManager::destroyQueuedObjects() {
    for (auto& objPair : m_queuedForDestroy) {
      objPair.second->destroyInternal(objPair.second, true);
    }

    m_queuedForDestroy.clear();
  }

  GameObjectHandleBase
  GameObjectManager::registerObject(const SPtr<GameObject>& object) {
    const uint64 id = m_nextAvailableID.fetch_add(1, memory_order_relaxed);
    object->initialize(object, id);

    GameObjectHandleBase handle(object);
    {
      Lock lock(m_mutex);
      m_objects[id] = handle;
    }

    return handle;
  }

  void
  GameObjectManager::unregisterObject(GameObjectHandleBase& object) {
    {
      Lock lock(m_mutex);
      m_objects.erase(object->getInstanceId());
    }

    onDestroyed(static_object_cast<GameObject>(object));
    object.destroy();
  }

  GameObjectDeserializationState::~GameObjectDeserializationState() {
    GE_ASSERT(m_unresolvedHandles.empty() &&
              "Deserialization state being destroyed before all handles are "
              "resolved.");
    GE_ASSERT(m_deserializedObjects.empty() &&
              "Deserialization state being destroyed before all objects are "
              "resolved.");
  }

  void
  GameObjectDeserializationState::resolve() {
    for (auto& entry : m_unresolvedHandles) {
      uint64 instanceId = entry.originalInstanceId;

      bool isInternalReference = false;

      const auto findIter = m_idMapping.find(instanceId);
      if (m_idMapping.end() != findIter) {
        if ((m_options & GODM::kUseNewIds) != 0) {
          instanceId = findIter->second;
        }
        isInternalReference = true;
      }

      if (isInternalReference) {
        const auto findIterObj = m_deserializedObjects.find(instanceId);

        if (m_deserializedObjects.end() != findIterObj) {
          entry.handle._resolve(findIterObj->second);
        }
        else {
          if ((m_options & GODM::kKeepMissing) == 0) {
            entry.handle._resolve(nullptr);
          }
        }
      }
      else if (!isInternalReference && (m_options & GODM::kRestoreExternal) != 0) {
        HGameObject obj;
        if (GameObjectManager::instance().tryGetObject(instanceId, obj)) {
          entry.handle._resolve(obj);
        }
        else {
          if ((m_options & GODM::kKeepMissing) == 0)
            entry.handle._resolve(nullptr);
        }
      }
      else {
        if ((m_options & GODM::kKeepMissing) == 0) {
          entry.handle._resolve(nullptr);
        }
      }
    }

    for (auto iter = m_endCallbacks.rbegin(); iter != m_endCallbacks.rend(); ++iter) {
      (*iter)();
    }

    m_idMapping.clear();
    m_unresolvedHandles.clear();
    m_endCallbacks.clear();
    m_unresolvedHandleData.clear();
    m_deserializedObjects.clear();
  }

  void
  GameObjectDeserializationState::registerUnresolvedHandle(uint64 originalId,
                                                           GameObjectHandleBase& object) {
    //All handles that are deserialized during a single
    //begin/endDeserialization session pointing to the same object must share
    //the same GameObjectHandleData as that makes certain operations in other
    //systems much simpler.
    //Therefore we store all the unresolved handles, and if a handle pointing
    //to the same object was already processed, or that object was already
    //created we replace the handle's internal GameObjectHandleData.

    //Update the provided handle to ensure all handles pointing to the same
    //object share the same handle data
    bool foundHandleData = false;

    //Search object that are currently being deserialized
    const auto iterFind = m_idMapping.find(originalId);
    if (m_idMapping.end() != iterFind) {
      const auto iterFind2 = m_deserializedObjects.find(iterFind->second);
      if (m_deserializedObjects.end() != iterFind2) {
        object.m_data = iterFind2->second.m_data;
        foundHandleData = true;
      }
    }

    //Search previously deserialized handles
    if (!foundHandleData) {
      auto iterFindUHD = m_unresolvedHandleData.find(originalId);
      if (m_unresolvedHandleData.end() != iterFindUHD) {
        object.m_data = iterFindUHD->second;
        foundHandleData = true;
      }
    }

    //If still not found, this is the first such handle so register its handle
    //data
    if (!foundHandleData) {
      m_unresolvedHandleData[originalId] = object.m_data;
    }
    m_unresolvedHandles.push_back({ originalId, object });
  }

  void
  GameObjectDeserializationState::registerObject(uint64 originalId,
                                                 GameObjectHandleBase& object) {
    GE_ASSERT(0 != originalId && "Invalid game object ID.");

    const auto iterFind = m_unresolvedHandleData.find(originalId);
    if (iterFind != m_unresolvedHandleData.end()) {
      SPtr<GameObject> ptr = object.getInternalPtr();

      object.m_data = iterFind->second;
      object._setHandleData(ptr);
    }

    const uint64 newId = object->getInstanceId();
    m_idMapping[originalId] = newId;
    m_deserializedObjects[newId] = object;
  }

  void
  GameObjectDeserializationState::registerOnDeserializationEndCallback(function<void()>
                                                                       callback) {

    m_endCallbacks.push_back(callback);
  }
}
