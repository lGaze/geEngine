/*****************************************************************************/
/**
 * @file    geBinarySerializer.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesUtil.h"
#include "geSerializedObject.h"
#include "geRTTIField.h"

namespace geEngineSDK {
  using std::function;

  class IReflectable;
  struct RTTIReflectableFieldBase;
  struct RTTIReflectablePtrFieldBase;
  struct SerializationContext;

  class GE_UTILITY_EXPORT BinarySerializer
  {
  public:
    BinarySerializer();

    /**
     * @brief Encodes all serializable fields provided by @p object into a
     *        binary format. Data is written in chunks. Whenever a chunk is
     *        filled a callback is triggered that gives the user opportunity to
     *        expand or empty the buffer (for example write the chunk to disk)
     * @param[in]  object  Object to encode into binary format.
     * @param[out] buffer  Preallocated buffer where the data will be stored.
     * @param[in]  bufferLength  Length of the buffer, in bytes.
     * @param[out] bytesWritten  Length of the data that was actually written
     *             to the buffer, in bytes.
     * @param[in]  flushBufferCallback This callback will get called whenever
     *             the buffer gets full (Be careful to check the provided @p
     *             bytesRead variable, as buffer might not be full
     *             completely). User must then either create a new buffer or
     *             empty the existing one, and then return it by the callback.
     *             If the returned buffer address is NULL, encoding is aborted.
     * @param[in]  shallow Determines how to handle referenced objects. If true
     *             then references will not be encoded and will be set to null.
     *             If false then references will be encoded as well and
     *             restored upon decoding.
     * @param[in]  context  Optional parameters to be passed to the
     *             serialization callbacks on the objects being serialized.
     */
    void
    encode(IReflectable* object,
           uint8* buffer,
           uint32 bufferLength,
           uint32* bytesWritten,
           function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback,
           bool shallow = false,
           SerializationContext* context = nullptr);



    /**
     * @brief Decodes an object from binary data.
     * @param[in] data        Binary data to decode.
     * @param[in] dataLength  Length of the data in bytes.
     * @param[in] params      Optional parameters to be passed to the
     *            serialization callbacks on the objects being serialized.
     */
    SPtr<IReflectable>
    decode(const SPtr<DataStream>& data,
           uint32 dataLength,
           SerializationContext* context = nullptr);

   private:
    struct ObjectMetaData
    {
      uint32 objectMeta;
      uint32 typeId;
    };

    struct ObjectToEncode
    {
      ObjectToEncode(uint32 _objectId, const SPtr<IReflectable>& _object)
        : objectId(_objectId),
          object(_object)
      {}

      uint32 objectId;
      SPtr<IReflectable> object;
    };

    struct ObjectToDecode
    {
      ObjectToDecode(const SPtr<IReflectable>& _object, SIZE_T _offset = 0)
        : object(_object),
          offset(_offset)
      {}

      SPtr<IReflectable> object;
      bool isDecoded = false;
      bool decodeInProgress = false; //Used for error reporting circular references
      SIZE_T offset;
    };

    /**
     * @brief Encodes a single IReflectable object.
     */
    uint8*
    encodeEntry(IReflectable* object,
                uint32 objectId,
                uint8* buffer,
                uint32& bufferLength,
                uint32* bytesWritten,
                function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback,
                bool shallow);

    /**
     * @brief Decodes a single IReflectable object.
     */
    bool
    decodeEntry(const SPtr<DataStream>& data,
                SIZE_T dataLength,
                const SPtr<IReflectable>& output);

    /**
     * @brief Helper method for encoding a complex object and copying its data
     *        to a buffer.
     */
    uint8*
    complexTypeToBuffer(IReflectable* object,
                        uint8* buffer,
                        uint32& bufferLength,
                        uint32* bytesWritten,
                        function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback,
                        bool shallow);

    /**
     * @brief Helper method for encoding a data block to a buffer.
     */
    uint8*
    dataBlockToBuffer(uint8* data,
                      uint32 size,
                      uint8* buffer,
                      uint32& bufferLength,
                      uint32* bytesWritten,
                      function<uint8*(uint8*, uint32, uint32&)> flushBufferCallback);

    /**
     * @brief Finds an existing, or creates a unique identifier for the
     *        specified object.
     */
    uint32
    findOrCreatePersistentId(IReflectable* object);

    /**
     * @brief Finds or creates an id for the provided object and returns it.
     *        And it adds the object to a list of objects that need to be
     *        encoded, if it's not already there.
     */
    uint32
    registerObjectPtr(SPtr<IReflectable> object);

    /**
     * @brief Encodes data required for representing a serialized field, into
     *        4 bytes.
     */
    static uint32
    encodeFieldMetaData(uint16 id,
                        uint8 size,
                        bool array,
                        SERIALIZABLE_FIELD_TYPE::E type,
                        bool hasDynamicSize,
                        bool terminator);

    /**
     * @brief Decode meta field that was encoded using encodeFieldMetaData()
     */
    static void
    decodeFieldMetaData(uint32 encodedData,
                        uint16& id,
                        uint8& size,
                        bool& array,
                        SERIALIZABLE_FIELD_TYPE::E& type,
                        bool& hasDynamicSize,
                        bool& terminator);

    /**
     * @brief Encodes data required for representing an object identifier, into
     *        8 bytes.
     * @param[in] objId Unique ID of the object instance.
     * @param[in] objTypeId Unique ID of the object type.
     * @param[in] isBaseClass true if this object is base class (that is, just
     *            a part of a larger object).
     * @note  Id can be a maximum of 30 bits, as two bits are reserved.
     */
    static ObjectMetaData
    encodeObjectMetaData(uint32 objId, uint32 objTypeId, bool isBaseClass);

    /**
     * @brief Decode meta field that was encoded using encodeObjectMetaData.
     */
    static void
    decodeObjectMetaData(ObjectMetaData encodedData,
                         uint32& objId,
                         uint32& objTypeId,
                         bool& isBaseClass);

    /**
     * @brief Returns true if the provided encoded meta data represents object
     *        meta data.
     */
    static bool
    isObjectMetaData(uint32 encodedData);

    Map<uint32, ObjectToDecode> m_decodeObjectMap;
    Vector<ObjectToEncode> m_objectsToEncode;
    UnorderedMap<void*, uint32> m_objectAddrToId;
    uint32 m_lastUsedObjectId = 1;
    uint32 m_totalBytesWritten;
    FrameAlloc* m_alloc = nullptr;

    SerializationContext* m_context = nullptr;

    //Meta field size
    static constexpr const uint32 META_SIZE = 4;
    
    //Size of the field storing number of array elements
    static constexpr const uint32 NUM_ELEM_FIELD_SIZE = 4;
    
    //Size of the field storing the size of a child complex type
    static constexpr const uint32 COMPLEX_TYPE_FIELD_SIZE = 4;

    static constexpr const uint32 DATA_BLOCK_TYPE_FIELD_SIZE = 4;
  };
}
