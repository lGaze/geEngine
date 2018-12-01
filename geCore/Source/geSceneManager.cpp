/*****************************************************************************/
/**
 * @file    geSceneManager.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geSceneManager.h"
#include "geSceneObject.h"
#include "geComponent.h"
#include "geGameObjectManager.h"
#include "geSceneActor.h"

/*
#include "geRenderable.h"
#include "geCamera.h"
#include "geLight.h"
#include "geLightProbeVolume.h"
*/
#include "geViewport.h"
#include "geRenderTarget.h"

namespace geEngineSDK {
  using std::find_if;
  using std::bind;
  using std::swap;

  namespace LIST_TYPE {
    enum E {
      kActiveList = 0,
      kInactiveList = 1,
      kUninitializedList = 2
    };
  }

  struct ScopeToggle
  {
    ScopeToggle(bool& val)
      : val(val) {
      val = true;
    }

    ~ScopeToggle() {
      val = false;
    }

   private:
    bool& val;
  };

  SceneManager::SceneManager()
    : m_rootNode(SceneObject::createInternal("SceneRoot"))
  {}

  SceneManager::~SceneManager() {
    if (nullptr != m_rootNode && !m_rootNode.isDestroyed()) {
      m_rootNode->destroy(true);
    }
  }

  void
  SceneManager::clearScene(bool forceAll) {
    uint32 numChildren = m_rootNode->getNumChildren();

    uint32 curIdx = 0;
    for (uint32 i = 0; i < numChildren; ++i) {
      HSceneObject child = m_rootNode->getChild(curIdx);

      if (forceAll || !child->hasFlag(SCENE_OBJECT_FLAGS::kPersistent)) {
        child->destroy();
      }
      else {
        ++curIdx;
      }
    }

    GameObjectManager::instance().destroyQueuedObjects();

    HSceneObject newRoot = SceneObject::createInternal("SceneRoot");
    setRootNode(newRoot);
  }

  void
  SceneManager::setRootNode(const HSceneObject& root) {
    if (nullptr == root) {
      return;
    }

    HSceneObject oldRoot = m_rootNode;

    uint32 numChildren = oldRoot->getNumChildren();
    
    //Make sure to keep persistent objects
    ge_frame_mark();
    {
      FrameVector<HSceneObject> toRemove;
      for (uint32 i = 0; i < numChildren; ++i) {
        HSceneObject child = oldRoot->getChild(i);

        if (child->hasFlag(SCENE_OBJECT_FLAGS::kPersistent)) {
          toRemove.push_back(child);
        }
      }

      for (auto& entry : toRemove) {
        entry->setParent(root, false);
      }
    }
    ge_frame_clear();

    m_rootNode = root;
    m_rootNode->_setParent(HSceneObject());
    oldRoot->destroy();
  }

  void
  SceneManager::_bindActor(const SPtr<SceneActor>& actor, const HSceneObject& so) {
    m_boundActors[actor.get()] = BoundActorData(actor, so);
  }

  void
  SceneManager::_unbindActor(const SPtr<SceneActor>& actor) {
    m_boundActors.erase(actor.get());
  }

  HSceneObject
  SceneManager::_getActorSO(const SPtr<SceneActor>& actor) const {
    auto iterFind = m_boundActors.find(actor.get());
    if (iterFind != m_boundActors.end()) {
      return iterFind->second.so;
    }
    return HSceneObject();
  }

  void
  SceneManager::_registerCamera(const SPtr<Camera>& camera) {
    m_cameras[camera.get()] = camera;
  }

  void
  SceneManager::_unregisterCamera(const SPtr<Camera>& camera) {
    m_cameras.erase(camera.get());

    auto iterFind = find_if(m_mainCameras.begin(), m_mainCameras.end(),
    [&](const SPtr<Camera>& entry) {
      return entry == camera;
    });

    if (m_mainCameras.end() != iterFind) {
      m_mainCameras.erase(iterFind);
    }
  }

