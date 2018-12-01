/*****************************************************************************/
/**
 * @file    gePrefabDiffRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/04
 * @brief   RTTI Objects for gePrefabDiff.
 *
 * RTTI Objects for gePrefabDiff.
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
#include "gePrefabDiff.h"
#include "geUtility.h"

#include <geRTTIType.h>
#include <geSerializedObject.h>
#include <geGameObjectManager.h>
#include <geBinarySerializer.h>

namespace geEngineSDK {
  using std::bind;
  using std::static_pointer_cast;

  class GE_CORE_EXPORT PrefabComponentDiffRTTI
    : public RTTIType< PrefabComponentDiff, IReflectable, PrefabComponentDiffRTTI >
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(id, 0)
      GE_RTTI_MEMBER_REFLPTR(data, 1)
    GE_END_RTTI_MEMBERS

   public:
    const String&
    getRTTIName() override {
      static String name = "PrefabComponentDiff";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_PrefabComponentDiff;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<PrefabComponentDiff>();
    }
  };

  class GE_CORE_EXPORT PrefabObjectDiffRTTI
    : public RTTIType < PrefabObjectDiff, IReflectable, PrefabObjectDiffRTTI >
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(id, 0)
      GE_RTTI_MEMBER_PLAIN(name, 1)

      GE_RTTI_MEMBER_REFLPTR_ARRAY(componentDiffs, 2)
      GE_RTTI_MEMBER_PLAIN_ARRAY(removedComponents, 3)
      GE_RTTI_MEMBER_REFLPTR_ARRAY(addedComponents, 4)
      GE_RTTI_MEMBER_REFLPTR_ARRAY(childDiffs, 5)

      GE_RTTI_MEMBER_PLAIN_ARRAY(removedChildren, 6)
      GE_RTTI_MEMBER_REFLPTR_ARRAY(addedChildren, 7)

      GE_RTTI_MEMBER_PLAIN(translation, 8)
      GE_RTTI_MEMBER_PLAIN(rotation, 9)
      GE_RTTI_MEMBER_PLAIN(scale, 10)
      GE_RTTI_MEMBER_PLAIN(isActive, 11)
      GE_RTTI_MEMBER_PLAIN(soFlags, 12)
    GE_END_RTTI_MEMBERS

   public:
    const String&
    getRTTIName() override {
      static String name = "PrefabObjectDiff";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_PrefabObjectDiff;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<PrefabObjectDiff>();
    }
  };

  class GE_CORE_EXPORT PrefabDiffRTTI
    : public RTTIType<PrefabDiff, IReflectable, PrefabDiffRTTI>
  {
    /**
     * @brief Contains data about a game object handle serialized in a prefab
     *        diff.
     */
    struct SerializedHandle
    {
      SPtr<SerializedObject> object;
      SPtr<GameObjectHandleBase> handle;
    };

   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_REFLPTR(m_root, 0)
    GE_END_RTTI_MEMBERS

   public:
    void
    onDeserializationStarted(IReflectable* obj, SerializationContext* context) override {
      auto prefabDiff = static_cast<PrefabDiff*>(obj);

      GE_ASSERT(nullptr != context &&
                rtti_is_of_type<CoreSerializationContext>(context));
      
      auto coreContext = static_cast<CoreSerializationContext*>(context);

      if (coreContext->goState) {
        coreContext->goState->registerOnDeserializationEndCallback(
          bind(&PrefabDiffRTTI::delayedOnDeserializationEnded, prefabDiff));
      }
    }

    void
    onDeserializationEnded(IReflectable* obj, SerializationContext* context) override {
      GE_ASSERT(nullptr != context &&
                rtti_is_of_type<CoreSerializationContext>(context));
      const auto coreContext = static_cast<CoreSerializationContext*>(context);
      GE_ASSERT(coreContext->goState);

      //Make sure to deserialize all game object handles since their IDs need
      //to be updated. Normally they are updated automatically upon
      //deserialization but since we store them in intermediate form we need to
      //manually deserialize and reserialize them in order to update their IDs.
      auto prefabDiff = static_cast<PrefabDiff*>(obj);
      Stack<SPtr<PrefabObjectDiff>> todo;

      if (nullptr != prefabDiff->m_root) {
        todo.push(prefabDiff->m_root);
      }

      UnorderedSet<SPtr<SerializedObject>> handleObjects;

      while (!todo.empty()) {
        SPtr<PrefabObjectDiff> current = todo.top();
        todo.pop();

        for (auto& component : current->addedComponents) {
          findGameObjectHandles(component, handleObjects);
        }

        for (auto& child : current->addedChildren) {
          findGameObjectHandles(child, handleObjects);
        }

        for (auto& component : current->componentDiffs) {
          findGameObjectHandles(component->data, handleObjects);
        }

        for (auto& child : current->childDiffs) {
          todo.push(child);
        }
      }

      Vector<SerializedHandle> handleData(handleObjects.size());

      uint32 idx = 0;
      for (auto& handleObject : handleObjects) {
        SerializedHandle& handle = handleData[idx];
        handle.object = handleObject;
        handle.handle = static_pointer_cast<GameObjectHandleBase>
                          (handleObject->decode(context));
        ++idx;
      }

      prefabDiff->m_rttiData = handleData;
    }

    /**
     * @brief Decodes GameObjectHandles from their binary format, because
     *        during deserialization GameObjectManager will update all object
     *        IDs and we want to keep the handles up to date.So we deserialize
     *        them and allow them to be updated before storing them back into
     *        binary format.
     */
    static void
    delayedOnDeserializationEnded(PrefabDiff* prefabDiff) {
      auto& handleData = any_cast_ref<Vector<SerializedHandle>>(prefabDiff->m_rttiData);

      for (auto& serializedHandle : handleData) {
        if (nullptr != serializedHandle.handle) {
          *serializedHandle.object = *SerializedObject::create(*serializedHandle.handle);
        }
      }

      prefabDiff->m_rttiData = nullptr;
    }

    /**
     * @brief Scans the entire hierarchy and find all serialized
     *        GameObjectHandle objects.
     */
    static void
    findGameObjectHandles(const SPtr<SerializedObject>& serializedObject,
                          UnorderedSet<SPtr<SerializedObject>>& handleObjects) {
      for (auto& subObject : serializedObject->subObjects) {
        RTTITypeBase* rtti = IReflectable::_getRTTIfromTypeId(subObject.typeId);
        if (nullptr == rtti) {
          continue;
        }

        if (rtti->getRTTIId() == TYPEID_CORE::kID_GameObjectHandleBase) {
          handleObjects.insert(serializedObject);
          return;
        }

        for (auto& child : subObject.entries) {
          RTTIField* curGenericField = rtti->findField(child.second.fieldId);
          if (nullptr == curGenericField) {
            continue;
          }

          SPtr<SerializedInstance> entryData = child.second.serialized;
          if (nullptr == entryData) {
            continue;
          }

          if (rtti_is_of_type<SerializedArray>(entryData)) {
            auto arrayData = static_pointer_cast<SerializedArray>(entryData);

            for (auto& arrayElem : arrayData->entries) {
              if (nullptr != arrayElem.second.serialized &&
                  rtti_is_of_type<SerializedObject>(arrayElem.second.serialized)) {
                auto arrayElemData =
                  static_pointer_cast<SerializedObject>(arrayElem.second.serialized);
                findGameObjectHandles(arrayElemData, handleObjects);
              }
            }
          }
          else if (rtti_is_of_type<SerializedObject>(entryData)) {
            auto fieldObjectData = static_pointer_cast<SerializedObject>(entryData);
            findGameObjectHandles(fieldObjectData, handleObjects);
          }
        }
      }
    }

    const String&
    getRTTIName() override {
      static String name = "PrefabDiff";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_PrefabDiff;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<PrefabDiff>();
    }
  };
}
