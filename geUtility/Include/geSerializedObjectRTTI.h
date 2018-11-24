/*****************************************************************************/
/**
 * @file    geSerializedObjectRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   RTTI Implementations of SerializedObjects.
 *
 * RTTI Implementations of SerializedObjects.
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
#include "gePrerequisitesUtil.h"
#include "geRTTIType.h"
#include "geSerializedObject.h"
#include "geDataStream.h"

namespace geEngineSDK {
  class GE_UTILITY_EXPORT SerializedInstanceRTTI
    : public RTTIType<SerializedInstance, IReflectable, SerializedInstanceRTTI>
  {
   public:
    const String&
    getRTTIName() override {
      static String name = "SerializedInstance";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedInstance;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return nullptr;
    }
  };

  class GE_UTILITY_EXPORT SerializedFieldRTTI
    : public RTTIType<SerializedField, SerializedInstance, SerializedFieldRTTI>
  {
   private:
    SPtr<DataStream>
    getData(SerializedField* obj, uint32& size) {
      size = obj->size;
      return ge_shared_ptr_new<MemoryDataStream>(obj->value,
                                                 static_cast<SIZE_T>(obj->size),
                                                 false);
    }

    void
    setData(SerializedField* obj, const SPtr<DataStream>& value, uint32 size) {
      obj->value = reinterpret_cast<uint8*>(ge_alloc(static_cast<SIZE_T>(size)));
      obj->size = size;
      obj->ownsMemory = true;
      value->read(obj->value, static_cast<SIZE_T>(size));
    }

   public:
    SerializedFieldRTTI() {
      addDataBlockField("data",
                        0,
                        &SerializedFieldRTTI::getData,
                        &SerializedFieldRTTI::setData,
                        0);
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedField";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedField;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedField>();
    }
  };

  class GE_UTILITY_EXPORT SerializedDataBlockRTTI
    : public RTTIType<SerializedDataBlock, SerializedInstance, SerializedDataBlockRTTI>
  {
   private:
    SPtr<DataStream>
    getData(SerializedDataBlock* obj, uint32& size) {
      size = obj->size;
      obj->stream->seek(obj->offset);
      return obj->stream;
    }

    void
    setData(SerializedDataBlock* obj, const SPtr<DataStream>& value, uint32 size) {
      auto data = reinterpret_cast<uint8*>(ge_alloc(static_cast<SIZE_T>(size)));
      SPtr<MemoryDataStream>
        memStream = ge_shared_ptr_new<MemoryDataStream>(data, static_cast<SIZE_T>(size));
      value->read(data, static_cast<SIZE_T>(size));

      obj->stream = memStream;
      obj->size = size;
      obj->offset = 0;
    }

   public:
    SerializedDataBlockRTTI() {
      addDataBlockField("data",
                        0,
                        &SerializedDataBlockRTTI::getData,
                        &SerializedDataBlockRTTI::setData,
                        0);
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedDataBlock";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedDataBlock;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedDataBlock>();
    }
  };

  class GE_UTILITY_EXPORT SerializedObjectRTTI
    : public RTTIType<SerializedObject, SerializedInstance, SerializedObjectRTTI>
  {
   private:
    SerializedSubObject&
    getEntry(SerializedObject* obj, uint32 arrayIdx) {
      return obj->subObjects[arrayIdx];
    }

    void
    setEntry(SerializedObject* obj, uint32 arrayIdx, SerializedSubObject& val) {
      obj->subObjects[arrayIdx] = val;
    }

    uint32
    getNumEntries(SerializedObject* obj) {
      return static_cast<uint32>(obj->subObjects.size());
    }

    void
    setNumEntries(SerializedObject* obj, uint32 numEntries) {
      obj->subObjects = Vector<SerializedSubObject>(numEntries);
    }
   public:
    SerializedObjectRTTI() {
      addReflectableArrayField("entries",
                               1,
                               &SerializedObjectRTTI::getEntry,
                               &SerializedObjectRTTI::getNumEntries,
                               &SerializedObjectRTTI::setEntry,
                               &SerializedObjectRTTI::setNumEntries);
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedObject";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedObject;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedObject>();
    }
  };

  class GE_UTILITY_EXPORT SerializedArrayRTTI
    : public RTTIType<SerializedArray, SerializedInstance, SerializedArrayRTTI>
  {
   private:
    uint32&
    getNumElements(SerializedArray* obj) {
      return obj->numElements;
    }

    void
    setNumElements(SerializedArray* obj, uint32& val) {
      obj->numElements = val;
    }

    SerializedArrayEntry&
    getEntry(SerializedArray* obj, uint32 arrayIdx) {
      return m_sequentialEntries[arrayIdx];
    }

    void
    setEntry(SerializedArray* obj, uint32 /*arrayIdx*/, SerializedArrayEntry& val) {
      obj->entries[val.index] = val;
    }

    uint32
    getNumEntries(SerializedArray* obj) {
      return static_cast<uint32>(m_sequentialEntries.size());
    }

    void
    setNumEntries(SerializedArray* obj, uint32 /*numEntries*/) {
      obj->entries = UnorderedMap<uint32, SerializedArrayEntry>();
    }

   public:
    SerializedArrayRTTI() {
      addPlainField("numElements",
                    0,
                    &SerializedArrayRTTI::getNumElements,
                    &SerializedArrayRTTI::setNumElements);

      addReflectableArrayField("entries",
                               1,
                               &SerializedArrayRTTI::getEntry,
                               &SerializedArrayRTTI::getNumEntries,
                               &SerializedArrayRTTI::setEntry,
                               &SerializedArrayRTTI::setNumEntries);
    }

    void
    onSerializationStarted(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto serializedArray = static_cast<SerializedArray*>(obj);

      for (auto& entry : serializedArray->entries) {
        m_sequentialEntries.push_back(entry.second);
      }
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedArray";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedArray;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedArray>();
    }

   private:
    Vector<SerializedArrayEntry> m_sequentialEntries;
  };

  class GE_UTILITY_EXPORT SerializedSubObjectRTTI
    : public RTTIType<SerializedSubObject, IReflectable, SerializedSubObjectRTTI>
  {
   private:
    uint32&
    getTypeId(SerializedSubObject* obj) {
      return obj->typeId;
    }

    void
    setTypeId(SerializedSubObject* obj, uint32& val) {
      obj->typeId = val;
    }

    SerializedEntry&
    getEntry(SerializedSubObject* obj, uint32 arrayIdx) {
      return m_sequentialEntries[arrayIdx];
    }

    void
    setEntry(SerializedSubObject* obj, uint32 /*arrayIdx*/, SerializedEntry& val) {
      obj->entries[val.fieldId] = val;
    }

    uint32
    getNumEntries(SerializedSubObject* obj) {
      return static_cast<uint32>(m_sequentialEntries.size());
    }

    void
    setNumEntries(SerializedSubObject* obj, uint32 /*numEntries*/) {
      obj->entries = UnorderedMap<uint32, SerializedEntry>();
    }

   public:
    SerializedSubObjectRTTI() {
      addPlainField("typeId",
                    0,
                    &SerializedSubObjectRTTI::getTypeId,
                    &SerializedSubObjectRTTI::setTypeId);

      addReflectableArrayField("entries",
                               1,
                               &SerializedSubObjectRTTI::getEntry,
                               &SerializedSubObjectRTTI::getNumEntries,
                               &SerializedSubObjectRTTI::setEntry,
                               &SerializedSubObjectRTTI::setNumEntries);
    }

    void
    onSerializationStarted(IReflectable* obj,
                           SerializationContext* /*params*/) override {
      auto serializableObject = static_cast<SerializedSubObject*>(obj);

      for (auto& entry : serializableObject->entries) {
        m_sequentialEntries.push_back(entry.second);
      }
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedSubObject";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedSubObject;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedSubObject>();
    }

   private:
    Vector<SerializedEntry> m_sequentialEntries;
  };

  class GE_UTILITY_EXPORT SerializedEntryRTTI
    : public RTTIType<SerializedEntry, IReflectable, SerializedEntryRTTI>
  {
   private:
    uint32&
    getFieldId(SerializedEntry* obj) {
      return obj->fieldId;
    }

    void
    setFieldId(SerializedEntry* obj, uint32& val) {
      obj->fieldId = val;
    }

    SPtr<SerializedInstance>
    getSerialized(SerializedEntry* obj) {
      return obj->serialized;
    }

    void
    setSerialized(SerializedEntry* obj, SPtr<SerializedInstance> val) {
      obj->serialized = val;
    }

   public:
    SerializedEntryRTTI() {

      addPlainField("fieldId",
                    0,
                    &SerializedEntryRTTI::getFieldId,
                    &SerializedEntryRTTI::setFieldId);

      addReflectablePtrField("serialized",
                             1,
                             &SerializedEntryRTTI::getSerialized,
                             &SerializedEntryRTTI::setSerialized);
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedEntry";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedEntry;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedEntry>();
    }
  };

  class GE_UTILITY_EXPORT SerializedArrayEntryRTTI
    : public RTTIType<SerializedArrayEntry, IReflectable, SerializedArrayEntryRTTI>
  {
   private:
    uint32&
    getArrayIdx(SerializedArrayEntry* obj) {
      return obj->index;
    }

    void
    setArrayIdx(SerializedArrayEntry* obj, uint32& val) {
      obj->index = val;
    }

    SPtr<SerializedInstance>
    getSerialized(SerializedArrayEntry* obj) {
      return obj->serialized;
    }

    void
    setSerialized(SerializedArrayEntry* obj, SPtr<SerializedInstance> val) {
      obj->serialized = val;
    }

   public:
    SerializedArrayEntryRTTI() {  
      addPlainField("index",
        0,
        &SerializedArrayEntryRTTI::getArrayIdx,
        &SerializedArrayEntryRTTI::setArrayIdx,
        0);

      addReflectablePtrField("serialized",
                             1,
                             &SerializedArrayEntryRTTI::getSerialized,
                             &SerializedArrayEntryRTTI::setSerialized);
    }

    const String&
    getRTTIName() override {
      static String name = "SerializedArrayEntry";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializedArrayEntry;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<SerializedArrayEntry>();
    }
  };
}
