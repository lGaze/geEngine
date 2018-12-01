/*****************************************************************************/
/**
 * @file    gePrefabDiff.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/04
 * @brief   Contains differences between two components of the same type.
 *
 * Contains differences between two components of the same type.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrefabDiff.h"
#include "gePrefabDiffRTTI.h"
#include "geSceneObject.h"
#include "geSceneManager.h"
#include "geUtility.h"

#include <geMemorySerializer.h>
#include <geBinarySerializer.h>
#include <geBinaryDiff.h>

namespace geEngineSDK {
  using std::static_pointer_cast;

  RTTITypeBase*
  PrefabComponentDiff::getRTTIStatic() {
    return PrefabComponentDiffRTTI::instance();
  }

  RTTITypeBase*
  PrefabComponentDiff::getRTTI() const {
    return PrefabComponentDiff::getRTTIStatic();
  }

  RTTITypeBase*
  PrefabObjectDiff::getRTTIStatic() {
    return PrefabObjectDiffRTTI::instance();
  }

  RTTITypeBase*
  PrefabObjectDiff::getRTTI() const {
    return PrefabObjectDiff::getRTTIStatic();
  }

  SPtr<PrefabDiff>
  PrefabDiff::create(const HSceneObject& prefab, const HSceneObject& instance) {
    if (prefab->m_prefabLinkUUID != instance->m_prefabLinkUUID) {
      return nullptr;
    }

    //NOTE: If this method is called multiple times in a row then renaming all
    //objects every time is redundant, it would be more efficient to do it once
    //outside of this method. I'm keeping it this way for simplicity for now.
    //Rename instance objects so they share the same IDs as the prefab objects
    //(if they link IDs match). This allows game object handle diff to work
    //properly, because otherwise handles that point to same objects would be
    //marked as different because the instance IDs of the two objects don't
    //match (since one is in prefab and one in instance).
    Vector<RenamedGameObject> renamedObjects;
    renameInstanceIds(prefab, instance, renamedObjects);

    SPtr<PrefabDiff> output = ge_shared_ptr_new<PrefabDiff>();
    output->m_root = generateDiff(prefab, instance);

    restoreInstanceIds(renamedObjects);

    return output;
  }

  void
  PrefabDiff::apply(const HSceneObject& object) {
    if (nullptr == m_root) {
      return;
    }

    CoreSerializationContext serzContext;
    serzContext.goState = ge_shared_ptr_new<GameObjectDeserializationState>
                            (GODM::kUseNewIds | GODM::kRestoreExternal);
    serzContext.goDeserializationActive = true;

    applyDiff(m_root, object, &serzContext);

    serzContext.goState->resolve();
  }

  void
  PrefabDiff::applyDiff(const SPtr<PrefabObjectDiff>& diff,
                        const HSceneObject& object,
                        SerializationContext* context) {
    if ((diff->soFlags & static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kName)) != 0) {
      object->setName(diff->name);
    }

    if ((diff->soFlags & static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kTranslation)) != 0) {
      object->setTranslation(diff->translation);
    }

    if ((diff->soFlags & static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kRotation)) != 0) {
      object->setRotation(diff->rotation);
    }

    if ((diff->soFlags & static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kScale)) != 0) {
      object->setScale(diff->scale);
    }

    if ((diff->soFlags & static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kActive)) != 0) {
      object->setActive(diff->isActive);
    }

    //NOTE: It is important to remove objects and components first, before
    //adding them.
    //Some systems rely on the fact that applyDiff added components / objects
    //are always at the end.
    const Vector<HComponent>& components = object->getComponents();
    for (auto& removedId : diff->removedComponents) {
      for (auto& component : components) {
        if (removedId == component->getLinkId()) {
          component->destroy();
          break;
        }
      }
    }

    for (auto& removedId : diff->removedChildren) {
      uint32 childCount = object->getNumChildren();
      for (uint32 i = 0; i < childCount; ++i) {
        HSceneObject child = object->getChild(i);
        if (removedId == child->getLinkId()) {
          child->destroy();
          break;
        }
      }
    }

    for (auto& addedComponentData : diff->addedComponents) {
      auto component = static_pointer_cast<Component>(addedComponentData->decode(context));
      object->addAndInitializeComponent(component);
    }

    for (auto& addedChildData : diff->addedChildren) {
      auto sceneObject = static_pointer_cast<SceneObject>(addedChildData->decode(context));
      sceneObject->setParent(object);

      if (object->isInstantiated()) {
        sceneObject->_instantiate();
      }
    }

    for (auto& componentDiff : diff->componentDiffs) {
      for (auto& component : components) {
        if (static_cast<int32>(component->getLinkId()) == componentDiff->id) {
          IDiff& diffHandler = component->getRTTI()->getDiffHandler();
          diffHandler.applyDiff(component.getInternalPtr(), componentDiff->data, context);
          break;
        }
      }
    }

    for (auto& childDiff : diff->childDiffs) {
      uint32 childCount = object->getNumChildren();
      for (uint32 i = 0; i < childCount; ++i) {
        HSceneObject child = object->getChild(i);
        if (child->getLinkId() == childDiff->id) {
          applyDiff(childDiff, child, context);
          break;
        }
      }
    }
  }

  SPtr<PrefabObjectDiff>
  PrefabDiff::generateDiff(const HSceneObject& prefab, const HSceneObject& instance) {
    SPtr<PrefabObjectDiff> output;

    if (prefab->getName() != instance->getName()) {
      if (nullptr == output) {
        output = ge_shared_ptr_new<PrefabObjectDiff>();
      }

      output->name = instance->getName();
      output->soFlags |= static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kName);
    }

    const Transform& prefabTfrm = prefab->getLocalTransform();
    const Transform& instanceTfrm = instance->getLocalTransform();
    if (prefabTfrm.getTranslation() != instanceTfrm.getTranslation()) {
      if (nullptr == output) {
        output = ge_shared_ptr_new<PrefabObjectDiff>();
      }

      output->translation = instanceTfrm.getTranslation();
      output->soFlags |= static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kTranslation);
    }

    if (prefabTfrm.getRotation() != instanceTfrm.getRotation()) {
      if (nullptr == output) {
        output = ge_shared_ptr_new<PrefabObjectDiff>();
      }

      output->rotation = instanceTfrm.getRotation();
      output->soFlags |= static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kRotation);
    }

    if (prefabTfrm.getScale3D() != instanceTfrm.getScale3D())
    {
      if (nullptr == output) {
        output = ge_shared_ptr_new<PrefabObjectDiff>();
      }

      output->scale = instanceTfrm.getScale3D();
      output->soFlags |= static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kScale);
    }

    if (prefab->getActive() != instance->getActive()) {
      if (nullptr == output) {
        output = ge_shared_ptr_new<PrefabObjectDiff>();
      }

      output->isActive = instance->getActive();
      output->soFlags |= static_cast<uint32>(SCENE_OBJECT_DIFF_FLAGS::kActive);
    }

    uint32 prefabChildCount = prefab->getNumChildren();
    uint32 instanceChildCount = instance->getNumChildren();

    //Find modified and removed children
    for (uint32 i = 0; i < prefabChildCount; ++i) {
      HSceneObject prefabChild = prefab->getChild(i);

      SPtr<PrefabObjectDiff> childDiff;
      bool foundMatching = false;
      for (uint32 j = 0; j < instanceChildCount; ++j) {
        HSceneObject instanceChild = instance->getChild(j);

        if (prefabChild->getLinkId() == instanceChild->getLinkId()) {
          if (instanceChild->m_prefabLinkUUID.empty()) {
            childDiff = generateDiff(prefabChild, instanceChild);
          }

          foundMatching = true;
          break;
        }
      }

      if (foundMatching) {
        if (nullptr != childDiff) {
          if (nullptr == output)
            output = ge_shared_ptr_new<PrefabObjectDiff>();

          output->childDiffs.push_back(childDiff);
        }
      }
      else {
        if (nullptr == output) {
          output = ge_shared_ptr_new<PrefabObjectDiff>();
        }

        output->removedChildren.push_back(prefabChild->getLinkId());
      }
    }

    //Find added children
    for (uint32 i = 0; i < instanceChildCount; ++i) {
      HSceneObject instanceChild = instance->getChild(i);

      if (instanceChild->hasFlag(SCENE_OBJECT_FLAGS::kDontSave)) {
        continue;
      }

      bool foundMatching = false;
      if (instanceChild->getLinkId() != NumLimit::MAX_UINT32) {
        for (uint32 j = 0; j < prefabChildCount; ++j) {
          HSceneObject prefabChild = prefab->getChild(j);

          if (prefabChild->getLinkId() == instanceChild->getLinkId()) {
            foundMatching = true;
            break;
          }
        }
      }

      if (!foundMatching) {
        SPtr<SerializedObject> obj = SerializedObject::create(*instanceChild);

        if (nullptr == output) {
          output = ge_shared_ptr_new<PrefabObjectDiff>();
        }

        output->addedChildren.push_back(obj);
      }
    }

    const Vector<HComponent>& prefabComponents = prefab->getComponents();
    const Vector<HComponent>& instanceComponents = instance->getComponents();

    auto prefabComponentCount = static_cast<uint32>(prefabComponents.size());
    auto instanceComponentCount = static_cast<uint32>(instanceComponents.size());

    //Find modified and removed components
    for (uint32 i = 0; i < prefabComponentCount; ++i) {
      HComponent prefabComponent = prefabComponents[i];

      SPtr<PrefabComponentDiff> childDiff;
      bool foundMatching = false;
      for (uint32 j = 0; j < instanceComponentCount; ++j) {
        HComponent instanceComponent = instanceComponents[j];

        if (prefabComponent->getLinkId() == instanceComponent->getLinkId()) {
          auto encodedPrefab = SerializedObject::create(*prefabComponent);
          auto encodedInstance = SerializedObject::create(*instanceComponent);

          IDiff& diffHandler = prefabComponent->getRTTI()->getDiffHandler();
          auto diff = diffHandler.generateDiff(encodedPrefab, encodedInstance);

          if (nullptr != diff) {
            childDiff = ge_shared_ptr_new<PrefabComponentDiff>();
            childDiff->id = prefabComponent->getLinkId();
            childDiff->data = diff;
          }

          foundMatching = true;
          break;
        }
      }

      if (foundMatching) {
        if (nullptr != childDiff) {
          if (nullptr == output) {
            output = ge_shared_ptr_new<PrefabObjectDiff>();
          }
          output->componentDiffs.push_back(childDiff);
        }
      }
      else {
        if (nullptr == output) {
          output = ge_shared_ptr_new<PrefabObjectDiff>();
        }

        output->removedComponents.push_back(prefabComponent->getLinkId());
      }
    }

    //Find added components
    for (uint32 i = 0; i < instanceComponentCount; ++i) {
      HComponent instanceComponent = instanceComponents[i];

      bool foundMatching = false;
      if (instanceComponent->getLinkId() != NumLimit::MAX_UINT32) {
        for (uint32 j = 0; j < prefabComponentCount; ++j) {
          HComponent prefabComponent = prefabComponents[j];

          if (prefabComponent->getLinkId() == instanceComponent->getLinkId()) {
            foundMatching = true;
            break;
          }
        }
      }

      if (!foundMatching) {
        SPtr<SerializedObject> obj = SerializedObject::create(*instanceComponent);

        if (nullptr == output) {
          output = ge_shared_ptr_new<PrefabObjectDiff>();
        }

        output->addedComponents.push_back(obj);
      }
    }

    if (nullptr != output) {
      output->id = instance->getLinkId();
    }

    return output;
  }

  void
  PrefabDiff::renameInstanceIds(const HSceneObject& prefab,
                                const HSceneObject& instance,
                                Vector<RenamedGameObject>& output) {
    UnorderedMap<UUID, UnorderedMap<uint32, uint64>> linkToInstanceId;

    struct StackEntry
    {
      HSceneObject so;
      UUID uuid;
    };

    //When renaming it is important to rename the prefab and not the instance,
    //since the diff will otherwise contain prefab's IDs, but will be used for
    //the instance.

    Stack<StackEntry> todo;
    todo.push({ instance, UUID::EMPTY });

    while (!todo.empty()) {
      StackEntry current = todo.top();
      todo.pop();

      UUID childParentUUID;
      if (current.so->m_prefabLinkUUID.empty()) {
        childParentUUID = current.uuid;
      }
      else {
        childParentUUID = current.so->m_prefabLinkUUID;
      }

      UnorderedMap<uint32, uint64>& idMap = linkToInstanceId[childParentUUID];

      const Vector<HComponent>& components = current.so->getComponents();
      for (auto& component : components) {
        if (component->getLinkId() != NumLimit::MAX_UINT32) {
          idMap[component->getLinkId()] = component->getInstanceId();
        }
      }

      uint32 numChildren = current.so->getNumChildren();
      for (uint32 i = 0; i < numChildren; ++i) {
        HSceneObject child = current.so->getChild(i);

        if (child->getLinkId() != NumLimit::MAX_UINT32) {
          idMap[child->getLinkId()] = child->getInstanceId();
        }

        todo.push({ child, childParentUUID });
      }
    }

    //Root has link ID from its parent so we handle it separately
    {
      output.emplace_back();
      RenamedGameObject& renamedGO = output.back();
      renamedGO.instanceData = instance->m_instanceData;
      renamedGO.originalId = instance->getInstanceId();
      prefab->m_instanceData->instanceId = instance->getInstanceId();
    }

    todo.push({ prefab, UUID::EMPTY });
    while (!todo.empty()) {
      StackEntry current = todo.top();
      todo.pop();

      UUID childParentUUID;
      if (current.so->m_prefabLinkUUID.empty()) {
        childParentUUID = current.uuid;
      }
      else {
        childParentUUID = current.so->m_prefabLinkUUID;
      }

      auto iterFind = linkToInstanceId.find(childParentUUID);
      if (iterFind != linkToInstanceId.end()) {
        UnorderedMap<uint32, uint64>& idMap = iterFind->second;

        const Vector<HComponent>& components = current.so->getComponents();
        for (auto& component : components) {
          auto iterFind2 = idMap.find(component->getLinkId());
          if (iterFind2 != idMap.end()) {
            output.emplace_back();
            RenamedGameObject& renamedGO = output.back();
            renamedGO.instanceData = component->m_instanceData;
            renamedGO.originalId = component->getInstanceId();

            component->m_instanceData->instanceId = iterFind2->second;
          }
        }
      }

      uint32 numChildren = current.so->getNumChildren();
      for (uint32 i = 0; i < numChildren; ++i) {
        HSceneObject child = current.so->getChild(i);

        if (iterFind != linkToInstanceId.end()) {
          if (child->getLinkId() != NumLimit::MAX_UINT32) {
            UnorderedMap<uint32, uint64>& idMap = iterFind->second;

            auto iterFind2 = idMap.find(child->getLinkId());
            if (iterFind2 != idMap.end()) {
              output.emplace_back();
              RenamedGameObject& renamedGO = output.back();
              renamedGO.instanceData = child->m_instanceData;
              renamedGO.originalId = child->getInstanceId();

              child->m_instanceData->instanceId = iterFind2->second;
            }
          }
        }
        todo.push({ child, childParentUUID });
      }
    }
  }

  void
  PrefabDiff::restoreInstanceIds(const Vector<RenamedGameObject>& renamedObjects) {
    for (auto& renamedGO : renamedObjects) {
      renamedGO.instanceData->instanceId = renamedGO.originalId;
    }
  }

  RTTITypeBase*
  PrefabDiff::getRTTIStatic() {
    return PrefabDiffRTTI::instance();
  }

  RTTITypeBase*
  PrefabDiff::getRTTI() const {
    return PrefabDiff::getRTTIStatic();
  }
}
