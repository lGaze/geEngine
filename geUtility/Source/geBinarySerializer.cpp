/*****************************************************************************/
/**
* @file    geBinarySerializer.cpp
* @author  Samuel Prince (samuel.prince.quezada@gmail.com)
* @date    2017/11/03
* @brief   Encodes all the fields of the provided object into a binary format.
*
* Encodes all the fields of the provided object into a binary format. Fields
* are encoded using their unique IDs.
* Encoded data will remain compatible for decoding even if you modify the
* encoded class, as long as you assign new unique field IDs to added/modified
* fields.
*
* Like for any serializable class, fields are defined in RTTIType that each
* IReflectable class must be able to return.
*
* Any data the object or its children are pointing to will also be serialized
* (unless the pointer isn't registered in RTTIType). Upon decoding the pointer
* addresses will be set to proper values.
*
* @note Child elements are guaranteed to be fully deserialized before their
* parents, except for fields marked with WeakRef flag.
*
* @bug     No known bugs.
*/
/*****************************************************************************/

/*****************************************************************************/
/**
* Includes
*/
/*****************************************************************************/
#include "geBinarySerializer.h"
#include "geException.h"
#include "geDebug.h"
#include "geIReflectable.h"
#include "geRTTIType.h"
#include "geRTTIField.h"
#include "geRTTIPlainField.h"
#include "geRTTIReflectableField.h"
#include "geRTTIReflectablePtrField.h"
#include "geRTTIManagedDataBlockField.h"
#include "geMemorySerializer.h"
#include "geDataStream.h"

#include <unordered_set>

/**
 * @brief A macro that represents a block of code that gets used a lot inside
 *        encodeInternal. It checks if the buffer has enough space, and if it
 *        does it copies the data from the specified location and increments
 *        the needed pointers and counters. If there is not enough space the
 *        buffer is flushed (hopefully to make some space). If there is still
 *        not enough space the entire encoding process ends.
 * @param dataPtr Pointer to data which to copy.
 * @param size    Size of the data to copy
 */
#define COPY_TO_BUFFER(dataIter, size)                                        \
if((*bytesWritten + size) > bufferLength) {                                   \
	m_totalBytesWritten += *bytesWritten;                                       \
	buffer = flushBufferCallback(buffer - *bytesWritten,                        \
                               *bytesWritten,                                 \
                               bufferLength);                                 \
  if (nullptr == buffer || bufferLength < size) {                             \
    return nullptr;                                                           \
  }                                                                           \
	*bytesWritten = 0;                                                          \
}                                                                             \
                                                                              \
memcpy(buffer, dataIter, size);                                               \
buffer += size;                                                               \
*bytesWritten += size;

namespace geEngineSDK {
  using std::function;
  using std::make_pair;
  using std::static_pointer_cast;

  BinarySerializer::BinarySerializer()
    : m_alloc(&g_frameAlloc())
  {}

  void
  BinarySerializer::encode(IReflectable* object,
                           uint8* buffer,
                           uint32 bufferLength,
                           uint32* bytesWritten,
                           function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback,
                           bool shallow,
                           SerializationContext* context) {
    m_objectsToEncode.clear();
    m_objectAddrToId.clear();
    m_lastUsedObjectId = 1;
    *bytesWritten = 0;
    m_totalBytesWritten = 0;
    m_context = context;

    m_alloc->markFrame();

    Vector<SPtr<IReflectable>> encodedObjects;
    uint32 objectId = findOrCreatePersistentId(object);

    //Encode primary object and its value types
    buffer = encodeEntry(object,
                         objectId,
                         buffer,
                         bufferLength,
                         bytesWritten,
                         flushBufferCallback,
                         shallow);

    if (nullptr == buffer) {
      GE_EXCEPT(InternalErrorException,
                "Destination buffer is null or not large enough.");
    }

    //Encode pointed to objects and their value types
    UnorderedSet<uint32> serializedObjects;
    while (true) {
      auto iter = m_objectsToEncode.begin();
      bool foundObjectToProcess = false;
      for (; iter != m_objectsToEncode.end(); ++iter) {
        auto foundExisting = serializedObjects.find(iter->objectId);
        if (serializedObjects.end() != foundExisting){
          continue; //Already processed
        }

        SPtr<IReflectable> curObject = iter->object;
        uint32 curObjectid = iter->objectId;
        serializedObjects.insert(curObjectid);
        m_objectsToEncode.erase(iter);

        buffer = encodeEntry(curObject.get(),
                             curObjectid,
                             buffer,
                             bufferLength,
                             bytesWritten,
                             flushBufferCallback,
                             shallow);
        if (nullptr == buffer) {
          GE_EXCEPT(InternalErrorException,
                    "Destination buffer is null or not large enough.");
        }

        foundObjectToProcess = true;

        //Ensure we keep a reference to the object so it isn't released.
        //The system assigns unique IDs to IReflectable objects based on
        //pointer addresses but if objects get released then same address could
        //be assigned twice.
        //NOTE: To get around this I could assign unique IDs to IReflectable
        //objects
        encodedObjects.push_back(curObject);

        //Need to start over as m_objectsToSerialize was possibly modified
        break;
      }

      if (!foundObjectToProcess) { //We're done
        break;
      }
    }

    //Final flush
    if (*bytesWritten > 0) {
      m_totalBytesWritten += *bytesWritten;
      buffer = flushBufferCallback(buffer - *bytesWritten, *bytesWritten, bufferLength);
    }

    *bytesWritten = m_totalBytesWritten;

    encodedObjects.clear();
    m_objectsToEncode.clear();
    m_objectAddrToId.clear();

    m_alloc->clear();
  }

