/*****************************************************************************/
/**
 * @file    geBinaryCloner.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Class that performs cloning of an object that implements RTTI.
 *
 * Helper class that performs cloning of an object that implements RTTI.
 *
 * @bug     No known bugs.
 * @note    Requires the Frame Buffer to be initialized
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geBinaryCloner.h"
#include "geIReflectable.h"
#include "geRTTIType.h"
#include "geRTTIField.h"
#include "geRTTIPlainField.h"
#include "geRTTIReflectableField.h"
#include "geRTTIReflectablePtrField.h"
#include "geRTTIManagedDataBlockField.h"
#include "geMemorySerializer.h"

namespace geEngineSDK {
  using std::function;

  SPtr<IReflectable>
  BinaryCloner::clone(IReflectable* object, bool shallow) {
    if (nullptr == object) {
      return nullptr;
    }

    ObjectReferenceData referenceData;
    if (shallow) {
      FrameAlloc& alloc = g_frameAlloc();

      alloc.markFrame();
      gatherReferences(object, alloc, referenceData);
      alloc.clear();

      gatherReferences(object, alloc, referenceData);
    }

    function<void*(SIZE_T)> allocator = &MemoryAllocator<GenAlloc>::allocate;

    MemorySerializer ms;
    uint32 dataSize = 0;
    uint8* data = ms.encode(object, dataSize, allocator, shallow);
    SPtr<IReflectable> clonedObj = ms.decode(data, dataSize);

    if (shallow) {
      FrameAlloc& alloc = g_frameAlloc();

      alloc.markFrame();
      restoreReferences(clonedObj.get(), alloc, referenceData);
      alloc.clear();
    }

    ge_free(data);
    return clonedObj;
  }

  void
  BinaryCloner::gatherReferences(IReflectable* object,
                                 FrameAlloc& alloc,
                                 ObjectReferenceData& referenceData) {
    if (nullptr == object) {
      return;
    }

    RTTITypeBase* rtti = object->getRTTI();
    Stack<RTTITypeBase*> rttiInstances;
    while (nullptr != rtti) {
      RTTITypeBase* rttiInstance = rtti->_clone(alloc);
      rttiInstance->onSerializationStarted(object, nullptr);
      SubObjectReferenceData* subObjectData = nullptr;

      uint32 numFields = rtti->getNumFields();
      for (uint32 i = 0; i < numFields; ++i) {
        RTTIField* field = rtti->getField(i);
        FieldId fieldId;
        fieldId.field = field;
        fieldId.arrayIdx = -1;

        if (field->isArray()) {
          uint32 numElements = field->getArraySize(rttiInstance, object);

          for (uint32 j = 0; j < numElements; ++j) {
            fieldId.arrayIdx = j;

            if (SERIALIZABLE_FIELD_TYPE::kReflectablePtr == field->m_type) {
              auto curField = static_cast<RTTIReflectablePtrFieldBase*>(field);
              SPtr<IReflectable> childObj = curField->getArrayValue(rttiInstance, object, j);

              if (nullptr != childObj) {
                if (nullptr == subObjectData) {
                  referenceData.subObjectData.emplace_back();
                  subObjectData = &referenceData.subObjectData
                                  [referenceData.subObjectData.size() - 1];
                  subObjectData->rtti = rtti;
                }

                subObjectData->references.emplace_back();
                ObjectReference& reference = subObjectData->references.back();
                reference.fieldId = fieldId;
                reference.object = childObj;
              }
            }
            else if (SERIALIZABLE_FIELD_TYPE::kReflectable == field->m_type) {
              auto curField = static_cast<RTTIReflectableFieldBase*>(field);
              IReflectable* childObj = &curField->getArrayValue(rttiInstance, object, j);

              if (nullptr == subObjectData) {
                referenceData.subObjectData.emplace_back();
                subObjectData = &referenceData.subObjectData
                                [referenceData.subObjectData.size() - 1];
                subObjectData->rtti = rtti;
              }

              subObjectData->children.emplace_back();
              ObjectReferenceData& childData = subObjectData->children.back();
              childData.fieldId = fieldId;

              gatherReferences(childObj, alloc, childData);
            }
          }
        }
        else {
          if (SERIALIZABLE_FIELD_TYPE::kReflectablePtr == field->m_type) {
            auto curField = static_cast<RTTIReflectablePtrFieldBase*>(field);
            SPtr<IReflectable> childObj = curField->getValue(rttiInstance, object);

            if (nullptr != childObj) {
              if (nullptr == subObjectData) {
                referenceData.subObjectData.emplace_back();
                subObjectData = &referenceData.subObjectData
                                [referenceData.subObjectData.size() - 1];
                subObjectData->rtti = rtti;
              }

              subObjectData->references.emplace_back();
              ObjectReference& reference = subObjectData->references.back();
              reference.fieldId = fieldId;
              reference.object = childObj;
            }
          }
          else if (SERIALIZABLE_FIELD_TYPE::kReflectable == field->m_type) {
            auto curField = static_cast<RTTIReflectableFieldBase*>(field);
            IReflectable* childObj = &curField->getValue(rttiInstance, object);

            if (nullptr == subObjectData) {
              referenceData.subObjectData.emplace_back();
              subObjectData = &referenceData.subObjectData
                              [referenceData.subObjectData.size() - 1];
              subObjectData->rtti = rtti;
            }

            subObjectData->children.emplace_back();
            ObjectReferenceData& childData = subObjectData->children.back();
            childData.fieldId = fieldId;

            gatherReferences(childObj, alloc, childData);
          }
        }
      }

      rttiInstances.push(rttiInstance);
      rtti = rtti->getBaseClass();
    }

    while (!rttiInstances.empty()) {
      RTTITypeBase* rttiInstance = rttiInstances.top();
      rttiInstances.pop();

      rttiInstance->onSerializationEnded(object, nullptr);
      alloc.destruct(rttiInstance);
    }
  }

  void
  BinaryCloner::restoreReferences(IReflectable* object,
                                  FrameAlloc& alloc,
                                  const ObjectReferenceData& referenceData) {
    for (auto iter = referenceData.subObjectData.rbegin();
         iter != referenceData.subObjectData.rend();
         ++iter) {
      const SubObjectReferenceData& subObject = *iter;

      if (!subObject.references.empty()) {
        RTTITypeBase* rttiInstance = subObject.rtti->_clone(alloc);
        rttiInstance->onDeserializationStarted(object, nullptr);

        for (auto& reference : subObject.references) {
          auto curField = static_cast<RTTIReflectablePtrFieldBase*>(reference.fieldId.field);

          if (curField->isArray()) {
            curField->setArrayValue(rttiInstance,
                                    object,
                                    reference.fieldId.arrayIdx,
                                    reference.object);
          }
          else {
            curField->setValue(rttiInstance, object, reference.object);
          }
        }

        rttiInstance->onDeserializationEnded(object, nullptr);
        alloc.destruct(rttiInstance);
      }
    }

    for (auto& subObject : referenceData.subObjectData) {
      if (!subObject.children.empty()) {
        RTTITypeBase* rttiInstance = subObject.rtti->_clone(alloc);
        rttiInstance->onSerializationStarted(object, nullptr);

        for (auto& childObjectData : subObject.children) {
          auto curField = static_cast<RTTIReflectableFieldBase*>
                            (childObjectData.fieldId.field);

          IReflectable* childObj = nullptr;
          if (curField->isArray()) {
            childObj = &curField->getArrayValue(rttiInstance,
                                                object,
                                                childObjectData.fieldId.arrayIdx);
          }
          else {
            childObj = &curField->getValue(rttiInstance, object);
          }

          restoreReferences(childObj, alloc, childObjectData);
        }

        rttiInstance->onSerializationEnded(object, nullptr);
        alloc.destruct(rttiInstance);
      }
    }
  }
}