  void
  SceneManager::_notifyMainCameraStateChanged(const SPtr<Camera>& /*camera*/) {
    /* TODO: Remove commentary when the renderer is ready
    auto iterFind = find_if(m_mainCameras.begin(), m_mainCameras.end(),
    [&](const SPtr<Camera>& entry) {
      return entry == camera;
    });

    SPtr<Viewport> viewport = camera->getViewport();
    if (camera->isMain()) {
      if (m_mainCameras.end() == iterFind) {
        m_mainCameras.push_back(m_cameras[camera.get()]);
      }

      viewport->setTarget(m_mainRT);
    }
    else {
      if (m_mainCameras.end() != iterFind) {
        m_mainCameras.erase(iterFind);
      }

      if (viewport->getTarget() == m_mainRT) {
        viewport->setTarget(nullptr);
      }
    }
    */
  }

  void
  SceneManager::_updateCoreObjectTransforms() {
    for (auto& entry : m_boundActors) {
      entry.second.actor->_updateState(*entry.second.so);
    }
  }

  SPtr<Camera>
  SceneManager::getMainCamera() const {
    if (!m_mainCameras.empty()) {
      return m_mainCameras[0];
    }
    return nullptr;
  }

  void
  SceneManager::setMainRenderTarget(const SPtr<RenderTarget>& /*rt*/) {
    /* TODO: Remove commentary when the renderer is ready
    if (m_mainRT == rt) {
      return;
    }

    m_mainRTResizedConn.disconnect();

    if (nullptr != rt) {
      m_mainRTResizedConn =
        rt->onResized.connect(bind(&SceneManager::onMainRenderTargetResized, this));
    }
    m_mainRT = rt;

    float aspect = 1.0f;
    if (nullptr != rt) {
      auto& rtProps = rt->getProperties();
      aspect = rtProps.width / static_cast<float>(rtProps.height);
    }

    for (auto& entry : m_mainCameras) {
      entry->getViewport()->setTarget(rt);
      entry->setAspectRatio(aspect);
    }
    */
  }

  void
  SceneManager::setComponentState(COMPONENT_STATE::E state) {
    if (m_disableStateChange) {
      LOGWRN("Component state cannot be changed from the calling locating. "
             "Are you calling it from Component callbacks?");
      return;
    }

    if (state == m_componentState) {
      return;
    }

    COMPONENT_STATE::E oldState = m_componentState;

    //Make sure to change the state before calling any callbacks, so callbacks
    //can query the state
    m_componentState = state;

    // Make sure the per-state lists are up-to-date
    processStateChanges();

    ScopeToggle toggle(m_disableStateChange);

    //Wake up all components with onInitialize/onEnable events if moving to
    //running or paused state
    if (COMPONENT_STATE::kRunning == state || COMPONENT_STATE::kPaused == state) {
      if (COMPONENT_STATE::kStopped == oldState) {
        //Disable, and then re-enable components that have an AlwaysRun flag
        for (auto& entry : m_activeComponents) {
          if (entry->sceneObject()->getActive()) {
            entry->onDisabled();
            entry->onEnabled();
          }
        }

        //Process any state changes queued by the component callbacks
        processStateChanges();

        //Trigger enable on all components that don't have AlwaysRun flag
        //(at this point those will be all inactive components that have
        //active scene object parents)
        for (auto& entry : m_inactiveComponents) {
          if (entry->sceneObject()->getActive()) {
            entry->onEnabled();
            if (state == COMPONENT_STATE::kRunning) {
              m_stateChanges.emplace_back(entry, COMPONENT_STATE_EVENT::kActivated);
            }
          }
        }

        //Process any state changes queued by the component callbacks
        processStateChanges();

        //Initialize and enable uninitialized components
        for (auto& entry : m_uninitializedComponents) {
          entry->onInitialized();

          if (entry->sceneObject()->getActive()) {
            entry->onEnabled();
            m_stateChanges.emplace_back(entry, COMPONENT_STATE_EVENT::kActivated);
          }
          else {
            m_stateChanges.emplace_back(entry, COMPONENT_STATE_EVENT::kDeactivated);
          }
        }

        //Process any state changes queued by the component callbacks
        processStateChanges();
      }
    }

    //Stop updates on all active components
    if (COMPONENT_STATE::kPaused == state || COMPONENT_STATE::kStopped == state) {
      //Trigger onDisable events if stopping
      if (COMPONENT_STATE::kStopped == state) {
        for (const auto& component : m_activeComponents) {
          bool alwaysRun = component->hasFlag(ComponentFlag::kAlwaysRun);
          component->onDisabled();
          if (alwaysRun) {
            component->onEnabled();
          }
        }
      }

      //Move from active to inactive list
      for (int32 i = 0; i < static_cast<int32>(m_activeComponents.size()); ++i) {
        const HComponent component = m_activeComponents[i];

        bool alwaysRun = component->hasFlag(ComponentFlag::kAlwaysRun);
        if (alwaysRun) {
          continue;
        }

        removeFromStateList(component);
        addToStateList(component, LIST_TYPE::kInactiveList);
        --i;
      }
    }
  }

