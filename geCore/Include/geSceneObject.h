/*****************************************************************************/
/**
 * @file    geSceneObject.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/01
 * @brief   An object in the scene graph.
 *
 * An object in the scene graph. It has a transform object that allows it to be
 * positioned, scaled and rotated. It can have other scene objects as children,
 * and will have a scene object as a parent, in which case transform changes to
 * the parent are reflected to the child scene objects (children are relative
 * to the parent).
 *
 * Each scene object can have one or multiple Component%s attached to it, where
 * the components inherit the scene object's transform, and receive updates
 * about transform and hierarchy changes.
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
#include "geGameObjectManager.h"
#include "geGameObject.h"
#include "geComponent.h"

#include <geMatrix4.h>
#include <geVector3.h>
#include <geQuaternion.h>
#include <geTransform.h>
#include <geRTTIType.h>

namespace geEngineSDK {
  using std::is_base_of;
  using std::forward;

  /**
   * @brief Possible modifiers that can be applied to a SceneObject.
   */
  namespace SCENE_OBJECT_FLAGS {
    enum E {
      /**
       * Object wont be in the main scene and its components won't receive
       * updates.
       */
      kDontInstantiate = 0x01,
      
      /**
       * Object will be skipped when saving the scene hierarchy or a prefab.
       */
      kDontSave = 0x02,

      /**
       * Object will remain in the scene even after scene clear, unless
       * destroyed directly. This only works with top-level objects.
       */
      kPersistent = 0x04,
      
      /**
       * Provides a hint to external systems that his object is used by engine
       * internals. For example, those systems might not want to display those
       * objects together with the user created ones.
       */
      kInternal = 0x08
    };
  }

  /**
   * @brief Flags that signify which part of the SceneObject needs updating.
   */
  namespace DIRTY_FLAGS {
    enum E {
      kLocalTransformDirty = 0x01,
      kWorldTransformDirty = 0x02
    };
  }

  class GE_CORE_EXPORT SceneObject : public GameObject
  {
    friend class SceneManager;
    friend class Prefab;
    friend class PrefabDiff;
    friend class PrefabUtility;
   public:
    ~SceneObject();

    /**
     * @brief Creates a new SceneObject with the specified name. Object will be
     *        placed in the top of the scene hierarchy.
     * @param[in] name  Name of the scene object.
     * @param[in] flags Optional flags that control object behavior.
     *            See SceneObjectFlags.
     */
    static HSceneObject
    create(const String& name, uint32 flags = 0);

    /**
     * @brief Destroys this object and any of its held components.
     * @param[in] immediate If true, the object will be deallocated and become
     *            unusable right away. Otherwise the deallocation will be
     *            delayed to the end of frame (preferred method).
     */
    void
    destroy(bool immediate = false);

    /**
     * @brief Returns a handle to this object.
     */
    HSceneObject
    getHandle() const {
      return m_thisHandle;
    }

    /**
     * @brief Returns the UUID of the prefab this object is linked to, if any.
     * @param[in] onlyDirect  If true, this method will return prefab link only
     *            for the root object of the prefab instance. If false the
     *            parent objects will be searched for the prefab ID.
     */
    UUID
    getPrefabLink(bool onlyDirect = false) const;

    /**
     * @brief Returns the root object of the prefab instance that this object
     *        belongs to, if any. Returns null if the object is not part of a
     *        prefab instance.
     */
    HSceneObject
    getPrefabParent() const;

    /**
     * @brief Breaks the link between this prefab instance and its prefab.
     *        Object will retain all current values but will no longer be
     *        influenced by modifications to its parent prefab.
     */
    void
    breakPrefabLink();

    /**
      * @brief Checks if the scene object has a specific bit flag set.
      */
    bool
    hasFlag(uint32 flag) const;

   public:
    /**
     * @copydoc GameObject::_setInstanceData
     */
    void
    _setInstanceData(GameObjectInstanceDataPtr& other) override;

    /**
     * @brief Register the scene object with the scene and activate all of its
     *        components.
     * @param[in] prefabOnly  If true, only objects within the current prefab
     *            will be instantiated. If false all child objects and
     *            components will.
     */
    void
    _instantiate(bool prefabOnly = false);

    /**
     * @brief Clears the internally stored prefab diff. If this object is
     *        updated from prefab its instance specific changes will be lost.
     */
    void
    _clearPrefabDiff() {
      m_prefabDiff = nullptr;
    }

    /**
     * @brief Returns the UUID of the prefab this object is linked to, if any.
     *        Unlike getPrefabLink() method this will not search parents, but
     *        instead return only the value assigned to this object.
     */
    const UUID&
    _getPrefabLinkUUID() const {
      return m_prefabLinkUUID;
    }

    /**
     * @brief Allows you to change the prefab link UUID of this object.
     *        Normally this should be accompanied by reassigning the link IDs.
     */
    void
    _setPrefabLinkUUID(const UUID& UUID) {
      m_prefabLinkUUID = UUID;
    }

    /**
     * @brief Returns a prefab diff object containing instance specific
     *        modifications of this object compared to its prefab reference,
     *        if any.
     */
    const SPtr<PrefabDiff>&
    _getPrefabDiff() const {
      return m_prefabDiff;
    }

    /**
     * @brief Assigns a new prefab diff object. Caller must ensure the prefab
     *        diff was generated for this object.
     */
    void
    _setPrefabDiff(const SPtr<PrefabDiff>& diff) {
      m_prefabDiff = diff;
    }

    /**
     * @brief Recursively enables the provided set of flags on this object and
     *        all children.
     */
    void
    _setFlags(uint32 flags);

    /**
     * @brief Recursively disables the provided set of flags on this object and
     *        all children.
     */
    void
    _unsetFlags(uint32 flags);

   private:
    SceneObject(const String& name, uint32 flags);

    /**
     * @brief Creates a new SceneObject instance, registers it with the game
     *        object manager, creates and returns a handle to the new object.
     * @note When creating objects with DontInstantiate flag it is the callers
     *        responsibility to manually destroy the object, otherwise it will
     *        leak.
     */
    static HSceneObject
    createInternal(const String& name, uint32 flags = 0);

    /**
     * @brief Creates a new SceneObject instance from an existing pointer,
     *        registers it with the game object manager, creates and returns a
     *        handle to the object.
     * @param[in] soPtr Pointer to the scene object register and return a
     *            handle to.
     * @param[in] originalId  If the provided pointer was deserialized, this is
     *            the original object's ID at the time of serialization. Used
     *            for resolving handles pointing to the object.
     */
    static HSceneObject
    createInternal(const SPtr<SceneObject>& soPtr);

    /**
     * @brief Destroys this object and any of its held components.
     * @param[in] handle  Game object handle to this object.
     * @param[in] immediate If true, the object will be deallocated and become
     *            unusable right away. Otherwise the deallocation will be
     *            delayed to the end of frame (preferred method).
     * @note  Unlike destroy(), does not remove the object from its parent.
     */
    void
    destroyInternal(GameObjectHandleBase& handle,
                    bool immediate = false) override;

    /**
     * @brief Checks is the scene object instantiated and visible in the scene.
     */
    bool
    isInstantiated() const {
      return (m_flags & SCENE_OBJECT_FLAGS::kDontInstantiate) == 0;
    }

   private:
    HSceneObject m_thisHandle;
    UUID m_prefabLinkUUID;
    SPtr<PrefabDiff> m_prefabDiff;
    uint32 m_prefabHash;
    uint32 m_flags;

    /*************************************************************************/
    /**
     * Transforms
     */
    /*************************************************************************/
   public:
    /**
     * @brief Gets the transform object representing object's
     *        position / rotation/ scale in world space.
     */
    const Transform&
    getTransform() const;

    /**
     * @brief Gets the transform object representing object's
     *        position / rotation / scale relative to its parent.
     */
    const Transform&
    getLocalTransform() const {
      return m_localTransform;
    }

    /**
     * @brief Sets the local position of the object.
     */
    void
    setTranslation(const Vector3& translation);

    /**
     * @brief Sets the world position of the object.
     */
    void
    setWorldPosition(const Vector3& position);

    /**
     * @brief Sets the local rotation of the object.
     */
    void
    setRotation(const Quaternion& rotation);

    /**
     * @brief Sets the world rotation of the object.
     */
    void
    setWorldRotation(const Quaternion& rotation);

    /**
     * @brief Sets the local scale of the object.
     */
    void
    setScale(const Vector3& scale);

    /**
     * @brief Sets the world scale of the object.
     * @note  This will not work properly if this object or any of its parents
     *        have non-affine transform matrices.
     */
    void
    setWorldScale(const Vector3& scale);

    /**
     * @brief Orients the object so it is looking at the provided @p location
     *        (world space) where @p up is used for determining the location of
     *        the object's Y axis.
     */
    void
    lookAt(const Vector3& location, const Vector3& up = Vector3::UP);

    /**
     * @brief Gets the objects world transform matrix.
     * @note  Performance warning: This might involve updating the transforms
     *        if the transform is dirty.
     */
    const Matrix4&
    getWorldMatrix() const;

    /**
     * @brief Gets the objects inverse world transform matrix.
     * @note  Performance warning: This might involve updating the transforms
              if the transform is dirty.
     */
    Matrix4
    getInvWorldMatrix() const;

    /**
     * @brief Gets the objects local transform matrix.
     */
    const Matrix4&
    getLocalMatrix() const;

    /**
     * @brief Moves the object's position by the vector offset provided along
     *        world axes.
     */
    void
    move(const Vector3& vec);

    /**
     * @brief Moves the object's position by the vector offset provided along
     *        it's own axes (relative to orientation).
     */
    void
    moveRelative(const Vector3& vec);

    /**
     * @brief Rotates the game object so it's forward axis faces the provided
     *        direction.
     * @param[in] forwardDir  The forward direction to face, in world space.
     * @note  Local forward axis is considered to be negative Z.
     */
    void
    setForward(const Vector3& forwardDir);

    /**
     * @brief Rotate the object around an arbitrary axis.
     */
    void
    rotate(const Vector3& axis, const Radian& angle);

    /**
     * @brief Rotate the object around an arbitrary axis using a Quaternion.
     */
    void
    rotate(const Quaternion& q);

    /**
     * @brief Rotates around local Forward axis.
     * @param[in] angle Angle to rotate by.
     */
    void
    roll(const Radian& angle);

    /**
     * @brief Rotates around Y axis.
     * @param[in] angle Angle to rotate by.
     */
    void
    yaw(const Radian& angle);

    /**
     * @brief Rotates around X axis
     * @param[in] angle Angle to rotate by.
     */
    void
    pitch(const Radian& angle);

    /**
     * @brief Forces any dirty transform matrices on this object to be updated.
     * @note  Normally this is done internally when retrieving a transform, but
     *        sometimes it is useful to update transforms manually.
     */
    void
    updateTransformsIfDirty();

    /**
     * @brief Returns a hash value that changes whenever a scene objects
     *        transform gets updated. It allows you to detect changes with the
     *        local or world transforms without directly comparing their values
     *        with some older state.
     */
    uint32
    getTransformHash() const {
      return m_dirtyHash;
    }

   private:
    Transform m_localTransform;
    mutable Transform m_worldTransform;

    mutable Matrix4 m_cachedLocalTransform;
    mutable Matrix4 m_cachedWorldTransforms;

    mutable uint32 m_dirtyFlags;
    mutable uint32 m_dirtyHash;

    /**
     * @brief Notifies components and child scene object that a transform has
     *        been changed.
     * @param	flags Specifies in what way was the transform changed.
     */
    void
    notifyTransformChanged(TransformChangedFlags flags) const;

    /**
     * @brief Updates the local transform. Normally just reconstructs the
     *        transform matrix from the position/rotation/scale.
     */
    void
    updateLocalTransform() const;

    /**
     * @brief Updates the world transform. Reconstructs the local transform
     *        matrix and multiplies it with any parent transforms.
     * @note	If parent transforms are dirty they will be updated.
     */
    void
    updateWorldTransform() const;

    /**
     * @brief Checks if cached local transform needs updating.
     */
    bool
    isCachedLocalTransformUpToDate() const {
      return (m_dirtyFlags & DIRTY_FLAGS::kLocalTransformDirty) == 0;
    }

    /**
     * @brief Checks if cached world transform needs updating.
     */
    bool
    isCachedWorldTransformUpToDate() const {
      return (m_dirtyFlags & DIRTY_FLAGS::kWorldTransformDirty) == 0;
    }

    /*************************************************************************/
    /**
     * Hierarchy
     */
    /*************************************************************************/
   public:
    /**
     * @brief Changes the parent of this object. Also removes the object from
     *        the current parent, and assigns it to the new parent.
     * @param[in] parent  New parent.
     * @param[in] keepWorldTransform  Determines should the current transform
     *            be maintained even after the parent is changed (this means
     *            the local transform will be modified accordingly).
     */
    void
    setParent(const HSceneObject& parent, bool keepWorldTransform = true);

    /**
     * @brief Gets the parent of this object.
     * @return  Parent object, or nullptr if this SceneObject is at root level.
     */
    HSceneObject
    getParent() const {
      return m_parent;
    }

    /**
     * @brief Gets a child of this item.
     * @param[in] idx The zero based index of the child.
     * @return  SceneObject of the child.
     */
    HSceneObject
    getChild(uint32 idx) const;

    /**
     * @brief Find the index of the specified child. Don't persist this value
     *        as it may change whenever you add / remove children.
     * @param[in]	child	The child to look for.
     * @return  The zero-based index of the found child,
     *          or -1 if no match was found.
     */
    int32
    indexOfChild(const HSceneObject& child) const;

    /**
     * @brief Gets the number of all child GameObjects.
     */
    uint32
    getNumChildren() const {
      return static_cast<uint32>(m_children.size());
    }

    /**
     * @brief Searches the scene object hierarchy to find a child scene object
     *        using the provided path.
     * @param[in] path  Path to the property, where each element of the path is
     *            separated with "/" Path elements signify names of child scene
     *            objects (first one relative to this object).
     */
    HSceneObject
    findPath(const String& path) const;

    /**
     * @brief Searches the child objects for an object matching the specified
     *        name.
     * @param[in] name  Name of the object to locate.
     * @param[in] recursive If true all descendants of the scene object will be
     *            searched, otherwise only immediate children.
     * @return First found scene object, or empty handle if none found.
     */
    HSceneObject
    findChild(const String& name, bool recursive = true);

    /**
     * @brief Searches the child objects for objects matching the specified
     *        name.
     * @param[in] name  Name of the objects to locate.
     * @param[in] recursive If true all descendants of the scene object will
     *            be searched, otherwise only immediate children.
     * @return  All scene objects matching the specified name.
     */
    Vector<HSceneObject>
    findChildren(const String& name, bool recursive = true);

    /**
     * @brief Enables or disables this object. Disabled objects also implicitly
     *        disable all their child objects. No components on the disabled
     *        object are updated.
     */
    void
    setActive(bool active);

    /**
     * @brief Returns whether or not an object is active.
     * @param[in] self  If true, the method will only check if this particular
     *            object was activated or deactivated directly via setActive.
     *            If false we we also check if any of the objects parents are
     *            inactive.
     */
    bool
    getActive(bool self = false) const;

    /**
     * @brief Sets the mobility of a scene object. This is used primarily as a
     *        performance hint to engine systems. Objects with more restricted
     *        mobility will result in higher performance. Some mobility
     *        constraints will be enforced by the engine itself, while for
     *        others the caller must be sure not to break the promise he made
     *        when mobility was set. By default scene object's mobility is
     *        unrestricted.
     */
    void
    setMobility(ObjectMobility mobility);

    /**
     * @brief Gets the mobility setting for this scene object.
     *        See setMobility();
     */
    ObjectMobility
    getMobility() const {
      return m_mobility;
    }

    /**
     * @brief Makes a deep copy of this object.
     * @param[in] instantiate If false, the cloned hierarchy will just be a
     *            memory copy, but will not be present in the scene or
     *            otherwise active until instantiate() is called.
     */
    HSceneObject
    clone(bool instantiate = true);

   private:
    HSceneObject m_parent;
    Vector<HSceneObject> m_children;
    bool m_activeSelf;
    bool m_activeHierarchy;
    ObjectMobility m_mobility;

    /**
     * @brief Internal version of setParent() that allows you to set a null
     *        parent.
     * @param[in] parent  New parent.
     * @param[in] keepWorldTransform  Determines should the current transform
     *            be maintained even after the parent is changed (this means
     *            the local transform will be modified accordingly).
     */
    void
    _setParent(const HSceneObject& parent, bool keepWorldTransform = true);

    /**
     * @brief Adds a child to the child array. This method doesn't check for
     *        null or duplicate values.
     * @param[in] object  New child.
     */
    void
    addChild(const HSceneObject& object);

    /**
     * @brief Removes the child from the object.
     * @param[in] object  Child to remove.
     */
    void
    removeChild(const HSceneObject& object);

    /**
     * @brief Changes the object active in hierarchy state, and triggers
     *        necessary events.
     */
    void
    setActiveHierarchy(bool active, bool triggerEvents = true);

    /*************************************************************************/
    /**
     * Component
     */
    /*************************************************************************/
   public:
    /**
     * @brief Constructs a new component of the specified type and adds it to
     *        the internal component list.
     */
    template<class T, class... Args>
    GameObjectHandle<T>
    addComponent(Args &&... args) {
      static_assert((is_base_of<geEngineSDK::Component, T>::value),
                    "Specified type is not a valid Component.");

      SPtr<T> gameObject(new (ge_alloc<T>()) T(m_thisHandle, forward<Args>(args)...),
                         &ge_delete<T>,
                         StdAlloc<T>());

      GameObjectHandle<T>
        newComponent = GameObjectManager::instance().registerObject(gameObject);

      addAndInitializeComponent(newComponent);

      return newComponent;
    }

    /**
     * @brief Constructs a new component of the specified type id and adds it
     *        to the internal component list. Component must have a
     *        parameterless constructor.
     */
    HComponent
    addComponent(uint32 typeId);

    /**
     * @brief Searches for a component with the specific type and returns the
     *        first one it finds. Will also return components derived from the
     *        type.
     * @tparam  T Type of the component.
     * @return  Component if found, nullptr otherwise.
     * @note  Don't call this too often as it is relatively slow. It is more
     *        efficient to call it once and store the result for further use.
     */
    template<typename T>
    GameObjectHandle<T>
    getComponent() {
      static_assert((is_base_of<geEngineSDK::Component, T>::value),
                    "Specified type is not a valid Component.");
      return static_object_cast<T>(getComponent(T::getRTTIStatic()));
    }

    /**
     * @brief Returns all components with the specific type. Will also return
     *        components derived from the type.
     * @tparam  typename T  Type of the component.
     * @return  Array of found components.
     * @note Don't call this too often as it is relatively slow. It is more
     *      efficient to call it once and store the result for further use.
     */
    template <typename T>
    Vector<GameObjectHandle<T>>
    getComponents() {
      static_assert((is_base_of<geEngineSDK::Component, T>::value),
                    "Specified type is not a valid Component.");
      Vector<GameObjectHandle<T>> output;
      for (auto entry : m_components) {
        if (entry->getRTTI()->isDerivedFrom(T::getRTTIStatic())) {
          output.push_back(entry);
        }
      }

      return output;
    }

    /**
     * @brief Checks if the current object contains the specified component or
     *        components derived from the provided type.
     * @tparam  typename T  Type of the component.
     * @return  True if component exists on the object.
     * @note  Don't call this too often as it is relatively slow.
     */
    template <typename T>
    bool
    hasComponent() {
      static_assert((is_base_of<geEngineSDK::Component, T>::value),
                    "Specified type is not a valid Component.");

      for (const auto& entry : m_components) {
        if (entry->getRTTI()->isDerivedFrom(T::getRTTIStatic())) {
          return true;
        }
      }

      return false;
    }

    /**
     * @brief Searches for a component with the specified type and returns the
     *        first one it finds. Will also return components derived from the
     *        type.
     * @param[in] type  RTTI information for the type.
     * @return  Component if found, nullptr otherwise.
     * @note  Don't call this too often as it is relatively slow. It is more
     *        efficient to call it once and store the result for further use.
     */
    HComponent
    getComponent(RTTITypeBase* type) const;

    /**
     * @brief Removes the component from this object, and deallocates it.
     * @param[in]	component	The component to destroy.
     * @param[in]	immediate	If true, the component will be deallocated and
     *            become unusable right away. Otherwise the deallocation will
     *            be delayed to the end of frame (preferred method).
     */
    void
    destroyComponent(const HComponent component, bool immediate = false);

    /**
     * @brief Removes the component from this object, and deallocates it.
     * @param[in] component The component to destroy.
     * @param[in] immediate If true, the component will be deallocated and
     *            become unusable right away. Otherwise the deallocation will
     *            be delayed to the end of frame (preferred method).
     */
    void
    destroyComponent(Component* component, bool immediate = false);

    /**
     * @brief Returns all components on this object.
     */
    const Vector<HComponent>&
    getComponents() const {
      return m_components;
    }

   private:
    /**
     * @brief Creates an empty component with the default constructor.
     */
    template<typename T>
    static SPtr<T>
    createEmptyComponent() {
      static_assert((is_base_of<geEngineSDK::Component, T>::value),
                    "Specified type is not a valid Component.");
      T* rawPtr = new (ge_alloc<T>()) T();
      SPtr<T> gameObject(rawPtr, &ge_delete<T>, StdAlloc<T>());
      return gameObject;
    }

    /**
     * @brief Adds the component to the internal component array.
     */
    void
    addComponentInternal(const SPtr<Component> component);

    /**
     * @brief Adds the component to the internal component array, and
     *        initializes it.
     */
    void
    addAndInitializeComponent(const HComponent& component);

    /**
     * @brief Adds the component to the internal component array, and
     *        initializes it.
     */
    void
    addAndInitializeComponent(const SPtr<Component> component);

    Vector<HComponent> m_components;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/
   public:
    friend class GameObjectRTTI;
    friend class SceneObjectRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };
}