  SPtr<IReflectable>
  BinarySerializer::decode(const SPtr<DataStream>& data,
                           uint32 dataLength,
                           SerializationContext* context) {
    m_context = context;

    if (0 == dataLength) {
      return nullptr;
    }

    const SIZE_T start = data->tell();
    const SIZE_T end = start + dataLength;
    m_decodeObjectMap.clear();

    //Note: Ideally we can avoid iterating twice over the stream data
    //Create empty instances of all ptr objects
    SPtr<IReflectable> rootObject = nullptr;

    do {
      ObjectMetaData objectMetaData;
      objectMetaData.objectMeta = 0;
      objectMetaData.typeId = 0;

      if (data->read(&objectMetaData, sizeof(ObjectMetaData)) != sizeof(ObjectMetaData)) {
        GE_EXCEPT(InternalErrorException, "Error decoding data.");
      }

      data->seek(data->tell() - sizeof(ObjectMetaData));

      uint32 objectId = 0;
      uint32 objectTypeId = 0;
      bool objectIsBaseClass = false;
      decodeObjectMetaData(objectMetaData, objectId, objectTypeId, objectIsBaseClass);

      if (objectIsBaseClass) {
        GE_EXCEPT(InternalErrorException,
                  "Encountered a base-class object while looking for a new "  \
                  "object. Base class objects are only supposed to be parts " \
                  "of a larger object.");
      }

      SPtr<IReflectable> object = IReflectable::createInstanceFromTypeId(objectTypeId);
      m_decodeObjectMap.insert(make_pair(objectId, ObjectToDecode(object, data->tell())));

      if (rootObject == nullptr) {
        rootObject = object;
      }

    }while (decodeEntry(data, end, nullptr));

    //Now go through all of the objects and actually decode them
    for (auto& iter : m_decodeObjectMap) {
      ObjectToDecode& objToDecode = iter.second;

      if (objToDecode.isDecoded) {
        continue;
      }

      data->seek(objToDecode.offset);

      objToDecode.decodeInProgress = true;
      decodeEntry(data, end, objToDecode.object);
      objToDecode.decodeInProgress = false;
      objToDecode.isDecoded = true;
    }

    m_decodeObjectMap.clear();
    data->seek(end);

    return rootObject;
  }

