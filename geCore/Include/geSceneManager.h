/*****************************************************************************/
/**
 * @file    geSceneManager.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/05
 * @brief   Keeps track of all active SceneObject's and their components.
 *
 * Keeps track of all active SceneObject's and their components. Keeps track of
 * component state and triggers their events. Updates the transforms of objects
 * as SceneObject%s move.
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

#include <geModule.h>

namespace geEngineSDK {
  using std::move;
  using std::array;

  class LightProbeVolume;

  /**
   * @brief Information about a scene actor and the scene object it has been
   *        bound to.
   */
  struct BoundActorData
  {
    BoundActorData() = default;
    BoundActorData(const SPtr<SceneActor>& _actor, const HSceneObject& _so)
      : actor(_actor),
        so(_so)
    {}

    SPtr<SceneActor> actor;
    HSceneObject so;
  };

  /**
   * @brief Possible states components can be in. Controls which component
   *        callbacks are triggered.
   */
  namespace COMPONENT_STATE {
    enum E {
      /**
       * All components callbacks are being triggered normally.
       */
      kRunning,

      /**
       * All component callbacks except update are being triggered normally.
       */
      kPaused,

      /**
       * No component callbacks are being triggered.
       */
      kStopped
    };
  }

  /**
   * @brief Types of events that represent component state changes relevant to
   *        the scene manager.
   */
  namespace COMPONENT_STATE_EVENT {
    enum E {
      kCreated,
      kActivated,
      kDeactivated,
      kDestroyed
    };
  }

  class GE_CORE_EXPORT SceneManager : public Module<SceneManager>
  {
   public:
    SceneManager();
    ~SceneManager();

    /**
     * @brief Returns the root scene object.
     */
    HSceneObject
    getRootNode() const {
      return m_rootNode;
    }

    /**
     * @brief Destroys all scene objects in the scene.
     * @param[in] forceAll  If true, then even the persistent objects will be
     *            unloaded.
     */
    void
    clearScene(bool forceAll = false);

    /**
     * @brief Changes the root scene object. Any persistent objects will remain
     *        in the scene, now parented to the new root.
     */
    void
    setRootNode(const HSceneObject& root);

    /**
     * @brief Changes the component state that globally determines which
     *        component callbacks are activated. Only affects components that
     *        don't have the ComponentFlag::AlwaysRun flag set.
     */
    void
    setComponentState(COMPONENT_STATE::E state);

    /**
     * @brief Checks are the components currently in the Running state.
     */
    bool
    isRunning() const {
      return COMPONENT_STATE::kRunning == m_componentState;
    }

    /**
     * @brief Returns a list of all components of the specified type currently
     *        in the scene.
     * @tparam  T Type of the component to search for.
     * @param[in] activeOnly  If true only active components are returned,
     *            otherwise all components are returned.
     * @return  A list of all matching components in the scene.
     */
    template<class T>
    Vector<GameObjectHandle<T>>
    findComponents(bool activeOnly = true);

    /**
     * @brief Returns all cameras in the scene.
     */
    const UnorderedMap<Camera*, SPtr<Camera>>&
    getAllCameras() const {
      return m_cameras;
    }

    /**
     * @brief Returns the camera in the scene marked as main. Main camera
     *        controls the final render surface that is displayed to the user.
     *        If there are multiple main cameras, the first one found returned.
     */
    SPtr<Camera>
    getMainCamera() const;

    /**
     * @brief Sets the render target that the main camera in the scene (if any)
     *        will render its view to. This generally means the main game
     *        window when running standalone, or the Game viewport when running
     *        in editor.
     */
    void
    setMainRenderTarget(const SPtr<RenderTarget>& rt);

    /**
     * @brief Binds a scene actor with a scene object. Every frame the scene
     *        object's transform will be monitored for changes and those
     *        changes will be automatically transfered to the actor.
     */
    void
    _bindActor(const SPtr<SceneActor>& actor, const HSceneObject& so);

    /**
     * @brief Unbinds an actor that was previously bound using bindActor().
     */
    void
    _unbindActor(const SPtr<SceneActor>& actor);

    /**
     * @brief Returns a scene object bound to the provided actor, if any.
     */
    HSceneObject
    _getActorSO(const SPtr<SceneActor>& actor) const;

    /**
     * @brief Notifies the scene manager that a new camera was created.
     */
    void
    _registerCamera(const SPtr<Camera>& camera);

    /**
     * @brief Notifies the scene manager that a camera was removed.
     */
    void
    _unregisterCamera(const SPtr<Camera>& camera);

    /**
     * @brief Notifies the scene manager that a camera either became the main
     *        camera, or has stopped being main camera.
     */
    void
    _notifyMainCameraStateChanged(const SPtr<Camera>& camera);

    /**
     * @brief Called every frame. Calls update methods on all scene objects
     *        and their components.
     */
    void
    _update();

    /**
     * @brief Called at fixed time internals. Calls the fixed update method on
     *        all active components.
     */
    void
    _fixedUpdate();

    /**
     * @brief Updates dirty transforms on any core objects that may be tied
     *        with scene objects.
     */
    void
    _updateCoreObjectTransforms();

    /**
     * @brief Notifies the manager that a new component has just been created.
     *        The manager triggers necessary callbacks.
     */
    void
    _notifyComponentCreated(const HComponent& component, bool parentActive);

    /**
     * @brief Notifies the manager that a scene object the component belongs to
     *        was activated. The manager triggers necessary callbacks.
     */
    void
    _notifyComponentActivated(const HComponent& component, bool triggerEvent);

    /**
     * @brief Notifies the manager that a scene object the component belongs to
     *        was deactivated. The manager triggers necessary callbacks.
     */
    void
    _notifyComponentDeactivated(const HComponent& component, bool triggerEvent);

    /**
     * @brief Notifies the manager that a component is about to be destroyed.
     *        The manager triggers necessary callbacks.
     */
    void
    _notifyComponentDestroyed(const HComponent& component, bool immediate);

   protected:
    /**
     * @brief Describes a single component state change.
     */
    struct ComponentStateChange
    {
      ComponentStateChange(HComponent _obj, COMPONENT_STATE_EVENT::E _type)
        : obj(move(_obj)),
          type(_type)
      {}

      HComponent obj;
      COMPONENT_STATE_EVENT::E type;
    };

    friend class SceneObject;

    /**
     * @brief Register a new node in the scene manager, on the top-most level
     *        of the hierarchy.
     * @param[in] node  Node you wish to add. It's your responsibility not to
     *            add duplicate or null nodes. This method won't check.
     * @note  After you add a node in the scene manager, it takes ownership of
     *        its memory and is responsible for releasing it.
     * Do NOT add nodes that have already been added (if you just want to
     * change their parent). Normally this method will only be called by
     * SceneObject.
     */
    void
    registerNewSO(const HSceneObject& node);

    /**
     * @brief Callback that is triggered when the main render target size is
     *        changed.
     */
    void
    onMainRenderTargetResized();

    /**
     * @brief Adds a component to the specified state list. Caller is expected
     *        to first remove the component from any existing state lists.
     */
    void
    addToStateList(const HComponent& component, uint32 listType);

    /**
     * @brief Removes a component from its current scene manager state list
     *        (if any).
     */
    void
    removeFromStateList(const HComponent& component);

    /**
     * @brief Iterates over components that had their state modified and moves
     *        them to the appropriate state lists.
     */
    void
    processStateChanges();

    /**
     * @brief Encodes an index and a type into a single 32-bit integer. Top 2
     *        bits represent the type, while the rest represent the index.
     */
    uint32
    encodeComponentId(uint32 idx, uint32 type);

    /**
     * @brief Decodes an id encoded with encodeComponentId().
     */
    void
    decodeComponentId(uint32 id, uint32& idx, uint32& type);

    /**
     * @brief Checks does the specified component type match the provided
     *        RTTI id.
     */
    static bool
    isComponentOfType(const HComponent& component, uint32 rttiId);

   protected:
    HSceneObject m_rootNode;

    UnorderedMap<SceneActor*, BoundActorData> m_boundActors;
    UnorderedMap<Camera*, SPtr<Camera>> m_cameras;
    Vector<SPtr<Camera>> m_mainCameras;

    Vector<HComponent> m_activeComponents;
    Vector<HComponent> m_inactiveComponents;
    Vector<HComponent> m_uninitializedComponents;

    array<Vector<HComponent>*, 3> m_componentsPerState = {
      { &m_activeComponents,
        &m_inactiveComponents,
        &m_uninitializedComponents }
    };

    SPtr<RenderTarget> m_mainRT;
    HEvent m_mainRTResizedConn;

    COMPONENT_STATE::E m_componentState = COMPONENT_STATE::kRunning;
    bool m_disableStateChange = false;
    Vector<ComponentStateChange> m_stateChanges;
  };

  /**
   * @brief Provides easy access to the SceneManager.
   */
  GE_CORE_EXPORT SceneManager&
  g_sceneManager();

  template<class T>
  Vector<GameObjectHandle<T>>
  SceneManager::findComponents(bool activeOnly) {
    uint32 rttiId = T::getRTTIStatic()->getRTTIId();

    Vector<GameObjectHandle<T>> output;
    for (auto& entry : m_activeComponents) {
      if (isComponentOfType(entry, rttiId)) {
        output.push_back(static_object_cast<T>(entry));
      }
    }

    if (!activeOnly) {
      for (auto& entry : m_inactiveComponents) {
        if (isComponentOfType(entry, rttiId)) {
          output.push_back(static_object_cast<T>(entry));
        }
      }

      for (auto& entry : m_uninitializedComponents) {
        if (isComponentOfType(entry, rttiId)) {
          output.push_back(static_object_cast<T>(entry));
        }
      }
    }

    return output;
  }
}
