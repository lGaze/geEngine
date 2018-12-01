/*****************************************************************************/
/**
 * @file    geComponent.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geGameObject.h"
#include "geBoxSphereBounds.h"

namespace geEngineSDK {
  namespace COMPONENT_FLAG {
    enum E {
      /**
       * Ensures that scene manager cannot pause or stop component callbacks
       * from executing. Off by default.
       * Note that this flag must be specified on component creation, in its
       * constructor and any later changes to the flag will be ignored.
       */
      kAlwaysRun = 1
    };
  }

  using ComponentFlag = COMPONENT_FLAG::E;
  using ComponentFlags = Flags<ComponentFlag>;
  GE_FLAGS_OPERATORS(ComponentFlag);

  class GE_CORE_EXPORT Component : public GameObject
  {
   public:
    /**
     * @brief Returns the SceneObject this Component is assigned to.
     */
    HSceneObject
    sceneObject() const {
      return m_parent;
    }

    /**
     * @copydoc sceneObject
     */
    HSceneObject
    so() const {
      return sceneObject();
    }

    /**
     * @brief Returns a handle to this object.
     */
    HComponent
    getHandle() const {
      return m_thisHandle;
    }

    /**
     * @brief Called once per frame. Only called if the component is in
     *        Running state.
     */
    virtual void
    update() {}

    /**
     * @brief Called at fixed time intervals (e.g. 60 times per frame).
     *        Generally any physics-related functionality should go in this
     *        method in order to ensure stability of calculations. Only called
     *        if the component is in Running state.
     */
    virtual void
    fixedUpdate() {}

    /**
     * @brief Calculates bounds of the visible contents represented by this
     *        component (for example a mesh for Renderable).
     * @param[in] bounds  Bounds of the contents in world space coordinates.
     * @return  true if the component has bounds with non-zero volume,
     *          otherwise false.
     */
    virtual bool
    calculateBounds(BoxSphereBounds& bounds);

    /**
     * @brief Checks if this and the provided component represent the same type
     * @note RTTI type cannot be checked directly since components can be
     *       further specialized internally for scripting purposes.
     */
    virtual bool
    typeEquals(const Component& other);

    /**
     * @brief Removes the component from parent SceneObject and deletes it.
     *        All the references to this component will be marked as destroyed
     *        and you will get an exception if you try to use them.
     * @param[in] immediate If true the destruction will be performed
     *            immediately, otherwise it will be delayed until the end of
     *            the current frame (preferred option).
     */
    void
    destroy(bool immediate = false);

    /**
     * @brief Construct any resources the component needs before use. Called
     *        when the parent scene object is instantiated.
     * A non-instantiated component shouldn't be used for any other purpose
     * than serialization.
     */
    virtual void
    _instantiate() {}

    /**
     * @brief Sets new flags that determine when is onTransformChanged called.
     */
    void
    setNotifyFlags(TransformChangedFlags flags) {
      m_notifyFlags = flags;
    }

    /**
     * @brief Gets the currently assigned notify flags. See _setNotifyFlags().
     */
    TransformChangedFlags
    _getNotifyFlags() const {
      return m_notifyFlags;
    }

   protected:
    friend class SceneManager;
    friend class SceneObject;
    friend class SceneObjectRTTI;

    Component(HSceneObject parent);
    virtual ~Component() = default;

    /**
     * @brief Called once when the component has been created.
     *        Called regardless of the state the component is in.
     */
    virtual void
    onCreated() {}

    /**
     * @brief Called once when the component first leaves the Stopped state.
     *        This includes component creation if requirements for leaving
     *        Stopped state are met, in which case it is called after onCreated
     */
    virtual void
    onInitialized() {}

    /**
     * @brief Called once just before the component is destroyed.
     *        Called regardless of the state the component is in.
     */
    virtual void
    onDestroyed() {}

    /**
     * @brief Called every time a component is placed into the Stopped state.
     *        This includes component destruction if component wasn't already
     *        in Stopped state during destruction. When called during
     *        destruction it is called before onDestroyed.
     */
    virtual void
    onDisabled() {}

    /**
     * @brief Called every time a component leaves the Stopped state. This
     *        includes component creation if requirements for leaving the
     *        Stopped state are met. When called during creation it is called
     *        after onInitialized.
     */
    virtual void
    onEnabled() {}

    /**
     * @brief Called when the component's parent scene object has changed.
     *        Not called if the component is in Stopped state.
     *        Also only called if necessary notify flags are set via
     *        _setNotifyFlags().
     */
    virtual void
    onTransformChanged(TransformChangedFlags /*flags*/) {}

    /**
     * @brief Checks whether the component wants to received the specified
     *        transform changed message.
     */
    bool
    supportsNotify(TransformChangedFlags flags) const {
      return (m_notifyFlags & flags) != 0;
    }

    /**
     * @brief Enables or disabled a flag controlling component's behaviour.
     */
    void
    setFlag(ComponentFlag flag, bool enabled) {
      if (enabled) {
        m_flags.set(flag);
      }
      else {
        m_flags.unset(flag);
      }
    }

    /**
     * @brief Checks if the component has a certain flag enabled.
     */
    bool
    hasFlag(ComponentFlag flag) const {
      return m_flags.isSet(flag);
    }

    /**
     * @brief Sets an index that uniquely identifies a component with the
     *        SceneManager.
     */
    void
    setSceneManagerId(uint32 id) {
      m_sceneManagerId = id;
    }

    /**
     * @brief Returns an index that unique identifies a component with the
     *        SceneManager.
     */
    uint32
    getSceneManagerId() const {
      return m_sceneManagerId;
    }

    /**
     * @brief Destroys this component.
     * @param[in] handle    Game object handle this this object.
     * @param[in] immediate If true, the object will be deallocated and become
     *            unusable right away. Otherwise the deallocation will be
     *            delayed to the end of frame (preferred method).
     * @note  Unlike destroy(), does not remove the component from its parent.
     */
    void
    destroyInternal(GameObjectHandleBase& handle,
                    bool immediate) override;

   private:
    Component(const Component& other) = default;

   protected:
    HComponent m_thisHandle;
    TransformChangedFlags m_notifyFlags = TransformChangedFlags::kNone;
    ComponentFlags m_flags;
    uint32 m_sceneManagerId = NumLimit::MAX_UINT32;

   private:
    HSceneObject m_parent;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/
   public:
    friend class ComponentRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;

   protected:
    Component() = default;  //Serialization only
  };
}