  uint8*
  BinarySerializer::encodeEntry(IReflectable* object,
                                uint32 objectId,
                                uint8* buffer,
                                uint32& bufferLength,
                                uint32* bytesWritten,
                                function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback,
                                bool shallow) {
    RTTITypeBase* rtti = object->getRTTI();
    bool isBaseClass = false;

    FrameStack<RTTITypeBase*> rttiInstances;

    const auto cleanup = [&]() {
      while (!rttiInstances.empty()) {
        RTTITypeBase* rttiInstance = rttiInstances.top();
        rttiInstance->onSerializationEnded(object, m_context);
        m_alloc->destruct(rttiInstance);

        rttiInstances.pop();
      }
    };

    //If an object has base classes, we need to iterate through all of them
    do {
      RTTITypeBase* rttiInstance = rtti->_clone(*m_alloc);
      rttiInstances.push(rttiInstance);

      rtti->onSerializationStarted(object, m_context);

      //Encode object ID & type
      ObjectMetaData objectMetaData = encodeObjectMetaData(objectId,
                                                           rtti->getRTTIId(),
                                                           isBaseClass);
      COPY_TO_BUFFER(&objectMetaData, sizeof(ObjectMetaData));

      uint32 numFields = rtti->getNumFields();
      for (uint32 i = 0; i < numFields; ++i) {
        RTTIField* curGenericField = rtti->getField(i);

        //Copy field ID & other meta-data like field size and type
        uint32 metaData = encodeFieldMetaData(curGenericField->m_uniqueId,
                                           static_cast<uint8>(curGenericField->getTypeSize()),
                                           curGenericField->m_isVectorType,
                                           curGenericField->m_type,
                                           curGenericField->hasDynamicSize(),
                                           false);
        COPY_TO_BUFFER(&metaData, META_SIZE);

        if (curGenericField->m_isVectorType) {
          uint32 arrayNumElems = curGenericField->getArraySize(rttiInstance, object);

          //Copy num vector elements
          COPY_TO_BUFFER(&arrayNumElems, NUM_ELEM_FIELD_SIZE);

          switch (curGenericField->m_type)
          {
            case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
            {
              auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                SPtr<IReflectable> childObject;

                if (!shallow) {
                  childObject = curField->getArrayValue(rttiInstance, object, arrIdx);
                }

                uint32 objId = registerObjectPtr(childObject);
                COPY_TO_BUFFER(&objId, sizeof(uint32));
              }

              break;
            }
            case SERIALIZABLE_FIELD_TYPE::kReflectable:
            {
              auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                IReflectable& childObject = curField->getArrayValue(rttiInstance,
                                                                    object,
                                                                    arrIdx);

                buffer = complexTypeToBuffer(&childObject,
                                             buffer,
                                             bufferLength,
                                             bytesWritten,
                                             flushBufferCallback,
                                             shallow);
                if (nullptr == buffer) {
                  cleanup();
                  return nullptr;
                }
              }
              break;
            }
            case SERIALIZABLE_FIELD_TYPE::kPlain:
            {
              auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElems; ++arrIdx) {
                uint32 typeSize = 0;
                if (curField->hasDynamicSize()) {
                  typeSize = curField->getArrayElemDynamicSize(rttiInstance, object, arrIdx);
                }
                else {
                  typeSize = curField->getTypeSize();
                }

                if ((*bytesWritten + typeSize) > bufferLength) {
                  auto tempBuffer = 
                    reinterpret_cast<uint8*>(ge_stack_alloc(static_cast<SIZE_T>(typeSize)));
                  curField->arrayElemToBuffer(rttiInstance, object, arrIdx, tempBuffer);

                  buffer = dataBlockToBuffer(tempBuffer,
                                             typeSize,
                                             buffer,
                                             bufferLength,
                                             bytesWritten,
                                             flushBufferCallback);
                  ge_stack_free(tempBuffer);

                  if (nullptr == buffer || 0 == bufferLength) {
                    cleanup();
                    return nullptr;
                  }
                }
                else {
                  curField->arrayElemToBuffer(rttiInstance, object, arrIdx, buffer);
                  buffer += typeSize;
                  *bytesWritten += typeSize;
                }
              }
              break;
            }
            default:
              GE_EXCEPT(InternalErrorException,
                        "Error encoding data. Encountered a type I don't know "
                        "how to encode. Type: " +
                        toString(uint32(curGenericField->m_type)) +
                        ", Is array: " +
                        toString(curGenericField->m_isVectorType));
          }
        }
        else {
          switch (curGenericField->m_type)
          {
            case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
            {
              auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);
              SPtr<IReflectable> childObject;

              if (!shallow) {
                childObject = curField->getValue(rttiInstance, object);
              }

              uint32 objId = registerObjectPtr(childObject);
              COPY_TO_BUFFER(&objId, sizeof(uint32));
              break;
            }
            case SERIALIZABLE_FIELD_TYPE::kReflectable:
            {
              auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);
              IReflectable& childObject = curField->getValue(rttiInstance, object);

              buffer = complexTypeToBuffer(&childObject,
                                           buffer,
                                           bufferLength,
                                           bytesWritten,
                                           flushBufferCallback,
                                           shallow);
              if (nullptr == buffer) {
                cleanup();
                return nullptr;
              }
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

              if ((*bytesWritten + typeSize) > bufferLength) {
                auto tempBuffer = 
                  reinterpret_cast<uint8*>(ge_stack_alloc(static_cast<SIZE_T>(typeSize)));
                curField->toBuffer(rttiInstance, object, tempBuffer);

                buffer = dataBlockToBuffer(tempBuffer,
                                           typeSize,
                                           buffer,
                                           bufferLength,
                                           bytesWritten,
                                           flushBufferCallback);
                ge_stack_free(tempBuffer);

                if (nullptr == buffer || 0 == bufferLength) {
                  cleanup();
                  return nullptr;
                }
              }
              else {
                curField->toBuffer(rttiInstance, object, buffer);
                buffer += typeSize;
                *bytesWritten += typeSize;
              }
              break;
            }
            case SERIALIZABLE_FIELD_TYPE::kDataBlock:
            {
              auto curField = static_cast<RTTIManagedDataBlockFieldBase*>(curGenericField);

              uint32 dataBlockSize = 0;
              SPtr<DataStream> blockStream = curField->getValue(rttiInstance,
                                                                object,
                                                                dataBlockSize);

              //Data block size
              COPY_TO_BUFFER(&dataBlockSize, sizeof(uint32));

              //Data block data
              auto dataToStore = reinterpret_cast<uint8*>(ge_stack_alloc(dataBlockSize));
              blockStream->read(dataToStore, dataBlockSize);

              buffer = dataBlockToBuffer(dataToStore,
                                         dataBlockSize,
                                         buffer,
                                         bufferLength,
                                         bytesWritten,
                                         flushBufferCallback);
              ge_stack_free(dataToStore);

              if (nullptr == buffer || 0 == bufferLength) {
                cleanup();
                return nullptr;
              }
              break;
            }
            default:
              GE_EXCEPT(InternalErrorException,
                        "Error encoding data. Encountered a type I don't know "
                        "how to encode. Type: " +
                        toString(uint32(curGenericField->m_type)) +
                        ", Is array: " +
                        toString(curGenericField->m_isVectorType));
          }
        }
      }

      rtti = rtti->getBaseClass();
      isBaseClass = true;

    } while (nullptr != rtti);  //Repeat until we reach the top of the inheritance hierarchy

    return buffer;
  }

  bool
  BinarySerializer::decodeEntry(const SPtr<DataStream>& data,
                                SIZE_T dataEnd,
                                const SPtr<IReflectable>& output) {
    ObjectMetaData objectMetaData;
    objectMetaData.objectMeta = 0;
    objectMetaData.typeId = 0;

    if (data->read(&objectMetaData, sizeof(ObjectMetaData)) != sizeof(ObjectMetaData)) {
      GE_EXCEPT(InternalErrorException, "Error decoding data.");
    }

    uint32 objectId = 0;
    uint32 objectTypeId = 0;
    bool objectIsBaseClass = false;
    decodeObjectMetaData(objectMetaData, objectId, objectTypeId, objectIsBaseClass);

    if (objectIsBaseClass) {
      GE_EXCEPT(InternalErrorException,
                "Encountered a base-class object while looking for a new object. "
                "Base class objects are only supposed to be parts of a larger object.");
    }

    RTTITypeBase* rtti = nullptr;
    if (output) {
      rtti = output->getRTTI();
    }

    FrameVector<RTTITypeBase*> rttiInstances;

    auto finalizeObject = [&rttiInstances, this](IReflectable* object) {
      //NOTE: It would make sense to finish deserializing derived classes
      //before base classes, but some code depends on the old functionality,
      //so we'll keep it this way
      for (auto iter = rttiInstances.rbegin(); iter != rttiInstances.rend(); ++iter) {
        RTTITypeBase* curRTTI = *iter;

        curRTTI->onDeserializationEnded(object, m_context);
        m_alloc->destruct(curRTTI);
      }

      rttiInstances.clear();
    };

    RTTITypeBase* curRTTI = rtti;
    while (curRTTI) {
      RTTITypeBase* rttiInstance = curRTTI->_clone(*m_alloc);
      rttiInstances.push_back(rttiInstance);

      curRTTI = curRTTI->getBaseClass();
    }

    //Iterate in reverse to notify base classes before derived classes
    for (auto iter = rttiInstances.rbegin(); iter != rttiInstances.rend(); ++iter) {
      (*iter)->onDeserializationStarted(output.get(), m_context);
    }

    RTTITypeBase* rttiInstance = nullptr;
    uint32 rttiInstanceIdx = 0;
    if (!rttiInstances.empty()) {
      rttiInstance = rttiInstances[0];
    }

    while (data->tell() < dataEnd) {
      int32 metaData = -1;
      if (data->read(&metaData, META_SIZE) != META_SIZE) {
        GE_EXCEPT(InternalErrorException, "Error decoding data.");
      }

      if (isObjectMetaData(metaData)){
        //We've reached a new object or a base class of the current one
        ObjectMetaData objMetaData;
        objMetaData.objectMeta = 0;
        objMetaData.typeId = 0;

        data->seek(data->tell() - META_SIZE);
        if (data->read(&objMetaData, sizeof(ObjectMetaData)) != sizeof(ObjectMetaData)) {
          GE_EXCEPT(InternalErrorException, "Error decoding data.");
        }

        uint32 objId = 0;
        uint32 objTypeId = 0;
        bool objIsBaseClass = false;
        decodeObjectMetaData(objMetaData, objId, objTypeId, objIsBaseClass);

        //If it's a base class, get base class RTTI and handle that
        if (objIsBaseClass) {
          if (nullptr != rtti) {
            rtti = rtti->getBaseClass();
          }

          //Saved and current base classes don't match, so just skip over all that data
          if (nullptr == rtti || rtti->getRTTIId() != objTypeId) {
            rtti = nullptr;
          }

          rttiInstance = nullptr;

          if (rtti) {
            rttiInstance = rttiInstances[rttiInstanceIdx + 1];
            rttiInstanceIdx++;
          }

          continue;
        }
        else {
          //Found new object, we're done
          data->seek(data->tell() - sizeof(ObjectMetaData));

          finalizeObject(output.get());
          return true;
        }
      }

      bool isArray;
      SERIALIZABLE_FIELD_TYPE::E fieldType;
      uint16 fieldId;
      uint8 fieldSize;
      bool hasDynamicSize;
      bool terminator;
      decodeFieldMetaData(metaData,
                          fieldId,
                          fieldSize,
                          isArray,
                          fieldType,
                          hasDynamicSize,
                          terminator);

      if (terminator) {
        /*
         * We've processed the last field in this object, so return. Although
         * we return false we don't actually know if there is an object
         * following this one. However it doesn't matter since terminator
         * fields are only used for embedded objects that are all processed
         * within this method so we can compensate.
         */
        finalizeObject(output.get());
        return false;
      }

      RTTIField* curGenericField = nullptr;

      if (nullptr != rtti) {
        curGenericField = rtti->findField(fieldId);
      }

      if (nullptr != curGenericField) {
        if (!hasDynamicSize && curGenericField->getTypeSize() != fieldSize) {
          GE_EXCEPT(InternalErrorException,
                    "Data type mismatch. Type size stored in file and actual "
                    "type size don't match. (" +
                    toString(curGenericField->getTypeSize()) +
                    " vs. " +
                    toString(fieldSize) + ")");
        }

        if (curGenericField->m_isVectorType != isArray) {
          GE_EXCEPT(InternalErrorException,
                    "Data type mismatch. One is array, other is a single type.");
        }

        if (curGenericField->m_type != fieldType) {
          GE_EXCEPT(InternalErrorException,
                    "Data type mismatch. Field types don't match. " +
                    toString(uint32(curGenericField->m_type)) +
                    " vs. " +
                    toString(uint32(fieldType)));
        }
      }

      int32 arrayNumElems = 1;
      if (isArray) {
        if (data->read(&arrayNumElems, NUM_ELEM_FIELD_SIZE) != NUM_ELEM_FIELD_SIZE) {
          GE_EXCEPT(InternalErrorException, "Error decoding data.");
        }

        if (nullptr != curGenericField) {
          curGenericField->setArraySize(rttiInstance, output.get(), arrayNumElems);
        }

        switch (fieldType) {
          case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
          {
            auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

            for (int32 i = 0; i < arrayNumElems; ++i) {
              int32 childObjectId = 0;
              if (data->read(&childObjectId,
                             COMPLEX_TYPE_FIELD_SIZE) != COMPLEX_TYPE_FIELD_SIZE) {
                GE_EXCEPT(InternalErrorException, "Error decoding data.");
              }

              if (nullptr != curField) {
                auto findObj = m_decodeObjectMap.find(childObjectId);

                if (m_decodeObjectMap.end() == findObj) {
                  if (childObjectId != 0) {
                    LOGWRN("When deserializing, object ID: " +
                           toString(childObjectId) +
                           " was found but no such object was contained in "
                           "the file.");
                  }

                  curField->setArrayValue(rttiInstance, output.get(), i, nullptr);
                }
                else {
                  ObjectToDecode& objToDecode = findObj->second;

                  const bool needsDecoding =
                    (curField->getFlags() & RTTI_FIELD_FLAG::kWeakRef) == 0 &&
                    !objToDecode.isDecoded;

                  if (needsDecoding) {
                    if (objToDecode.decodeInProgress) {
                      LOGWRN("Detected a circular reference when decoding. "
                             "Referenced object's fields will be resolved in "
                             "an undefined order (i.e. one of the objects "
                             "will not be fully deserialized when assigned "
                             "to its field). Use RTTI_Flag_WeakRef to get "
                             "rid of this warning and tell the system which "
                             "of the objects is allowed to be deserialized "
                             "after it is assigned to its field.");
                    }
                    else {
                      objToDecode.decodeInProgress = true;

                      const SIZE_T curOffset = data->tell();
                      data->seek(objToDecode.offset);
                      decodeEntry(data, dataEnd, objToDecode.object);
                      data->seek(curOffset);

                      objToDecode.decodeInProgress = false;
                      objToDecode.isDecoded = true;
                    }
                  }

                  curField->setArrayValue(rttiInstance, output.get(), i, objToDecode.object);
                }
              }
            }

            break;
          }
          case SERIALIZABLE_FIELD_TYPE::kReflectable:
          {
            auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

            for (int32 i = 0; i < arrayNumElems; ++i) {
              SPtr<IReflectable> childObj;
              if (curField) {
                childObj = curField->newObject();
              }

              decodeEntry(data, dataEnd, childObj);

              if (nullptr != curField) {
                //NOTE: Would be nice to avoid this copy by value and decode
                //directly into the field
                curField->setArrayValue(rttiInstance, output.get(), i, *childObj);
              }
            }
            break;
          }
          case SERIALIZABLE_FIELD_TYPE::kPlain:
          {
            auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

            for (int32 i = 0; i < arrayNumElems; ++i) {
              uint32 typeSize = fieldSize;
              if (hasDynamicSize) {
                data->read(&typeSize, sizeof(uint32));
                data->seek(data->tell() - sizeof(uint32));
              }

              if (nullptr != curField) {
                //NOTE: Two data copies that can potentially be avoided:
                // - Copy from stream into a temporary buffer
                //   (use stream directly for decoding)
                // - Internally the field will do a value copy of the decoded
                //   object (ideally we decode directly into the destination)
                void* fieldValue = ge_stack_alloc(typeSize);
                data->read(fieldValue, typeSize);

                curField->arrayElemFromBuffer(rttiInstance, output.get(), i, fieldValue);
                ge_stack_free(fieldValue);
              }
              else {
                data->skip(typeSize);
              }
            }
            break;
          }
          default:
            GE_EXCEPT(InternalErrorException,
                      "Error decoding data. Encountered a type I don't know "
                      "how to decode. Type: "+
                      toString(uint32(fieldType)) +
                      ", Is array: " +
                      toString(isArray));
        }
      }
      else {
        switch (fieldType) {
          case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
          {
            auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

            int32 childObjectId = 0;
            if (data->read(&childObjectId,
                           COMPLEX_TYPE_FIELD_SIZE) != COMPLEX_TYPE_FIELD_SIZE) {
              GE_EXCEPT(InternalErrorException, "Error decoding data.");
            }

            if (nullptr != curField) {
              auto findObj = m_decodeObjectMap.find(childObjectId);

              if (m_decodeObjectMap.end() == findObj) {
                if (childObjectId != 0) {
                  LOGWRN("When deserializing, object ID: " +
                         toString(childObjectId) +
                         " was found but no such object was contained in the "
                         "file.");
                }

                curField->setValue(rttiInstance, output.get(), nullptr);
              }
              else {
                ObjectToDecode& objToDecode = findObj->second;

                const bool needsDecoding =
                  (curField->getFlags() & RTTI_FIELD_FLAG::kWeakRef) == 0 &&
                  !objToDecode.isDecoded;

                if (needsDecoding) {
                  if (objToDecode.decodeInProgress) {
                    LOGWRN("Detected a circular reference when decoding. "
                           "Referenced object's fields will be resolved in "
                           "an undefined order (i.e. one of the objects will "
                           "not be fully deserialized when assigned to its "
                           "field). Use RTTI_Flag_WeakRef to get rid of this "
                           "warning and tell the system which of the objects "
                           "is allowed to be deserialized after it is "
                           "assigned to its field.");
                  }
                  else {
                    objToDecode.decodeInProgress = true;

                    const SIZE_T curOffset = data->tell();
                    data->seek(objToDecode.offset);
                    decodeEntry(data, dataEnd, objToDecode.object);
                    data->seek(curOffset);

                    objToDecode.decodeInProgress = false;
                    objToDecode.isDecoded = true;
                  }
                }

                curField->setValue(rttiInstance, output.get(), objToDecode.object);
              }
            }

            break;
          }
          case SERIALIZABLE_FIELD_TYPE::kReflectable:
          {
            auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

            //NOTE: Ideally we can skip decoding the entry if the field no longer exists
            SPtr<IReflectable> childObj;
            if (curField) {
              childObj = curField->newObject();
            }

            decodeEntry(data, dataEnd, childObj);

            if (nullptr != curField) {
              //NOTE: Would be nice to avoid this copy by value and decode
              //directly into the field
              curField->setValue(rttiInstance, output.get(), *childObj);
            }

            break;
          }
          case SERIALIZABLE_FIELD_TYPE::kPlain:
          {
            auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

            uint32 typeSize = fieldSize;
            if (hasDynamicSize) {
              data->read(&typeSize, sizeof(uint32));
              data->seek(data->tell() - sizeof(uint32));
            }

            if (nullptr != curField) {
              //NOTE: Two data copies that can potentially be avoided:
              // - Copy from stream into a temporary buffer
              //   (use stream directly for decoding)
              // - Internally the field will do a value copy of the decoded
              //   object (ideally we decode directly into the destination)
              void* fieldValue = ge_stack_alloc(typeSize);
              data->read(fieldValue, typeSize);

              curField->fromBuffer(rttiInstance, output.get(), fieldValue);
              ge_stack_free(fieldValue);
            }
            else {
              data->skip(typeSize);
            }

            break;
          }
          case SERIALIZABLE_FIELD_TYPE::kDataBlock:
          {
            auto curField = static_cast<RTTIManagedDataBlockFieldBase*>(curGenericField);

            //Data block size
            uint32 dataBlockSize = 0;
            if (data->read(&dataBlockSize,
                           DATA_BLOCK_TYPE_FIELD_SIZE) != DATA_BLOCK_TYPE_FIELD_SIZE) {
              GE_EXCEPT(InternalErrorException, "Error decoding data.");
            }

            //Data block data
            if (nullptr != curField) {
              if (data->isFile()) { //Allow streaming
                const SIZE_T dataBlockOffset = data->tell();
                curField->setValue(rttiInstance, output.get(), data, dataBlockSize);

                //Seek past the data
                //(use original offset in case the field read from the stream)
                data->seek(dataBlockOffset + dataBlockSize);
              }
              else {
                auto dataBlockBuffer = reinterpret_cast<uint8*>(ge_alloc(dataBlockSize));
                data->read(dataBlockBuffer, dataBlockSize);

                SPtr<DataStream> stream =
                  ge_shared_ptr_new<MemoryDataStream>(dataBlockBuffer, dataBlockSize);
                curField->setValue(rttiInstance, output.get(), stream, dataBlockSize);
              }
            }
            else {
              data->skip(dataBlockSize);
            }
            break;
          }
          default:
            GE_EXCEPT(InternalErrorException,
                      "Error decoding data. Encountered a type I don't know "
                      "how to decode. Type: " +
                      toString(uint32(fieldType)) +
                      ", Is array: " +
                      toString(isArray));
        }
      }
    }

    finalizeObject(output.get());
    return false;
  }

  uint8*
  BinarySerializer::complexTypeToBuffer(IReflectable* object,
                                        uint8* buffer,
                                        uint32& bufferLength,
                                        uint32* bytesWritten,
                                        function<uint8*(uint8*, uint32, uint32&)>
                                        flushBufferCallback,
                                        bool shallow) {
    if (nullptr != object) {
      buffer = encodeEntry(object,
                           0,
                           buffer,
                           bufferLength,
                           bytesWritten,
                           flushBufferCallback,
                           shallow);

      //Encode terminator field
      //Complex types require terminator fields because they can be embedded
      //within other complex types and we need to know when their fields end
      //and parent's resume
      int32 metaData = encodeFieldMetaData(0,
                                           0,
                                           false,
                                           SERIALIZABLE_FIELD_TYPE::kPlain,
                                           false,
                                           true);
      COPY_TO_BUFFER(&metaData, META_SIZE)
    }

    return buffer;
  }

  uint32
  BinarySerializer::encodeFieldMetaData(uint16 id,
                                        uint8 size,
                                        bool array,
                                        SERIALIZABLE_FIELD_TYPE::E type,
                                        bool hasDynamicSize,
                                        bool terminator) {
    // If O == 0 - Meta contains field information (Encoded using this method)
    //// Encoding: IIII IIII IIII IIII SSSS SSSS xTYP DCAO
    //// I - Id
    //// S - Size
    //// C - Complex
    //// A - Array
    //// D - Data block
    //// P - Complex ptr
    //// O - Object descriptor
    //// Y - Plain field has dynamic size
    //// T - Terminator (last field in an object)

    return (id << 16 | size << 8 |
           (array ? 0x02 : 0) |
           ((type == SERIALIZABLE_FIELD_TYPE::kDataBlock) ? 0x04 : 0) |
           ((type == SERIALIZABLE_FIELD_TYPE::kReflectable) ? 0x08 : 0) |
           ((type == SERIALIZABLE_FIELD_TYPE::kReflectablePtr) ? 0x10 : 0) |
           (hasDynamicSize ? 0x20 : 0) |
           (terminator ? 0x40 : 0));
    //TODO: Low priority. Technically I could encode this much more tightly,
    //and use var-ints for ID
  }

  void
  BinarySerializer::decodeFieldMetaData(uint32 encodedData,
                                        uint16& id,
                                        uint8& size,
                                        bool& array,
                                        SERIALIZABLE_FIELD_TYPE::E& type,
                                        bool& hasDynamicSize,
                                        bool& terminator) {
    if (isObjectMetaData(encodedData)) {
      GE_EXCEPT(InternalErrorException,
                "Meta data represents an object description but is trying to "
                "be decoded as a field descriptor.");
    }

    terminator = (encodedData & 0x40) != 0;
    hasDynamicSize = (encodedData & 0x20) != 0;

    if ((encodedData & 0x10) != 0) {
      type = SERIALIZABLE_FIELD_TYPE::kReflectablePtr;
    }
    else if ((encodedData & 0x08) != 0) {
      type = SERIALIZABLE_FIELD_TYPE::kReflectable;
    }
    else if ((encodedData & 0x04) != 0) {
      type = SERIALIZABLE_FIELD_TYPE::kDataBlock;
    }
    else {
      type = SERIALIZABLE_FIELD_TYPE::kPlain;
    }

    array = (encodedData & 0x02) != 0;
    size = static_cast<uint8>((encodedData >> 8) & 0xFF);
    id = static_cast<uint16>((encodedData >> 16) & 0xFFFF);
  }

  BinarySerializer::ObjectMetaData
  BinarySerializer::encodeObjectMetaData(uint32 objId, uint32 objTypeId, bool isBaseClass) {
    // If O == 1 - Meta contains object instance information (Encoded by encodeObjectMetaData)
    //// Encoding: SSSS SSSS SSSS SSSS xxxx xxxx xxxx xxBO
    //// S - Size of the object identifier
    //// O - Object descriptor
    //// B - Base class indicator

    if (objId > 1073741823) {
      GE_EXCEPT(InvalidParametersException,
                "Object ID is larger than we can store (max 30 bits): " +
                toString(objId));
    }

    ObjectMetaData metaData;
    metaData.objectMeta = (objId << 2) | (isBaseClass ? 0x02 : 0) | 0x01;
    metaData.typeId = objTypeId;
    return metaData;
  }

  void
  BinarySerializer::decodeObjectMetaData(BinarySerializer::ObjectMetaData encodedData,
                                         uint32& objId,
                                         uint32& objTypeId,
                                         bool& isBaseClass) {
    if (!isObjectMetaData(encodedData.objectMeta)) {
      GE_EXCEPT(InternalErrorException,
                "Meta data represents a field description but is trying to be "
                "decoded as an object descriptor.");
    }

    objId = (encodedData.objectMeta >> 2) & 0x3FFFFFFF;
    isBaseClass = (encodedData.objectMeta & 0x02) != 0;
    objTypeId = encodedData.typeId;
  }

  bool
  BinarySerializer::isObjectMetaData(uint32 encodedData) {
    return ((encodedData & 0x01) != 0);
  }

  uint8*
  BinarySerializer::dataBlockToBuffer(uint8* data,
                              uint32 size,
                              uint8* buffer,
                              uint32& bufferLength,
                              uint32* bytesWritten,
                              function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback) {
    uint32 remainingSize = size;
    while (remainingSize > 0) {
      uint32 remainingSpaceInBuffer = bufferLength - *bytesWritten;

      if (remainingSize <= remainingSpaceInBuffer) {
        COPY_TO_BUFFER(data, remainingSize);
        remainingSize = 0;
      }
      else {
        memcpy(buffer, data, remainingSpaceInBuffer);
        buffer += remainingSpaceInBuffer;
        *bytesWritten += remainingSpaceInBuffer;
        data += remainingSpaceInBuffer;
        remainingSize -= remainingSpaceInBuffer;

        m_totalBytesWritten += *bytesWritten;
        buffer = flushBufferCallback(buffer - *bytesWritten, *bytesWritten, bufferLength);
        if (nullptr == buffer || 0 == bufferLength) {
          return nullptr;
        }
        *bytesWritten = 0;
      }
    }

    return buffer;
  }

  uint32
  BinarySerializer::findOrCreatePersistentId(IReflectable* object) {
    auto ptrAddress = reinterpret_cast<void*>(object);

    auto findIter = m_objectAddrToId.find(ptrAddress);
    if (m_objectAddrToId.end() != findIter) {
      return findIter->second;
    }

    uint32 objId = m_lastUsedObjectId++;
    m_objectAddrToId.insert(make_pair(ptrAddress, objId));

    return objId;
  }

  uint32
  BinarySerializer::registerObjectPtr(SPtr<IReflectable> object) {
    if (nullptr == object) {
      return 0;
    }

    auto ptrAddress = reinterpret_cast<void*>(object.get());

    auto iterFind = m_objectAddrToId.find(ptrAddress);
    if (m_objectAddrToId.end() == iterFind) {
      uint32 objId = findOrCreatePersistentId(object.get());

      m_objectsToEncode.emplace_back(objId, object);
      m_objectAddrToId.insert(make_pair(ptrAddress, objId));

      return objId;
    }

    return iterFind->second;
  }
}

#undef COPY_TO_BUFFER
