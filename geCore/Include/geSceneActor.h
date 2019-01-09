/*****************************************************************************/
/**
 * @file    geSceneActor.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include <geTransform.h>

namespace geEngineSDK {
  namespace ACTOR_DIRTY_FLAG {
    enum E {
      kTransform = 1 << 0,
      kMobility = 1 << 1,
      kActive = 1 << 2,
      kEverything = 1 << 3,
      kDependency = DIRTY_DEPENDENCY_MASK
    };
  }

  using ActorDirtyFlags = Flags<ACTOR_DIRTY_FLAG::E>;
  GE_FLAGS_OPERATORS(ACTOR_DIRTY_FLAG::E)

  class GE_CORE_EXPORT SceneActor
  {
   public:
    SceneActor() = default;
    virtual ~SceneActor() = default;

    /**
     * @brief Determines the position, rotation and scale of the actor.
     */
    virtual void
    setTransform(const Transform& transform);

    /**
     * @copydoc setTransform
     */
    const Transform&
    getTransform() const {
      return m_transform;
    }

    /**
     * @brief Shorthand for getTransform().
     */
    const Transform&
    tfrm() const {
      return m_transform;
    }

    /**
     * @brief Determines if the actor is currently active. Deactivated actors
     *        act as if they have been destroyed, without actually being
     *        destroyed.
     */
    virtual void
    setActive(bool active);

    /**
     * @copydoc setActive
     */
    bool
    getActive() const {
      return m_active;
    }

    /**
     * @brief Determines the mobility of the actor. This is used primarily as a
     *        performance hint to engine systems. Objects with more restricted
     *        mobility will result in higher performance. Any transform changes
     *        to immobile actors will be ignored. By default actor's mobility
     *        is unrestricted.
     */
    virtual void
    setMobility(ObjectMobility mobility);

    /**
     * @copydoc setMobility
     */
    ObjectMobility
    getMobility() const {
      return m_mobility;
    }

    /**
     * @brief Updates the internal actor state by transferring the relevant
     *        state from the scene object. The system tracks the last state and
     *        only performs the update if the scene object was modified since
     *        the last call. You can force an update by setting the @p force
     *        parameter to true.
     * This method is used by the scene manager to update actors that have been
     * bound to a scene object. Never call this method for multiple different
     * scene objects, as actor can only ever be bound to one during its
     * lifetime.
     */
    virtual void
    _updateState(const SceneObject& so, bool force = false);

    /**
     * @brief Enumerates all the fields in the type and executes the specified
     *        processor action for each field.
     */
    template<class P>
    void
    rttiEnumFields(P p, ActorDirtyFlags flags = ACTOR_DIRTY_FLAG::kEverything) {
      if (flags.isSetAny(ACTOR_DIRTY_FLAG::kTransform |
                         ACTOR_DIRTY_FLAG::kEverything)) {
        p(m_transform);
      }

      if (flags.isSetAny(ACTOR_DIRTY_FLAG::kActive |
                         ACTOR_DIRTY_FLAG::kEverything)) {
        p(m_active);
      }

      if (flags.isSetAny(ACTOR_DIRTY_FLAG::kMobility |
                         ACTOR_DIRTY_FLAG::kEverything)) {
        p(m_mobility);
      }
    }

   protected:
    /**
     * @brief Marks the simulation thread object as dirty and notifies the
     *        system its data should be synced with its core thread
     *        counterpart.
     */
    virtual void
    _markCoreDirty(ACTOR_DIRTY_FLAG::E flag = ACTOR_DIRTY_FLAG::kEverything) {
      GE_UNREFERENCED_PARAMETER(flag);
    }

   protected:
    friend class SceneManager;

    Transform m_transform;
    ObjectMobility m_mobility = ObjectMobility::Movable;
    bool m_active = true;
    uint32 m_hash = 0;
  };
}
