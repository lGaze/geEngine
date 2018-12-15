/*****************************************************************************/
/**
 * @file    geSceneActor.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/02
 * @brief   A base class for objects that can be placed in the scene.
 *
 * A base class for objects that can be placed in the scene. It has a transform
 * object that allows it to be positioned, scaled and rotated, as well a
 * properties that control its mobility (movable vs. immovable) and active
 * status.
 *
 * In a way scene actors are similar to SceneObject 's, the main difference
 * being that their implementations perform some functionality directly, rather
 * than relying on attached Components. Scene actors can be considered as a
 * lower-level alternative to SceneObject / Component model. In fact many
 * Components internally just wrap scene actors.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geSceneActor.h"
#include "geSceneObject.h"

namespace geEngineSDK {
  void
  SceneActor::setTransform(const Transform& transform) {
    if (ObjectMobility::Movable != m_mobility) {
      return;
    }

    m_transform = transform;
    _markCoreDirty(ACTOR_DIRTY_FLAG::kTransform);
  }

  void
  SceneActor::setMobility(ObjectMobility mobility) {
    m_mobility = mobility;
    _markCoreDirty(ACTOR_DIRTY_FLAG::kMobility);
  }

  void
  SceneActor::setActive(bool active) {
    m_active = active;
    _markCoreDirty(ACTOR_DIRTY_FLAG::kActive);
  }

  void
  SceneActor::_updateState(const SceneObject& so, bool force) {
    uint32 curHash = so.getTransformHash();
    if (curHash != m_hash || force) {
      setTransform(so.getTransform());
      m_hash = curHash;
    }

    if (so.getActive() != m_active || force) {
      setActive(so.getActive());
    }

    if (so.getMobility() != m_mobility || force) {
      setMobility(so.getMobility());
    }
  }
}