  void
  SceneManager::_notifyComponentCreated(const HComponent& component, bool parentActive) {
    //NOTE: This method must remain reentrant (in case the callbacks below
    //trigger component state changes)

    //Queue the change before any callbacks trigger, as the callbacks could
    //trigger their own changes and they should be in order
    m_stateChanges.emplace_back(component, COMPONENT_STATE_EVENT::kCreated);
    ScopeToggle toggle(m_disableStateChange);

    component->onCreated();

    const bool alwaysRun = component->hasFlag(COMPONENT_FLAG::kAlwaysRun);
    if (alwaysRun || m_componentState != COMPONENT_STATE::kStopped) {
      component->onInitialized();

      if (parentActive) {
        component->onEnabled();
      }
    }    
  }

  void
  SceneManager::_notifyComponentActivated(const HComponent& component, bool triggerEvent) {
    //NOTE: This method must remain reentrant (in case the callbacks below
    //trigger component state changes)

    //Queue the change before any callbacks trigger, as the callbacks could
    //trigger their own changes and they should be in order
    m_stateChanges.emplace_back(component, COMPONENT_STATE_EVENT::kActivated);
    ScopeToggle toggle(m_disableStateChange);

    const bool alwaysRun = component->hasFlag(COMPONENT_FLAG::kAlwaysRun);
    if (alwaysRun || m_componentState != COMPONENT_STATE::kStopped) {
      if (triggerEvent) {
        component->onEnabled();
      }
    }
  }

  void
  SceneManager::_notifyComponentDeactivated(const HComponent& component, bool triggerEvent) {
    //NOTE: This method must remain reentrant (in case the callbacks below
    //trigger component state changes)

    //Queue the change before any callbacks trigger, as the callbacks could
    //trigger their own changes and they should be in order
    m_stateChanges.emplace_back(component, COMPONENT_STATE_EVENT::kDeactivated);
    ScopeToggle toggle(m_disableStateChange);

    const bool alwaysRun = component->hasFlag(COMPONENT_FLAG::kAlwaysRun);
    if (alwaysRun || m_componentState != COMPONENT_STATE::kStopped) {
      if (triggerEvent) {
        component->onDisabled();
      }
    }
  }

  void
  SceneManager::_notifyComponentDestroyed(const HComponent& component, bool immediate) {
    //NOTE: This method must remain reentrant (in case the callbacks below
    //trigger component state changes)

    //Queue the change before any callbacks trigger, as the callbacks could
    //trigger their own changes and they should be in order
    if (!immediate) {
      //If destruction is immediate no point in queuing state change since it
      //will be ignored anyway
      m_stateChanges.emplace_back(component, COMPONENT_STATE_EVENT::kDestroyed);
    }

    ScopeToggle toggle(m_disableStateChange);

    const bool alwaysRun = component->hasFlag(COMPONENT_FLAG::kAlwaysRun);
    const bool isEnabled = component->sceneObject()->getActive() &&
                             (alwaysRun || COMPONENT_STATE::kStopped != m_componentState);

    if (isEnabled) {
      component->onDisabled();
    }

    component->onDestroyed();

    if (immediate) {
      //Since the state change wasn't queued, remove the component from the
      //list right away. Its expected the caller knows what is he doing.
      uint32 existingListType;
      uint32 existingIdx;
      decodeComponentId(component->getSceneManagerId(), existingIdx, existingListType);

      if (existingListType != 0) {
        removeFromStateList(component);
      }
    }
  }

