/*****************************************************************************/
/**
 * @file    geGameObject.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   Type of object that can be referenced by a GameObject handle.
 *
 * Type of object that can be referenced by a GameObject handle. Each object
 * has an unique ID and is registered with the GameObjectManager.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geGameObject.h"
#include "geGameObjectRTTI.h"
#include "geGameObjectManager.h"

#include <geNumericLimits.h>

namespace geEngineSDK {
  void
  GameObject::initialize(const SPtr<GameObject>& object, uint64 instanceId) {
    m_instanceData = ge_shared_ptr_new<GameObjectInstanceData>();
    m_instanceData->object = object;
    m_instanceData->instanceId = instanceId;
  }

  void
  GameObject::_setInstanceData(GameObjectInstanceDataPtr& other) {
    SPtr<GameObject> myPtr = m_instanceData->object;
    uint64 oldId = m_instanceData->instanceId;

    m_instanceData = other;
    m_instanceData->object = myPtr;

    GameObjectManager::instance().remapId(oldId, m_instanceData->instanceId);
  }

  RTTITypeBase*
  GameObject::getRTTIStatic() {
    return GameObjectRTTI::instance();
  }

  RTTITypeBase*
  GameObject::getRTTI() const {
    return GameObject::getRTTIStatic();
  }
}
