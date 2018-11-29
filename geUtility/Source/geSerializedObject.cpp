/*****************************************************************************/
/**
 * @file    geSerializedObject.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Base class for intermediate representations of objects that are
 *          being decoded with BinarySerializer.
 *
 * Base class for intermediate representations of objects that are being
 * decoded with BinarySerializer.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geSerializedObject.h"
#include "geSerializedObjectRTTI.h"

namespace geEngineSDK {
  using std::make_pair;
  using std::static_pointer_cast;

  namespace detail {
    /**
     * @brief Helper class for performing SerializedObject <-> IReflectable
     *        encoding & decoding.
     */
    class GE_UTILITY_EXPORT IntermediateSerializer
    {
     public:
      IntermediateSerializer();

      /**
       * @brief Encodes an IReflectable object into an intermediate
       *        representation.
       */
      SPtr<SerializedObject>
      encode(IReflectable* object,
             bool shallow = false,
             SerializationContext* context = nullptr);

      /**
       * @brief Decodes an intermediate representation of a serialized object
       *        into the actual object.
       */
      SPtr<IReflectable>
      decode(const SerializedObject* serializedObject,
             SerializationContext* context = nullptr);

     private:
      struct ObjectToDecode
      {
        ObjectToDecode(const SPtr<IReflectable>& _object,
                       const SerializedObject* serializedObject)
          : object(_object),
            serializedObject(serializedObject)
        {}

        SPtr<IReflectable> object;
        const SerializedObject* serializedObject;
        bool isDecoded = false;
        bool decodeInProgress = false; //Used for error reporting circular references
      };

      /**
       * @brief Decodes a single IReflectable object.
       */
      void
      decodeEntry(const SPtr<IReflectable>& object,
                  const SerializedObject* serializableObject);

      /**
       * @brief Encodes a single IReflectable object.
       */
      SPtr<SerializedObject>
      encodeEntry(IReflectable* object, bool shallow);

      UnorderedMap<const SerializedObject*, ObjectToDecode> m_objectMap;
      SerializationContext* m_context = nullptr;
      FrameAlloc* m_alloc = nullptr;
    };

    IntermediateSerializer::IntermediateSerializer()
      : m_alloc(&g_frameAlloc())
    {}

    SPtr<IReflectable>
    IntermediateSerializer::decode(const SerializedObject* serializedObject,
                                   SerializationContext* context) {
      m_context = context;
      m_alloc->markFrame();
      m_objectMap.clear();

      SPtr<IReflectable> output;
      RTTITypeBase* type = IReflectable::_getRTTIfromTypeId(
                             serializedObject->getRootTypeId());
      if (nullptr != type) {
        output = type->newRTTIObject();
        auto iterNewObj = m_objectMap.insert(make_pair(serializedObject,
                                                       ObjectToDecode(output,
                                                                      serializedObject)));

        iterNewObj.first->second.decodeInProgress = true;
        decodeEntry(output, serializedObject);
        iterNewObj.first->second.decodeInProgress = false;
        iterNewObj.first->second.isDecoded = true;
      }

      //Go through the remaining objects (should be only ones with weak refs)
      for (auto & iter : m_objectMap) {
        ObjectToDecode& objToDecode = iter.second;

        if (objToDecode.isDecoded) {
          continue;
        }

        objToDecode.decodeInProgress = true;
        decodeEntry(objToDecode.object, objToDecode.serializedObject);
        objToDecode.decodeInProgress = false;
        objToDecode.isDecoded = true;
      }

      m_objectMap.clear();
      m_alloc->clear();

      return output;
    }

    SPtr<SerializedObject>
    IntermediateSerializer::encode(IReflectable* object,
                                   bool shallow,
                                   SerializationContext* context) {
      m_context = context;
      return encodeEntry(object, shallow);
    }

    void
    IntermediateSerializer::decodeEntry(const SPtr<IReflectable>& object,
                                        const SerializedObject* serializableObject) {
      auto numSubObjects = static_cast<uint32>(serializableObject->subObjects.size());
      if (0 == numSubObjects) {
        return;
      }

      FrameStack<RTTITypeBase*> rttiInstances;
      for (int32 subObjectIdx = numSubObjects - 1; subObjectIdx >= 0; --subObjectIdx) {
        const SerializedSubObject& subObject = serializableObject->subObjects[subObjectIdx];

        RTTITypeBase* rtti = IReflectable::_getRTTIfromTypeId(subObject.typeId);
        if (nullptr == rtti) {
          continue;
        }

        RTTITypeBase* rttiInstance = rtti->_clone(*m_alloc);
        rttiInstance->onDeserializationStarted(object.get(), m_context);
        rttiInstances.push(rttiInstance);

        uint32 numFields = rtti->getNumFields();
        for (uint32 fieldIdx = 0; fieldIdx < numFields; ++fieldIdx) {
          RTTIField* curGenericField = rtti->getField(fieldIdx);

          auto iterFindFieldData = subObject.entries.find(curGenericField->m_uniqueId);
          if (subObject.entries.end() == iterFindFieldData) {
            continue;
          }

          SPtr<SerializedInstance> entryData = iterFindFieldData->second.serialized;
          if (curGenericField->isArray()) {
            auto arrayData = static_pointer_cast<SerializedArray>(entryData);

            uint32 arrayNumElems = arrayData->numElements;
            curGenericField->setArraySize(rttiInstance, object.get(), arrayNumElems);

            switch (curGenericField->m_type) {
              case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
              {
                auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

                for (auto& arrayElem : arrayData->entries) {
                  auto arrayElemData = static_pointer_cast<SerializedObject>
                                         (arrayElem.second.serialized);
                  RTTITypeBase* childRtti = nullptr;

                  if (nullptr != arrayElemData) {
                    childRtti = IReflectable::_getRTTIfromTypeId(
                                  arrayElemData->getRootTypeId());
                  }

                  if (nullptr != childRtti) {
                    auto findObj = m_objectMap.find(arrayElemData.get());
                    if (findObj == m_objectMap.end()) {
                      SPtr<IReflectable> newObject = childRtti->newRTTIObject();
                      findObj = m_objectMap.insert(make_pair(arrayElemData.get(),
                                                             ObjectToDecode(newObject,
                                                               arrayElemData.get()))).first;
                    }

                    ObjectToDecode& objToDecode = findObj->second;

                    bool needsDecoding = (curField->getFlags() &
                                         RTTI_FIELD_FLAG::kWeakRef) == 0 &&
                                         !objToDecode.isDecoded;
                    if (needsDecoding) {
                      if (objToDecode.decodeInProgress) {
                        LOGWRN("Detected a circular reference when decoding. "
                               "Referenced object fields will be resolved in "
                               "an undefined order (i.e. one of the objects "
                               "will not be fully deserialized when assigned "
                               "to its field). Use RTTI_Flag_WeakRef to get "
                               "rid of this warning and tell the system which "
                               "of the objects is allowed to be deserialized "
                               "after it is assigned to its field.");
                      }
                      else {
                        objToDecode.decodeInProgress = true;
                        decodeEntry(objToDecode.object, objToDecode.serializedObject);
                        objToDecode.decodeInProgress = false;
                        objToDecode.isDecoded = true;
                      }
                    }

                    curField->setArrayValue(rttiInstance,
                                            object.get(),
                                            arrayElem.first,
                                            objToDecode.object);
                  }
                  else {
                    curField->setArrayValue(rttiInstance,
                                            object.get(),
                                            arrayElem.first,
                                            nullptr);
                  }
                }
              }
              break;
              case SERIALIZABLE_FIELD_TYPE::kReflectable:
              {
                auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

                for (auto& arrayElem : arrayData->entries) {
                  auto arrayElemData = static_pointer_cast<SerializedObject>
                                         (arrayElem.second.serialized);
                  RTTITypeBase* childRtti = nullptr;

                  if (nullptr != arrayElemData) {
                    childRtti = IReflectable::_getRTTIfromTypeId(
                                  arrayElemData->getRootTypeId());
                  }

                  if (nullptr != childRtti) {
                    SPtr<IReflectable> newObject = childRtti->newRTTIObject();
                    decodeEntry(newObject, arrayElemData.get());
                    curField->setArrayValue(rttiInstance, object.get(),
                                            arrayElem.first,
                                            *newObject);
                  }
                }
                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kPlain:
              {
                auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

                for (auto& arrayElem : arrayData->entries) {
                  auto fieldData = static_pointer_cast<SerializedField>
                                     (arrayElem.second.serialized);
                  if (nullptr != fieldData) {
                    curField->arrayElemFromBuffer(rttiInstance,
                                                  object.get(),
                                                  arrayElem.first,
                                                  fieldData->value);
                  }
                }
                break;
              }
              default:
                break;
            }
          }
          else {
            switch (curGenericField->m_type) {
              case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
              {
                auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

                auto fieldObjectData = static_pointer_cast<SerializedObject>(entryData);
                RTTITypeBase* childRtti = nullptr;

                if (nullptr != fieldObjectData) {
                  childRtti = IReflectable::_getRTTIfromTypeId(
                                fieldObjectData->getRootTypeId());
                }

                if (nullptr != childRtti) {
                  auto findObj = m_objectMap.find(fieldObjectData.get());
                  if (m_objectMap.end() == findObj) {
                    SPtr<IReflectable> newObject = childRtti->newRTTIObject();
                    findObj = m_objectMap.insert(make_pair(fieldObjectData.get(),
                                                             ObjectToDecode(newObject,
                                                               fieldObjectData.get()))).first;
                  }

                  ObjectToDecode& objToDecode = findObj->second;

                  bool needsDecoding = (curField->getFlags() &
                                        RTTI_FIELD_FLAG::kWeakRef) == 0 &&
                                        !objToDecode.isDecoded;
                  if (needsDecoding) {
                    if (objToDecode.decodeInProgress) {
                      LOGWRN("Detected a circular reference when decoding. "
                             "Referenced object's fields will be resolved in "
                             "an undefined order (i.e. one of the objects "
                             "will not be fully deserialized when assigned to "
                             "its field). Use RTTI_Flag_WeakRef to get rid of "
                             "this warning and tell the system which of the "
                             "objects is allowed to be deserialized after it "
                             "is assigned to its field.");
                    }
                    else {
                      objToDecode.decodeInProgress = true;
                      decodeEntry(objToDecode.object, objToDecode.serializedObject);
                      objToDecode.decodeInProgress = false;
                      objToDecode.isDecoded = true;
                    }
                  }

                  curField->setValue(rttiInstance, object.get(), objToDecode.object);
                }
                else {
                  curField->setValue(rttiInstance, object.get(), nullptr);
                }
              }
              break;
              case SERIALIZABLE_FIELD_TYPE::kReflectable:
              {
                auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

                auto fieldObjectData = static_pointer_cast<SerializedObject>(entryData);
                RTTITypeBase* childRtti = nullptr;

                if (nullptr != fieldObjectData) {
                  childRtti = IReflectable::_getRTTIfromTypeId(
                                fieldObjectData->getRootTypeId());
                }

                if (nullptr != childRtti) {
                  SPtr<IReflectable> newObject = childRtti->newRTTIObject();
                  decodeEntry(newObject, fieldObjectData.get());
                  curField->setValue(rttiInstance, object.get(), *newObject);
                }
                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kPlain:
              {
                auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

                auto fieldData = static_pointer_cast<SerializedField>(entryData);
                if (nullptr != fieldData) {
                  curField->fromBuffer(rttiInstance, object.get(), fieldData->value);
                }
                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kDataBlock:
              {
                auto curField = static_cast<RTTIManagedDataBlockFieldBase*>(curGenericField);

                auto fieldData = static_pointer_cast<SerializedDataBlock>(entryData);
                if (nullptr != fieldData) {
                  fieldData->stream->seek(fieldData->offset);
                  curField->setValue(rttiInstance, object.get(),
                                     fieldData->stream,
                                     fieldData->size);
                }
                break;
              }
            }
          }
        }
      }

      while (!rttiInstances.empty()) {
        RTTITypeBase* rttiInstance = rttiInstances.top();
        rttiInstance->onDeserializationEnded(object.get(), m_context);
        m_alloc->destruct(rttiInstance);

        rttiInstances.pop();
      }
    }

    SPtr<SerializedObject>
    IntermediateSerializer::encodeEntry(IReflectable* object, bool shallow) {
      FrameStack<RTTITypeBase*> rttiInstances;
      RTTITypeBase* rtti = object->getRTTI();

      const auto cleanup = [&]() {
        while (!rttiInstances.empty()) {
          RTTITypeBase* rttiInstance = rttiInstances.top();
          rttiInstance->onSerializationEnded(object, m_context);
          m_alloc->destruct(rttiInstance);

          rttiInstances.pop();
        }
      };

      auto output = ge_shared_ptr_new<SerializedObject>();

      //If an object has base classes, we need to iterate through all of them
      do {
        RTTITypeBase* rttiInstance = rtti->_clone(*m_alloc);
        rttiInstances.push(rttiInstance);

        rttiInstance->onSerializationStarted(object, m_context);

        output->subObjects.emplace_back();
        SerializedSubObject& subObject = output->subObjects.back();
        subObject.typeId = rtti->getRTTIId();

        const uint32 numFields = rtti->getNumFields();
        for (uint32 i = 0; i < numFields; ++i) {
          SPtr<SerializedInstance> serializedEntry;

          RTTIField* curGenericField = rtti->getField(i);
          if (curGenericField->m_isVectorType) {
            const uint32 arrayNumElems = curGenericField->getArraySize(rttiInstance, object);

            const auto serializedArray = ge_shared_ptr_new<SerializedArray>();
            serializedArray->numElements = arrayNumElems;

            serializedEntry = serializedArray;

            switch (curGenericField->m_type) {
              case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
              {
                auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

                for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                  SPtr<SerializedObject> serializedChildObj = nullptr;

                  if (!shallow) {
                    auto childObject = curField->getArrayValue(rttiInstance, object, arrIdx);

                    if (childObject) {
                      serializedChildObj = encodeEntry(childObject.get(), shallow);
                    }
                  }

                  SerializedArrayEntry arrayEntry;
                  arrayEntry.serialized = serializedChildObj;
                  arrayEntry.index = arrIdx;

                  serializedArray->entries[arrIdx] = arrayEntry;
                }

                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kReflectable:
              {
                auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

                for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                  auto& childObject = curField->getArrayValue(rttiInstance, object, arrIdx);

                  const auto serializedChildObj = encodeEntry(&childObject, shallow);

                  SerializedArrayEntry arrayEntry;
                  arrayEntry.serialized = serializedChildObj;
                  arrayEntry.index = arrIdx;

                  serializedArray->entries[arrIdx] = arrayEntry;
                }

                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kPlain:
              {
                auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

                for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                  uint32 typeSize = 0;
                  if (curField->hasDynamicSize()) {
                    typeSize = curField->getArrayElemDynamicSize(rttiInstance,
                                                                 object,
                                                                 arrIdx);
                  }
                  else {
                    typeSize = curField->getTypeSize();
                  }

                  const auto serializedField = ge_shared_ptr_new<SerializedField>();
                  serializedField->value = reinterpret_cast<uint8*>(ge_alloc(typeSize));
                  serializedField->ownsMemory = true;
                  serializedField->size = typeSize;

                  curField->arrayElemToBuffer(rttiInstance,
                                              object,
                                              arrIdx,
                                              serializedField->value);

                  SerializedArrayEntry arrayEntry;
                  arrayEntry.serialized = serializedField;
                  arrayEntry.index = arrIdx;

                  serializedArray->entries[arrIdx] = arrayEntry;
                }
                break;
              }
              default:
                GE_EXCEPT(InternalErrorException,
                          "Error encoding data. Encountered a type I don't "
                          "know how to encode. Type: " +
                          toString(uint32(curGenericField->m_type)) +
                          ", Is array: " +
                          toString(curGenericField->m_isVectorType));
            }
          }
          else {
            switch (curGenericField->m_type) {
              case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
              {
                auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

                if (!shallow) {
                  auto childObject = curField->getValue(rttiInstance, object);

                  if (childObject) {
                    serializedEntry = encodeEntry(childObject.get(), shallow);
                  }
                }

                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kReflectable:
              {
                auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);
                auto& childObject = curField->getValue(rttiInstance, object);
                serializedEntry = encodeEntry(&childObject, shallow);
                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kPlain:
              {
                auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

                uint32 typeSize = 0;
                if (curField->hasDynamicSize()) {
                  typeSize = curField->getDynamicSize(rttiInstance, object);
                }
                else {
                  typeSize = curField->getTypeSize();
                }

                const auto serializedField = ge_shared_ptr_new<SerializedField>();
                serializedField->value = reinterpret_cast<uint8*>(ge_alloc(typeSize));
                serializedField->ownsMemory = true;
                serializedField->size = typeSize;

                curField->toBuffer(rttiInstance, object, serializedField->value);

                serializedEntry = serializedField;

                break;
              }
              case SERIALIZABLE_FIELD_TYPE::kDataBlock:
              {
                auto curField = static_cast<RTTIManagedDataBlockFieldBase*>(curGenericField);

                uint32 dataBlockSize = 0;
                auto blockStream = curField->getValue(rttiInstance, object, dataBlockSize);

                auto dataBlockBuffer = reinterpret_cast<uint8*>(ge_alloc(dataBlockSize));
                blockStream->read(dataBlockBuffer, dataBlockSize);

                SPtr<DataStream> stream = ge_shared_ptr_new<MemoryDataStream>
                                            (dataBlockBuffer, dataBlockSize);

                auto serializedDataBlock = ge_shared_ptr_new<SerializedDataBlock>();
                serializedDataBlock->stream = stream;
                serializedDataBlock->offset = 0;

                serializedDataBlock->size = dataBlockSize;
                serializedEntry = serializedDataBlock;

                break;
              }
              default:
                GE_EXCEPT(InternalErrorException,
                          "Error encoding data. Encountered a type I don't "
                          "know how to encode. Type: " +
                          toString(uint32(curGenericField->m_type)) +
                          ", Is array: " +
                          toString(curGenericField->m_isVectorType));
            }
          }

          SerializedEntry entry;
          entry.fieldId = curGenericField->m_uniqueId;
          entry.serialized = serializedEntry;

          subObject.entries.insert(make_pair(curGenericField->m_uniqueId, entry));
        }

        rtti = rtti->getBaseClass();

      } while (rtti != nullptr); // Repeat until we reach the top of the inheritance hierarchy

      cleanup();

      return output;
    }
  }

  SPtr<SerializedObject>
  SerializedObject::create(IReflectable& obj,
                           bool shallow,
                           SerializationContext* context) {
    detail::IntermediateSerializer is;
    return is.encode(&obj, shallow, context);
  }

  SPtr<IReflectable>
  SerializedObject::decode(SerializationContext* context) const {
    detail::IntermediateSerializer is;
    return is.decode(this, context);
  }

  SPtr<SerializedInstance>
  SerializedObject::clone(bool cloneData) {
    SPtr<SerializedObject> copy = ge_shared_ptr_new<SerializedObject>();
    copy->subObjects = Vector<SerializedSubObject>(subObjects.size());

    uint32 i = 0;
    for (auto& subObject : subObjects) {
      copy->subObjects[i].typeId = subObject.typeId;

      for (auto& entryPair : subObject.entries) {
        SerializedEntry entry = entryPair.second;

        if (entry.serialized != nullptr) {
          entry.serialized = entry.serialized->clone(cloneData);
        }

        copy->subObjects[i].entries[entryPair.first] = entry;
      }
      ++i;
    }

    return copy;
  }

  SPtr<SerializedInstance>
  SerializedField::clone(bool cloneData) {
    SPtr<SerializedField> copy = ge_shared_ptr_new<SerializedField>();
    copy->size = size;

    if (cloneData) {
      copy->value = reinterpret_cast<uint8*>(ge_alloc(size));
      memcpy(copy->value, value, size);
      copy->ownsMemory = true;
    }
    else {
      copy->value = value;
      copy->ownsMemory = false;
    }

    return copy;
  }

  SPtr<SerializedInstance>
  SerializedDataBlock::clone(bool cloneData) {
    SPtr<SerializedDataBlock> copy = ge_shared_ptr_new<SerializedDataBlock>();
    copy->size = size;

    if (cloneData) {
      if (stream->isFile()) {
        LOGWRN("Cloning a file stream. Streaming is disabled and stream data "
               "will be loaded into memory.");
      }

      uint8* data = reinterpret_cast<uint8*>(ge_alloc(size));
      stream->read(data, size);

      copy->stream = ge_shared_ptr_new<MemoryDataStream>(data, size);
      copy->offset = 0;
    }
    else {
      copy->stream = stream;
      copy->offset = offset;
    }

    return copy;
  }

  SPtr<SerializedInstance>
  SerializedArray::clone(bool cloneData) {
    SPtr<SerializedArray> copy = ge_shared_ptr_new<SerializedArray>();
    copy->numElements = numElements;

    for (auto& entryPair : entries) {
      SerializedArrayEntry entry = entryPair.second;
      entry.serialized = entry.serialized->clone(cloneData);

      copy->entries[entryPair.first] = entry;
    }

    return copy;
  }

  RTTITypeBase*
  SerializedInstance::getRTTIStatic() {
    return SerializedInstanceRTTI::instance();
  }

  RTTITypeBase*
  SerializedInstance::getRTTI() const {
    return SerializedInstance::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedDataBlock::getRTTIStatic() {
    return SerializedDataBlockRTTI::instance();
  }

  RTTITypeBase*
  SerializedDataBlock::getRTTI() const {
    return SerializedDataBlock::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedField::getRTTIStatic() {
    return SerializedFieldRTTI::instance();
  }

  RTTITypeBase*
  SerializedField::getRTTI() const {
    return SerializedField::getRTTIStatic();
  }

  uint32
  SerializedObject::getRootTypeId() const {
    if (!subObjects.empty()) {
      return subObjects[0].typeId;
    }

    return 0;
  }

  RTTITypeBase*
  SerializedObject::getRTTIStatic() {
    return SerializedObjectRTTI::instance();
  }

  RTTITypeBase*
  SerializedObject::getRTTI() const {
    return SerializedObject::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedArray::getRTTIStatic() {
    return SerializedArrayRTTI::instance();
  }

  RTTITypeBase*
  SerializedArray::getRTTI() const {
    return SerializedArray::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedSubObject::getRTTIStatic() {
    return SerializedSubObjectRTTI::instance();
  }

  RTTITypeBase*
  SerializedSubObject::getRTTI() const {
    return SerializedSubObject::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedEntry::getRTTIStatic() {
    return SerializedEntryRTTI::instance();
  }

  RTTITypeBase*
  SerializedEntry::getRTTI() const {
    return SerializedEntry::getRTTIStatic();
  }

  RTTITypeBase*
  SerializedArrayEntry::getRTTIStatic() {
    return SerializedArrayEntryRTTI::instance();
  }

  RTTITypeBase*
  SerializedArrayEntry::getRTTI() const {
    return SerializedArrayEntry::getRTTIStatic();
  }
}
