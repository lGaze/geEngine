/*****************************************************************************/
/**
 * @file    geComponent.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/04/28
 * @brief   Components represent primary logic elements in the scene.
 *          They are attached to scene objects.
 *
 * Components represent primary logic elements in the scene. They are attached
 * to scene objects.
 *
 * You should implement some or all of update / onCreated / onInitialized /
 * onEnabled / onDisabled / onTransformChanged / onDestroyed methods to
 * implement the relevant component logic. Avoid putting logic in constructors
 * or destructors.
 *
 * Components can be in different states. These states control which of the
 * events listed above trigger:
 * - Running - Scene manager is sending out events.
 * - Paused  - Scene manager is sending out all events except per-frame
 *             update()
 * - Stopped - Scene manager is not sending out events except for
 *             onCreated / onDestroyed.
 *
 * These states can be changed globally though SceneManager and affect all
 * components. Individual components can override these states in two ways:
 * - Set the COMPONENT_FLAG::kAlwaysRun to true and the component will always
 *   stay in Running state, regardless of state set in SceneManager. This flag
 *   should be set in constructor and not change during component lifetime.
 * - If the component's parent SceneObject is inactive
 *   (SceneObject::setActive(false)), or any of his parents are inactive, then
 *   the component is considered to be in Stopped state, regardless whether the
 *   COMPONENT_FLAG::kAlwaysRun flag is set or not.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geComponent.h"
#include "geGameObjectManager.h"
#include "geSceneObject.h"
#include "geComponentRTTI.h"

namespace geEngineSDK {
  Component::Component(HSceneObject parent)
    : m_parent(std::move(parent)) {
    setName("Component");
  }

  bool
  Component::typeEquals(const Component& other) {
    return getRTTI()->getRTTIId() == other.getRTTI()->getRTTIId();
  }

  bool
  Component::calculateBounds(BoxSphereBounds& bounds) {
    Vector3 position = so()->getTransform().getTranslation();
    bounds = BoxSphereBounds(AABox(position, position), Sphere(position, 0.0f));
    return false;
  }

  void
  Component::destroy(bool immediate) {
    so()->destroyComponent(this, immediate);
  }

  void
  Component::destroyInternal(GameObjectHandleBase& handle, bool immediate) {
    if (immediate) {
      GameObjectManager::instance().unregisterObject(handle);
    }
    else {
      GameObjectManager::instance().queueForDestroy(handle);
    }
  }

  RTTITypeBase*
  Component::getRTTIStatic() {
    return ComponentRTTI::instance();
  }

  RTTITypeBase*
  Component::getRTTI() const {
    return Component::getRTTIStatic();
  }
}
