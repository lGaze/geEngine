/*****************************************************************************/
/**
 * @file    geSceneObject.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geSceneObject.h"
#include "geComponent.h"
#include "geSceneManager.h"
#include "geSceneObjectRTTI.h"
#include "geGameObjectManager.h"
#include "gePrefabUtility.h"
#include "geMatrix4.h"
#include "geCoreApplication.h"

#include <geException.h>
#include <geDebug.h>
#include <geMemorySerializer.h>

namespace geEngineSDK {
  using std::function;
  using std::static_pointer_cast;
  using std::find_if;

  SceneObject::SceneObject(const String& name, uint32 flags)
    : GameObject(),
      m_prefabHash(0),
      m_flags(flags),
      m_cachedLocalTransform(Matrix4::IDENTITY),
      m_cachedWorldTransforms(Matrix4::IDENTITY),
      m_dirtyFlags(0xFFFFFFFF),
      m_dirtyHash(0),
      m_activeSelf(true),
      m_activeHierarchy(true),
      m_mobility(ObjectMobility::Movable) {
    setName(name);
  }

  SceneObject::~SceneObject() {
    if (!m_thisHandle.isDestroyed()) {
      LOGWRN("Object is being deleted without being destroyed first? " + m_name);
      destroyInternal(m_thisHandle, true);
    }
  }

  HSceneObject
  SceneObject::create(const String& name, uint32 flags) {
    HSceneObject newObject = createInternal(name, flags);

    if (newObject->isInstantiated()) {
      g_sceneManager().registerNewSO(newObject);
    }
    return newObject;
  }

  HSceneObject
  SceneObject::createInternal(const String& name, uint32 flags) {
    auto sceneObjectPtr = SPtr<SceneObject>(new (ge_alloc<SceneObject>())
                                                 SceneObject(name, flags),
                                            &ge_delete<SceneObject>, StdAlloc<SceneObject>());

    HSceneObject sceneObject = static_object_cast<SceneObject>
      (GameObjectManager::instance().registerObject(sceneObjectPtr));
    sceneObject->m_thisHandle = sceneObject;

    return sceneObject;
  }

  HSceneObject
  SceneObject::createInternal(const SPtr<SceneObject>& soPtr) {
    HSceneObject sceneObject = static_object_cast<SceneObject>
      (GameObjectManager::instance().registerObject(soPtr));
    sceneObject->m_thisHandle = sceneObject;
    return sceneObject;
  }

  void
  SceneObject::destroy(bool immediate) {
    //Parent is our owner, so when his reference to us is removed, delete might
    //be called. So make sure this is the last thing we do.
    if (nullptr != m_parent) {
      if (!m_parent.isDestroyed()) {
        m_parent->removeChild(m_thisHandle);
      }
      m_parent = nullptr;
    }

    destroyInternal(m_thisHandle, immediate);
  }

  void
  SceneObject::destroyInternal(GameObjectHandleBase& handle, bool immediate) {
    if (immediate) {
      for (auto & iter : m_children) {
        iter->destroyInternal(iter, true);
      }

      m_children.clear();

      //It's important to remove the elements from the array as soon as they're
      //destroyed, as OnDestroy callbacks for components might query the SO's
      //components, and we want to only return live ones 
      while (!m_components.empty()) {
        HComponent component = m_components.back();
        component->_setIsDestroyed();

        if (isInstantiated()) {
          g_sceneManager()._notifyComponentDestroyed(component, immediate);
        }
        component->destroyInternal(component, true);
        m_components.erase(m_components.end() - 1);
      }

      GameObjectManager::instance().unregisterObject(handle);
    }
    else {
      GameObjectManager::instance().queueForDestroy(handle);
    }
  }

  void
  SceneObject::_setInstanceData(GameObjectInstanceDataPtr& other) {
    GameObject::_setInstanceData(other);

    //Instance data changed, so make sure to refresh the handles to reflect it
    SPtr<SceneObject> thisPtr = m_thisHandle.getInternalPtr();
    m_thisHandle._setHandleData(thisPtr);
  }

  UUID
  SceneObject::getPrefabLink(bool onlyDirect) const {
    const SceneObject* curObj = this;

    while (nullptr != curObj) {
      if (!curObj->m_prefabLinkUUID.empty()) {
        return curObj->m_prefabLinkUUID;
      }

      if (nullptr != curObj->m_parent && !onlyDirect) {
        curObj = curObj->m_parent.get();
      }
      else {
        curObj = nullptr;
      }
    }

    return UUID::EMPTY;
  }

  HSceneObject
  SceneObject::getPrefabParent() const {
    HSceneObject curObj = m_thisHandle;

    while (nullptr != curObj) {
      if (!curObj->m_prefabLinkUUID.empty()) {
        return curObj;
      }

      if (nullptr != curObj->m_parent) {
        curObj = curObj->m_parent;
      }
      else {
        curObj = nullptr;
      }
    }

    return curObj;
  }

  void
  SceneObject::breakPrefabLink() {
    SceneObject* rootObj = this;

    while (nullptr != rootObj) {
      if (!rootObj->m_prefabLinkUUID.empty()) {
        break;
      }

      if (nullptr != rootObj->m_parent) {
        rootObj = rootObj->m_parent.get();
      }
      else {
        rootObj = nullptr;
      }
    }

    if (nullptr != rootObj) {
      rootObj->m_prefabLinkUUID = UUID::EMPTY;
      rootObj->m_prefabDiff = nullptr;
      PrefabUtility::clearPrefabIds(rootObj->getHandle(), true, false);
    }
  }

  bool
  SceneObject::hasFlag(uint32 flag) const {
    return (m_flags & flag) != 0;
  }

  void
  SceneObject::_setFlags(uint32 flags) {
    m_flags |= flags;

    for (auto& child : m_children) {
      child->_setFlags(flags);
    }
  }

  void
  SceneObject::_unsetFlags(uint32 flags) {
    m_flags &= ~flags;

    for (auto& child : m_children) {
      child->_unsetFlags(flags);
    }
  }

  void
  SceneObject::_instantiate(bool prefabOnly) {
    function<void(SceneObject*)> instantiateRecursive = [&](SceneObject* obj) {
      obj->m_flags &= ~SCENE_OBJECT_FLAGS::kDontInstantiate;

      if (nullptr == obj->m_parent) {
        g_sceneManager().registerNewSO(obj->m_thisHandle);
      }

      for (auto& component : obj->m_components) {
        component->_instantiate();
      }

      for (auto& child : obj->m_children) {
        if (!prefabOnly || child->m_prefabLinkUUID.empty()) {
          instantiateRecursive(child.get());
        }
      }
    };

    function<void(SceneObject*)> triggerEventsRecursive = [&](SceneObject* obj) {
      for (auto& component : obj->m_components) {
        g_sceneManager()._notifyComponentCreated(component, obj->getActive());
      }

      for (auto& child : obj->m_children) {
        if (!prefabOnly || child->m_prefabLinkUUID.empty()) {
          triggerEventsRecursive(child.get());
        }
      }
    };

    instantiateRecursive(this);
    triggerEventsRecursive(this);
  }

  /***************************************************************************/
  /**
   * Transforms
   */
  /***************************************************************************/

  void
  SceneObject::setTranslation(const Vector3& translation) {
    if (ObjectMobility::Movable == m_mobility) {
      m_localTransform.setTranslation(translation);
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::setRotation(const Quaternion& rotation) {
    if (ObjectMobility::Movable == m_mobility) {
      m_localTransform.setRotation(rotation);
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::setScale(const Vector3& scale) {
    if (ObjectMobility::Movable == m_mobility) {
      m_localTransform.setScale3D(scale);
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::setWorldPosition(const Vector3& position) {
    if (ObjectMobility::Movable != m_mobility) {
      return;
    }

    if (nullptr != m_parent) {
      m_localTransform.setWorldTranslation(position, m_parent->getTransform());
    }
    else {
      m_localTransform.setTranslation(position);
    }

    notifyTransformChanged(TransformChangedFlags::kTransform);
  }

  void
  SceneObject::setWorldRotation(const Quaternion& rotation) {
    if (m_mobility != ObjectMobility::Movable) {
      return;
    }

    if (nullptr != m_parent) {
      m_localTransform.setWorldRotation(rotation, m_parent->getTransform());
    }
    else {
      m_localTransform.setRotation(rotation);
    }

    notifyTransformChanged(TransformChangedFlags::kTransform);
  }

  void
  SceneObject::setWorldScale(const Vector3& scale) {
    if (m_mobility != ObjectMobility::Movable) {
      return;
    }

    if (nullptr != m_parent) {
      m_localTransform.setWorldScale(scale, m_parent->getTransform());
    }
    else {
      m_localTransform.setScale3D(scale);
    }

    notifyTransformChanged(TransformChangedFlags::kTransform);
  }

  const Transform&
  SceneObject::getTransform() const {
    if (!isCachedWorldTransformUpToDate()) {
      updateWorldTransform();
    }

    return m_worldTransform;
  }

  void
  SceneObject::lookAt(const Vector3& location, const Vector3& up) {
    const Transform& worldTfrm = getTransform();

    Vector3 forward = location - worldTfrm.getTranslation();

    Quaternion rotation = worldTfrm.getRotation();
    rotation.lookRotation(forward, up);
    setWorldRotation(rotation);
  }

  const Matrix4&
  SceneObject::getWorldMatrix() const {
    if (!isCachedWorldTransformUpToDate()) {
      updateWorldTransform();
    }
    return m_cachedWorldTransforms;
  }

  Matrix4
  SceneObject::getInvWorldMatrix() const {
    if (!isCachedWorldTransformUpToDate()) {
      updateWorldTransform();
    }
    Matrix4 worldToLocal = m_worldTransform.toInverseMatrixWithScale();
    return worldToLocal;
  }

  const Matrix4&
  SceneObject::getLocalMatrix() const {
    if (!isCachedLocalTransformUpToDate()) {
      updateLocalTransform();
    }
    return m_cachedLocalTransform;
  }

  void
  SceneObject::move(const Vector3& vec) {
    if (ObjectMobility::Movable == m_mobility) {
      m_localTransform.addToTranslation(vec);
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::moveRelative(const Vector3& vec) {
    if (ObjectMobility::Movable == m_mobility) {
      m_localTransform.setTranslation(m_localTransform.transformPositionNoScale(vec));
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::rotate(const Vector3& axis, const Radian& angle) {
    if (ObjectMobility::Movable == m_mobility) {
      Quaternion quatNorm(axis, angle.valueRadians());
      quatNorm.normalize();
      m_localTransform.setRotation(quatNorm * m_localTransform.getRotation());
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::rotate(const Quaternion& q) {
    if (ObjectMobility::Movable == m_mobility) {
      Quaternion quatNorm = q;
      quatNorm.normalize();
      m_localTransform.setRotation(quatNorm * m_localTransform.getRotation());
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::roll(const Radian& angle) {
    if (ObjectMobility::Movable == m_mobility) {
      Vector3 axis = m_localTransform.getRotation().getForwardVector();
      Quaternion quatNorm(axis, angle.valueRadians());
      quatNorm.normalize();
      m_localTransform.setRotation(quatNorm * m_localTransform.getRotation());
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::yaw(const Radian& angle) {
    if (ObjectMobility::Movable == m_mobility) {
      Vector3 axis = m_localTransform.getRotation().getUpVector();
      Quaternion quatNorm(axis, angle.valueRadians());
      quatNorm.normalize();
      m_localTransform.setRotation(quatNorm * m_localTransform.getRotation());
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::pitch(const Radian& angle) {
    if (ObjectMobility::Movable == m_mobility) {
      Vector3 axis = m_localTransform.getRotation().getRightVector();
      Quaternion quatNorm(axis, angle.valueRadians());
      quatNorm.normalize();
      m_localTransform.setRotation(quatNorm * m_localTransform.getRotation());
      notifyTransformChanged(TransformChangedFlags::kTransform);
    }
  }

  void
  SceneObject::setForward(const Vector3& forwardDir) {
    const Transform& worldTfrm = getTransform();

    Quaternion currentRotation = worldTfrm.getRotation();
    currentRotation.lookRotation(forwardDir);

    setWorldRotation(currentRotation);
  }

  void
  SceneObject::updateTransformsIfDirty() {
    if (!isCachedLocalTransformUpToDate()) {
      updateLocalTransform();
    }

    if (!isCachedWorldTransformUpToDate()) {
      updateWorldTransform();
    }
  }

  void
  SceneObject::notifyTransformChanged(TransformChangedFlags flags) const {
    //If object is immovable, don't send transform changed events nor mark the
    //transform dirty
    TransformChangedFlags componentFlags = flags;
    if (ObjectMobility::Movable == m_mobility) {
      componentFlags = static_cast<TransformChangedFlags>
                        (componentFlags & ~TRANSFORM_CHANGED_FLAGS::kTransform);
    }
    else {
      m_dirtyFlags |= DIRTY_FLAGS::kLocalTransformDirty | DIRTY_FLAGS::kWorldTransformDirty;
      ++m_dirtyHash;
    }

    //Only send component flags if we haven't removed them all
    if (0 != componentFlags) {
      for (auto& entry : m_components) {
        if (entry->supportsNotify(flags))
        {
          bool alwaysRun = entry->hasFlag(ComponentFlag::kAlwaysRun);
          if (alwaysRun || g_sceneManager().isRunning())
            entry->onTransformChanged(componentFlags);
        }
      }
    }

    // Mobility flag is only relevant for this scene object
    flags = static_cast<TransformChangedFlags>(flags & ~TransformChangedFlags::kMobility);
    if (0 != flags) {
      for (auto& entry : m_children) {
        entry->notifyTransformChanged(flags);
      }
    }
  }

  void
  SceneObject::updateWorldTransform() const {
    m_worldTransform = m_localTransform;

    //Don't allow movement from parent when not movable
    if (nullptr != m_parent && ObjectMobility::Movable == m_mobility) {
      m_worldTransform.makeWorld(m_parent->getTransform());
      m_cachedWorldTransforms = m_worldTransform.toMatrixWithScale();
    }
    else {
      m_cachedWorldTransforms = getLocalMatrix();
    }

    m_dirtyFlags &= ~DIRTY_FLAGS::kWorldTransformDirty;
  }

  void
  SceneObject::updateLocalTransform() const {
    m_cachedLocalTransform = m_localTransform.toMatrixWithScale();
    m_dirtyFlags &= ~DIRTY_FLAGS::kLocalTransformDirty;
  }

  /***************************************************************************/
  /**
   * Hierarchy
   */
  /***************************************************************************/

  void SceneObject::setParent(const HSceneObject& parent, bool keepWorldTransform)
  {
    if (parent.isDestroyed()) {
      return;
    }

#if GE_IS_ENGINEEDITOR
    UUID originalPrefab = getPrefabLink();
#endif

    if (ObjectMobility::Movable != m_mobility) {
      keepWorldTransform = true;
    }

    _setParent(parent, keepWorldTransform);

#if GE_IS_ENGINEEDITOR
    if (g_coreApplication().isEditor()) {
      UUID newPrefab = getPrefabLink();
      if (originalPrefab != newPrefab) {
        PrefabUtility::clearPrefabIds(mThisHandle);
      }
    }
#endif
  }

  void
  SceneObject::_setParent(const HSceneObject& parent, bool keepWorldTransform) {
    if (parent == m_thisHandle) {
      return;
    }

    if (nullptr == m_parent || parent != m_parent ) {
      Transform worldTfrm;

      //Make sure the object keeps its world coordinates
      if (keepWorldTransform) {
        worldTfrm = getTransform();
      }

      if (nullptr != m_parent) {
        m_parent->removeChild(m_thisHandle);
      }

      if (nullptr != parent) {
        parent->addChild(m_thisHandle);
      }

      m_parent = parent;

      if (keepWorldTransform) {
        m_localTransform = worldTfrm;

        if (nullptr != m_parent) {
          m_localTransform.makeLocal(m_parent->getTransform());
        }
      }

      notifyTransformChanged(static_cast<TransformChangedFlags>(
        TransformChangedFlags::kParent | TransformChangedFlags::kTransform)
      );
    }
  }

  HSceneObject
  SceneObject::getChild(uint32 idx) const {
    if (idx >= m_children.size()) {
      GE_EXCEPT(InternalErrorException, "Child index out of range.");
    }

    return m_children[idx];
  }

  int32
  SceneObject::indexOfChild(const HSceneObject& child) const {
    for (SIZE_T i = 0; i < m_children.size(); ++i) {
      if (child == m_children[i]) {
        return static_cast<int32>(i);
      }
    }

    return -1;
  }

  void
  SceneObject::addChild(const HSceneObject& object) {
    m_children.push_back(object);
    object->_setFlags(m_flags);
  }

  void
  SceneObject::removeChild(const HSceneObject& object) {
    auto result = find(m_children.begin(), m_children.end(), object);

    if (m_children.end() != result) {
      m_children.erase(result);
    }
    else {
      GE_EXCEPT(InternalErrorException,
                "Trying to remove a child but it's not a child of the"
                " transform.");
    }
  }

  HSceneObject
  SceneObject::findPath(const String& path) const {
    if (path.empty()) {
      return HSceneObject();
    }

    String trimmedPath = path;
    StringUtil::trim(trimmedPath, "/");

    Vector<String> entries = StringUtil::split(trimmedPath, "/");

    //Find scene object referenced by the path
    HSceneObject so = getHandle();

    for (String entry : entries) {
      if (entry.empty()) {
        continue;
      }

      //This character signifies not-a-scene-object. This is allowed to support
      //paths used by the scripting system (which can point to properties of
      //components on scene objects).
      if ('!' != entry[0]) {
        break;
      }

      String childName = entry.substr(1, entry.size() - 1);
      so = so->findChild(childName);

      if (nullptr == so) {
        break;
      }
    }

    return so;
  }

  HSceneObject
  SceneObject::findChild(const String& name, bool recursive) {
    for (auto& child : m_children) {
      if (child->getName() == name) {
        return child;
      }
    }

    if (recursive) {
      for (auto& child : m_children) {
        HSceneObject foundObject = child->findChild(name, true);
        if (nullptr != foundObject) {
          return foundObject;
        }
      }
    }

    return HSceneObject();
  }

  Vector<HSceneObject>
  SceneObject::findChildren(const String& name, bool recursive) {
    function<void(const HSceneObject&, Vector<HSceneObject>&)>
      findChildrenInternal = [&](const HSceneObject& so, Vector<HSceneObject>& output) {
        for (auto& child : so->m_children) {
          if (child->getName() == name) {
            output.push_back(child);
          }
        }

        if (recursive) {
          for (auto& child : so->m_children) {
            findChildrenInternal(child, output);
          }
        }
    };

    Vector<HSceneObject> output;
    findChildrenInternal(m_thisHandle, output);

    return output;
  }

  void
  SceneObject::setActive(bool active) {
    m_activeSelf = active;
    setActiveHierarchy(active);
  }

  void
  SceneObject::setActiveHierarchy(bool active, bool triggerEvents) {
    bool activeHierarchy = active && m_activeSelf;

    if (m_activeHierarchy != activeHierarchy) {
      m_activeHierarchy = activeHierarchy;

      if (triggerEvents) {
        if (activeHierarchy) {
          for (auto& component : m_components) {
            g_sceneManager()._notifyComponentActivated(component, triggerEvents);
          }
        }
        else {
          for (auto& component : m_components) {
            g_sceneManager()._notifyComponentDeactivated(component, triggerEvents);
          }
        }
      }
    }

    for (const auto& child : m_children) {
      child->setActiveHierarchy(m_activeHierarchy, triggerEvents);
    }
  }

  bool
  SceneObject::getActive(bool self) const {
    if (self) {
      return m_activeSelf;
    }

    return m_activeHierarchy;
  }

  void
  SceneObject::setMobility(ObjectMobility mobility) {
    if (m_mobility != mobility) {
      m_mobility = mobility;

      //If mobility changed to movable, update both the mobility flag and
      //transform, otherwise just mobility
      if (ObjectMobility::Movable == m_mobility) {
        notifyTransformChanged(static_cast<TransformChangedFlags>(
                                                       TransformChangedFlags::kTransform |
                                                       TransformChangedFlags::kMobility));
      }
      else {
        notifyTransformChanged(TransformChangedFlags::kMobility);
      }
    }
  }

  HSceneObject
  SceneObject::clone(bool instantiate) {
    bool isInstantiated = !hasFlag(SCENE_OBJECT_FLAGS::kDontInstantiate);

    if (!instantiate) {
      _setFlags(SCENE_OBJECT_FLAGS::kDontInstantiate);
    }
    else {
      _unsetFlags(SCENE_OBJECT_FLAGS::kDontInstantiate);
    }

    uint32 bufferSize = 0;

    MemorySerializer serializer;
    uint8* buffer = serializer.encode(this, bufferSize,
                                      reinterpret_cast<(void*(*)(size_t))>(&ge_alloc));

    CoreSerializationContext serzContext;
    serzContext.goState = ge_shared_ptr_new<GameObjectDeserializationState>
                            (GODM::kUseNewIds | GODM::kRestoreExternal);

    auto cloneObj = static_pointer_cast<SceneObject>(serializer.decode(buffer,
                                                                       bufferSize,
                                                                       &serzContext));

    ge_free(buffer);

    if (isInstantiated) {
      _unsetFlags(SCENE_OBJECT_FLAGS::kDontInstantiate);
    }
    else {
      _setFlags(SCENE_OBJECT_FLAGS::kDontInstantiate);
    }

    return cloneObj->m_thisHandle;
  }

  HComponent
  SceneObject::getComponent(RTTITypeBase* type) const {
    for (auto& entry : m_components) {
      if (entry->getRTTI()->isDerivedFrom(type)) {
        return entry;
      }
    }

    return HComponent();
  }

  void
  SceneObject::destroyComponent(const HComponent component, bool immediate) {
    if (nullptr == component) {
      LOGDBG("Trying to remove a null component");
      return;
    }

    auto iter = std::find(m_components.begin(), m_components.end(), component);

    if (m_components.end() != iter) {
      (*iter)->_setIsDestroyed();

      if (isInstantiated()) {
        g_sceneManager()._notifyComponentDestroyed(*iter, immediate);
      }

      (*iter)->destroyInternal(*iter, immediate);
      m_components.erase(iter);
    }
    else {
      LOGDBG("Trying to remove a component that doesn't exist on this SceneObject.");
    }
  }

  void
  SceneObject::destroyComponent(Component* component, bool immediate) {
    auto iterFind = find_if(m_components.begin(),
                            m_components.end(),
    [component](const HComponent& x)
    {
      if (x.isDestroyed()) {
        return false;
      }
      return x._getHandleData()->m_ptr->object.get() == component;
    });

    if (m_components.end() != iterFind) {
      destroyComponent(*iterFind, immediate);
    }
  }

  HComponent
  SceneObject::addComponent(uint32 typeId) {
    SPtr<IReflectable> newObj = rtti_create(typeId);

    if (!rtti_is_subclass<Component>(newObj.get())) {
      LOGERR("Specified type is not a valid Component.");
      return HComponent();
    }

    auto componentPtr = static_pointer_cast<Component>(newObj);

    //Clean up the self-reference assigned by the RTTI system
    componentPtr->m_rttiData = nullptr;

    HComponent newComponent = static_object_cast<Component>
      (GameObjectManager::instance().registerObject(componentPtr));
    newComponent->m_parent = m_thisHandle;

    addAndInitializeComponent(newComponent);
    return newComponent;
  }

  void
  SceneObject::addComponentInternal(const SPtr<Component> component) {
    HComponent newComponent = static_object_cast<Component>(
      GameObjectManager::instance().getObject(component->getInstanceId()));
    newComponent->m_parent = m_thisHandle;
    newComponent->m_thisHandle = newComponent;
    m_components.push_back(newComponent);
  }

  void
  SceneObject::addAndInitializeComponent(const HComponent& component) {
    component->m_thisHandle = component;
    m_components.push_back(component);

    if (isInstantiated()) {
      component->_instantiate();
      g_sceneManager()._notifyComponentCreated(component, getActive());
    }
  }

  void
  SceneObject::addAndInitializeComponent(const SPtr<Component> component) {
    GameObjectHandle<Component>
      newComponent = GameObjectManager::instance().getObject(component->getInstanceId());
    newComponent->m_parent = m_thisHandle;
    addAndInitializeComponent(newComponent);
  }

  RTTITypeBase*
  SceneObject::getRTTIStatic() {
    return SceneObjectRTTI::instance();
  }

  RTTITypeBase*
  SceneObject::getRTTI() const {
    return SceneObject::getRTTIStatic();
  }
}