  void
  SceneManager::addToStateList(const HComponent& component, uint32 listType) {
    if (0 == listType) {
      return;
    }

    Vector<HComponent>& list = *m_componentsPerState[listType - 1];

    const auto idx = static_cast<uint32>(list.size());
    list.push_back(component);
    component->setSceneManagerId(encodeComponentId(idx, listType));
  }

  void
  SceneManager::removeFromStateList(const HComponent& component) {
    uint32 listType;
    uint32 idx;
    decodeComponentId(component->getSceneManagerId(), idx, listType);

    if (0 == listType) {
      return;
    }

    Vector<HComponent>& list = *m_componentsPerState[listType - 1];

    uint32 lastIdx;
    decodeComponentId(list.back()->getSceneManagerId(), lastIdx, listType);

    GE_ASSERT(list[idx] == component);

    if (idx != lastIdx) {
      swap(list[idx], list[lastIdx]);
      list[idx]->setSceneManagerId(encodeComponentId(idx, listType));
    }

    list.erase(list.end() - 1);
  }

  void
  SceneManager::processStateChanges() {
    const bool isStopped = m_componentState == COMPONENT_STATE::kStopped;

    for (auto& entry : m_stateChanges) {
      const HComponent& component = entry.obj;
      if (component.isDestroyed(false)) {
        continue;
      }

      const bool alwaysRun = component->hasFlag(COMPONENT_FLAG::kAlwaysRun);
      const bool isActive = component->so()->getActive();

      uint32 listType = 0;
      switch (entry.type)
      {
        case COMPONENT_STATE_EVENT::kCreated:
          if (alwaysRun || !isStopped) {
            listType = isActive ? LIST_TYPE::kActiveList : LIST_TYPE::kInactiveList;
          }
          else {
            listType = LIST_TYPE::kUninitializedList;
          }
          break;
        case COMPONENT_STATE_EVENT::kActivated:
        case COMPONENT_STATE_EVENT::kDeactivated:
          if (alwaysRun || !isStopped) {
            listType = isActive ? LIST_TYPE::kActiveList : LIST_TYPE::kInactiveList;
          }
          break;
        case COMPONENT_STATE_EVENT::kDestroyed:
          listType = 0;
          break;
        default:
          break;
      }

      uint32 existingListType;
      uint32 existingIdx;
      decodeComponentId(component->getSceneManagerId(), existingIdx, existingListType);

      if (existingListType == listType) {
        continue;
      }

      if (existingListType != 0) {
        removeFromStateList(component);
      }

      addToStateList(component, listType);
    }

    m_stateChanges.clear();
  }

  uint32
  SceneManager::encodeComponentId(uint32 idx, uint32 type) {
    GE_ASSERT(idx <= (0x3FFFFFFF));
    return (type << 30) | idx;
  }

  void
  SceneManager::decodeComponentId(uint32 id, uint32& idx, uint32& type) {
    idx = id & 0x3FFFFFFF;
    type = id >> 30;
  }

  bool
  SceneManager::isComponentOfType(const HComponent& component, uint32 rttiId) {
    return component->getRTTI()->getRTTIId() == rttiId;
  }

  void
  SceneManager::_update() {
    processStateChanges();

    //NOTE: Eventually perform updates based on component types and / or on
    //component priority. Right now we just iterate in an undefined order, but
    //it wouldn't be hard to change it.
    ScopeToggle toggle(m_disableStateChange);
    for (auto& entry : m_activeComponents) {
      entry->update();
    }
    GameObjectManager::instance().destroyQueuedObjects();
  }

  void
  SceneManager::_fixedUpdate() {
    processStateChanges();
    
    ScopeToggle toggle(m_disableStateChange);
    for (auto& entry : m_activeComponents) {
      entry->fixedUpdate();
    }
  }

  void
  SceneManager::registerNewSO(const HSceneObject& node) {
    if (m_rootNode) {
      node->setParent(m_rootNode);
    }
  }

  void
  SceneManager::onMainRenderTargetResized() {
    /* TODO: Remove commentary when the renderer is ready
    auto& rtProps = m_mainRT->getProperties();
    float aspect = rtProps.width / static_cast<float>(rtProps.height);

    for (auto& entry : m_mainCameras) {
      entry->setAspectRatio(aspect);
    }
    */
  }

  SceneManager&
  g_sceneManager() {
    return SceneManager::instance();
  }
}
