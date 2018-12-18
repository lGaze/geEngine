/*****************************************************************************/
/**
 * @file    geGameObjectHandle.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   A handle that can point to various types of game objects.
 *
 * A handle that can point to various types of game objects. It primarily keeps
 * track if the object is still alive, so anything still referencing it doesn't
 * accidentally use it.
 *
 * @note This class exists because references between game objects should be
 *       quite loose. For example one game object should be able to reference
 *       another one without the other one knowing. But if that is the case I
 *       also need to handle the case when the other object we're referencing
 *       has been deleted, and that is the main purpose of this class.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geGameObjectHandle.h"
#include "geGameObject.h"
#include "geGameObjectHandleRTTI.h"
#include <geException.h>

namespace geEngineSDK {
  GameObjectHandleBase::GameObjectHandleBase(const SPtr<GameObject>& ptr) {
    m_data = ge_shared_ptr_new<GameObjectHandleData>(ptr->m_instanceData);
  }

  bool
  GameObjectHandleBase::isDestroyed(bool checkQueued) const {
    return  nullptr == m_data->m_ptr ||
            nullptr == m_data->m_ptr->object ||
            (checkQueued && m_data->m_ptr->object->_getIsDestroyed());
  }

  void
  GameObjectHandleBase::_setHandleData(const SPtr<GameObject>& object) {
    m_data->m_ptr = object->m_instanceData;
  }

  void
  GameObjectHandleBase::throwIfDestroyed() const {
    if (isDestroyed()) {
      GE_EXCEPT(InternalErrorException,
                "Trying to access an object that has been destroyed.");
    }
  }

  RTTITypeBase*
  GameObjectHandleBase::getRTTIStatic() {
    return GameObjectHandleRTTI::instance();
  }

  RTTITypeBase*
  GameObjectHandleBase::getRTTI() const {
    return GameObjectHandleBase::getRTTIStatic();
  }
}
