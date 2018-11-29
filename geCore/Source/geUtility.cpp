/*****************************************************************************/
/**
 * @file    geUtility.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/01
 * @brief   Static class containing various utility methods that do not fit
 *          anywhere else.
 *
 * Static class containing various utility methods that do not fit anywhere
 * else.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geUtility.h"
#include "geRTTIType.h"
#include "geSceneObject.h"

namespace geEngineSDK {
  /**
   * @brief Checks if the specified type (or any of its derived classes)
   *        have any IReflectable pointer or value types as their fields.
   */
  bool
  hasReflectableChildren(RTTITypeBase* type) {
    uint32 numFields = type->getNumFields();
    for (uint32 i = 0; i < numFields; ++i) {
      RTTIField* field = type->getField(i);
      if (field->isReflectableType() || field->isReflectablePtrType()) {
        return true;
      }
    }

    const Vector<RTTITypeBase*>& derivedClasses = type->getDerivedClasses();
    for (auto& derivedClass : derivedClasses) {
      numFields = derivedClass->getNumFields();
      for (uint32 i = 0; i < numFields; ++i) {
        RTTIField* field = derivedClass->getField(i);
        if (field->isReflectableType() || field->isReflectablePtrType()) {
          return true;
        }
      }
    }

    return false;
  }

  void
  findResourceDependenciesInternal(IReflectable& obj,
                                   FrameAlloc& alloc,
                                   bool recursive,
                                   Map<UUID, ResourceDependency>& dependencies) {
    RTTITypeBase* rtti = obj.getRTTI();
    do {
      RTTITypeBase* rttiInstance = rtti->_clone(alloc);
      rttiInstance->onSerializationStarted(&obj, nullptr);

      const uint32 numFields = rtti->getNumFields();
      for (uint32 i = 0; i < numFields; ++i) {
        RTTIField* field = rtti->getField(i);
        if ((field->getFlags() & RTTI_FIELD_FLAG::kSkipInReferenceSearch) != 0) {
          continue;
        }

        if (field->isReflectableType()) {
          auto reflectableField = static_cast<RTTIReflectableFieldBase*>(field);

          if (TYPEID_CORE::kID_ResourceHandle == reflectableField->getType()->getRTTIId()) {
            if (reflectableField->isArray()) {
              const uint32 numElements = reflectableField->getArraySize(rttiInstance, &obj);
              for (uint32 j = 0; j < numElements; ++j) {
                HResource resource = static_cast<HResource&>(
                  reflectableField->getArrayValue(rttiInstance, &obj, j));
                if (!resource.getUUID().empty()) {
                  auto& dependency = dependencies[resource.getUUID()];
                  dependency.resource = resource;
                  ++dependency.numReferences;
                }
              }
            }
            else {
              HResource resource = static_cast<HResource&>
                                     (reflectableField->getValue(rttiInstance, &obj));
              if (!resource.getUUID().empty()) {
                auto& dependency = dependencies[resource.getUUID()];
                dependency.resource = resource;
                ++dependency.numReferences;
              }
            }
          }
          else if (recursive) {
            //Optimization, no need to retrieve its value and go deeper if it has
            //no reflectable children that may hold the reference.
            if (hasReflectableChildren(reflectableField->getType())) {
              if (reflectableField->isArray()) {
                const uint32 numElements = reflectableField->getArraySize(rttiInstance, &obj);
                for (uint32 j = 0; j < numElements; ++j) {
                  IReflectable& childObj = reflectableField->getArrayValue(rttiInstance,
                                                                           &obj,
                                                                           j);
                  findResourceDependenciesInternal(childObj, alloc, true, dependencies);
                }
              }
              else {
                auto& childObj = reflectableField->getValue(rttiInstance, &obj);
                findResourceDependenciesInternal(childObj, alloc, true, dependencies);
              }
            }
          }
        }
        else if (field->isReflectablePtrType() && recursive) {
          auto reflectablePtrField = static_cast<RTTIReflectablePtrFieldBase*>(field);

          //Optimization, no need to retrieve its value and go deeper if it has
          //no reflectable children that may hold the reference.
          if (hasReflectableChildren(reflectablePtrField->getType())) {
            if (reflectablePtrField->isArray()) {
              const auto numElements = reflectablePtrField->getArraySize(rttiInstance, &obj);
              for (uint32 j = 0; j < numElements; ++j) {
                const auto& childObj = reflectablePtrField->getArrayValue(rttiInstance,
                                                                          &obj,
                                                                          j);

                if (nullptr != childObj) {
                  findResourceDependenciesInternal(*childObj, alloc, true, dependencies);
                }
              }
            }
            else {
              const auto& childObj = reflectablePtrField->getValue(rttiInstance, &obj);

              if (nullptr != childObj) {
                findResourceDependenciesInternal(*childObj, alloc, true, dependencies);
              }
            }
          }
        }
      }

      rttiInstance->onSerializationEnded(&obj, nullptr);
      alloc.destruct(rttiInstance);
      
      rtti = rtti->getBaseClass();
    } while (nullptr != rtti);
  }

  Vector<ResourceDependency>
  Utility::findResourceDependencies(IReflectable& obj, bool recursive) {
    g_frameAlloc().markFrame();

    Map<UUID, ResourceDependency> dependencies;
    findResourceDependenciesInternal(obj, g_frameAlloc(), recursive, dependencies);

    g_frameAlloc().clear();

    Vector<ResourceDependency> dependencyList(dependencies.size());
    uint32 i = 0;
    for (auto& entry : dependencies) {
      dependencyList[i] = entry.second;
      ++i;
    }

    return dependencyList;
  }

  uint32
  Utility::getSceneObjectDepth(const HSceneObject& so) {
    HSceneObject parent = so->getParent();

    uint32 depth = 0;
    while (nullptr != parent) {
      ++depth;
      parent = parent->getParent();
    }

    return depth;
  }

  Vector<HComponent>
  Utility::findComponents(const HSceneObject& object, uint32 typeId) {
    Vector<HComponent> output;

    Stack<HSceneObject> todo;
    todo.push(object);

    while (!todo.empty()) {
      HSceneObject curSO = todo.top();
      todo.pop();

      const Vector<HComponent>& components = curSO->getComponents();
      for (auto& entry : components) {
        if (entry->getRTTI()->getRTTIId() == typeId) {
          output.push_back(entry);
        }
      }

      uint32 numChildren = curSO->getNumChildren();
      for (uint32 i = 0; i < numChildren; ++i) {
        todo.push(curSO->getChild(i));
      }
    }

    return output;
  }

  class CoreSerializationContextRTTI
    : public RTTIType<CoreSerializationContext,
                      SerializationContext,
                      CoreSerializationContextRTTI>
  {
    const String&
    getRTTIName() override {
      static String name = "CoreSerializationContext";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_CoreSerializationContext;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      GE_EXCEPT(InternalErrorException,
                "Cannot instantiate an abstract class.");
      return nullptr;
    }
  };

  RTTITypeBase*
  CoreSerializationContext::getRTTIStatic() {
    return CoreSerializationContextRTTI::instance();
  }

  RTTITypeBase*
  CoreSerializationContext::getRTTI() const {
    return getRTTIStatic();
  }
}
