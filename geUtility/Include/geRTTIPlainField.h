/*****************************************************************************/
/**
 * @file    geRTTIPlainField.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/06/11
 * @brief   Base class containing common functionality for a plain class field.
 *
 * Plain fields are considered those that may be serialized directly by copying
 * their memory. (All built-in types, strings, etc.)
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
#include "geRTTIField.h"
#include "geException.h"

namespace geEngineSDK {
  using std::move;
  using std::function;

  /**
   * @brief Base class containing common functionality for a plain class field.
   * @note  Plain fields are considered those that may be serialized directly
   *        by copying their memory. (All built-in types, strings, etc.)
   */
  struct RTTIPlainFieldBase : public RTTIField
  {
    virtual ~RTTIPlainFieldBase() = default;

    /**
     * @brief Throws an exception if the current field type and provided
     *        template types don't match.
     */
    template<class DataType>
    void
    checkType() {
      //TODO: Low priority. Because I wanted to get rid of SerializableType
      //I have no way of checking the actual type of the field and the type
      //provided to get/set methods matches
      /*
      if (mType.id != SerializableType<DataType>().id) {
        GE_EXCEPT(InternalErrorException,
                  "Invalid field type.",
                  "SerializableSimpleTypeFieldBase::checkType()");
      }
      */
    }

    /**
     * @brief Returns the unique identifier for the type owned by the field.
     */
    virtual uint32
    getTypeId() {
      return 0;
    }

    /**
     * @copydoc RTTIField::hasDynamicSize
     */
    bool
    hasDynamicSize() override {
      return false;
    }

    /**
     * @brief Gets the dynamic size of the object. If object has no dynamic
     *        size, static size of the object is returned.
     */
    virtual uint32
    getDynamicSize(RTTITypeBase*, void*) {
      return 0;
    }

    /**
     * @brief Gets the dynamic size of an array element. If the element has no
     *        dynamic size, static size of the element is returned.
     */
    virtual uint32
    getArrayElemDynamicSize(RTTITypeBase*, void*, uint32) {
      return 0;
    }

    /**
     * @brief Retrieves the value from the provided field of the provided
     *        object, and copies it into the buffer. It does not check if
     *        buffer is large enough.
     */
    virtual void
    toBuffer(RTTITypeBase* rtti, void* object, void* buffer) = 0;

    /**
     * @brief Retrieves the value at the specified array index on the provided
     *        field of the provided object, and copies it into the buffer.
     *        It does not check if buffer is large enough.
     */
    virtual void
    arrayElemToBuffer(RTTITypeBase* rtti, void* object, uint32 index, void* buffer) = 0;

    /**
     * @brief Sets the value on the provided field of the provided object.
     *        Value is copied from the buffer. It does not check the value in
     *        the buffer in any way. You must make sure buffer points to the
     *        proper location and contains the proper type.
     */
    virtual void
    fromBuffer(RTTITypeBase* rtti, void* object, void* buffer) = 0;

    /**
     * @brief Sets the value at the specified array index on the provided field
     *        of the provided object. Value is copied from the buffer. It does
     *        not check the value in the buffer in any way. You must make sure
     *        buffer points to the proper location and contains the proper type.
     */
    virtual void
    arrayElemFromBuffer(RTTITypeBase* rtti, void* object, uint32 index, void* buffer) = 0;
  };

  /**
   * @brief Represents a plain class field containing a specific type.
   */
  template <class InterfaceType, class DataType, class ObjectType>
  struct RTTIPlainField : public RTTIPlainFieldBase
  {
    using GetterType = DataType& (InterfaceType::*)(ObjectType*);
    using SetterType = void (InterfaceType::*)(ObjectType*, DataType&);

    using ArrayGetterType = DataType& (InterfaceType::*)(ObjectType*, uint32);
    using ArraySetterType = void (InterfaceType::*)(ObjectType*, uint32, DataType&);
    using ArrayGetSizeType = uint32(InterfaceType::*)(ObjectType*);
    using ArraySetSizeType = void(InterfaceType::*)(ObjectType*, uint32);

   private:
    union {
      struct
      {
        GetterType m_valueGetter;
        SetterType m_valueSetter;
      };

      struct
      {
        ArrayGetterType m_arrayGetter;
        ArraySetterType m_arraySetter;

        ArrayGetSizeType m_arraySizeGetter;
        ArraySetSizeType m_arraySizeSetter;
      };
    };

   public:

    /**
     * @brief Initializes a plain field containing a single value.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is also a unique
     *            identifier we want a small data type that can be used for efficiently
     *            serializing data to disk and similar. It is primarily used for
     *            compatibility between different versions of serialized data.
     * @param[in] getter  The getter method for the field. Must be a specific signature:
     *            DataType(ObjectType*).
     * @param[in] setter  The setter method for the field. Must be a specific signature:
     *            void(ObjectType*, DataType)
     * @param[in] flags   Various flags you can use to specialize how outside systems handle
     *            this field. See "RTTIFieldFlag".
     */
    void
    initSingle(String name,
               uint16 uniqueId,
               GetterType getter,
               SetterType setter,
               uint64 flags) {
      //Just making sure provided type has a type ID
      static_assert(sizeof(RTTIPlainType<DataType>::kID) > 0, "Type has no RTTI ID.");
      static_assert(0 != RTTIPlainType<DataType>::kHasDynamicSize || sizeof(DataType) <= 255,
                    "Trying to create a plain RTTI field with size larger than 255. "        \
                    "In order to use larger sizes for plain types please specialize "        \
                    "RTTIPlainType, set hasDynamicSize to true.");

      m_valueGetter = getter;
      m_valueSetter = setter;

      init(move(name),
           uniqueId,
           false,
           SERIALIZABLE_FIELD_TYPE::kPlain,
           flags);
    }

    /**
     * @brief Initializes a plain field containing multiple values in an array.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is also a unique
     *            identifier we want a small data type that can be used for efficiently
     *            serializing data to disk and similar. It is primarily used for
     *            compatibility between different versions of serialized data.
     * @param[in] getter    The getter method for the field. Must be a specific signature:
     *            DataType(ObjectType*, uint32)
     * @param[in] getSize   Getter method that returns the size of an array.
     *            Must be a specific signature: uint32(ObjectType*)
     * @param[in] setter    The setter method for the field. Must be a specific signature:
     *            void(ObjectType*, uint32, DataType)
     * @param[in] setSize   Setter method that allows you to resize an array. Can be null.
     *            Must be a specific signature: void(ObjectType*, uint32)
     * @param[in] flags     Various flags you can use to specialize how outside systems
     *            handle this field. See "RTTIFieldFlag".
     */
    void
    initArray(String name,
              uint16 uniqueId,
              ArrayGetterType getter,
              ArrayGetSizeType getSize,
              ArraySetterType setter,
              ArraySetSizeType setSize,
              uint64 flags) {
      //Just making sure provided type has a type ID
      static_assert(RTTIPlainType<DataType>::kID || true, "");
      static_assert(RTTIPlainType<DataType>::kHasDynamicSize != 0 || sizeof(DataType) <= 255,
                    "Trying to create a plain RTTI field with size larger than 255. "        \
                    "In order to use larger sizes for plain types please specialize "        \
                    "RTTIPlainType, set hasDynamicSize to true.");

      m_arrayGetter = getter;
      m_arraySetter = setter;
      m_arraySizeGetter = getSize;
      m_arraySizeSetter = setSize;

      init(move(name),
           uniqueId,
           true,
           SERIALIZABLE_FIELD_TYPE::kPlain,
           flags);
    }

    /**
     * @copydoc RTTIField::getTypeSize
     */
    uint32
    getTypeSize() override {
      return static_cast<uint32>(sizeof(DataType));
    }

    /**
     * @copydoc RTTIPlainFieldBase::getTypeId
     */
    uint32
    getTypeId() override {
      return RTTIPlainType<DataType>::kID;
    }

    /**
     * @copydoc RTTIPlainFieldBase::hasDynamicSize
     */
    bool
    hasDynamicSize() override {
      return 0 != RTTIPlainType<DataType>::kHasDynamicSize;
    }

    /**
     * @copydoc RTTIPlainFieldBase::getDynamicSize
     */
    uint32
    getDynamicSize(RTTITypeBase* rtti, void* object) override {
      checkIsArray(false);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      DataType value = (rttiObject->*m_valueGetter)(castObject);

      return RTTIPlainType<DataType>::getDynamicSize(value);
    }

    /**
     * @copydoc RTTIPlainFieldBase::getArrayElemDynamicSize
     */
    uint32
    getArrayElemDynamicSize(RTTITypeBase* rtti, void* object, uint32 index) override {
      checkIsArray(true);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      DataType value = (rttiObject->*m_arrayGetter)(castObject, index);

      return RTTIPlainType<DataType>::getDynamicSize(value);
    }

    /**
     * @brief Returns the size of the array managed by the field.
     */
    uint32
    getArraySize(RTTITypeBase* rtti, void* object) override {
      checkIsArray(true);

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      return (rttiObject->*m_arraySizeGetter)(castObject);
    }

    /**
     * @brief Changes the size of the array managed by the field.
     *        Array must be re-populated after.
     */
    void
    setArraySize(RTTITypeBase* rtti, void* object, uint32 size) override {
      checkIsArray(true);

      if (!m_arraySizeSetter) {
        GE_EXCEPT(InternalErrorException,
                  "Specified field (" + m_name + ") has no array size setter.");
      }

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      (rttiObject->*m_arraySizeSetter)(castObject, size);
    }

    /**
     * @copydoc RTTIPlainFieldBase::toBuffer
     */
    void
    toBuffer(RTTITypeBase* rtti, void* object, void* buffer) override {
      checkIsArray(false);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      DataType value = (rttiObject->*m_valueGetter)(castObject);

      RTTIPlainType<DataType>::toMemory(value, static_cast<char*>(buffer));
    }

    /**
     * @copydoc RTTIPlainFieldBase::arrayElemToBuffer
     */
    void
    arrayElemToBuffer(RTTITypeBase* rtti,
                      void* object,
                      uint32 index,
                      void* buffer) override {
      checkIsArray(true);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);
      DataType value = (rttiObject->*m_arrayGetter)(castObject, index);

      RTTIPlainType<DataType>::toMemory(value, static_cast<char*>(buffer));
    }

    /**
     * @copydoc RTTIPlainFieldBase::fromBuffer
     */
    void
    fromBuffer(RTTITypeBase* rtti, void* object, void* buffer) override {
      checkIsArray(false);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);

      DataType value;
      RTTIPlainType<DataType>::fromMemory(value, static_cast<char*>(buffer));

      if (!m_valueSetter) {
        GE_EXCEPT(InternalErrorException,
                  "Specified field (" + m_name + ") has no setter.");
      }

      (rttiObject->*m_valueSetter)(castObject, value);
    }

    /**
     * @copydoc RTTIPlainFieldBase::arrayElemFromBuffer
     */
    void
    arrayElemFromBuffer(RTTITypeBase* rtti,
                        void* object,
                        uint32 index,
                        void* buffer) override {
      checkIsArray(true);
      checkType<DataType>();

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);

      DataType value;
      RTTIPlainType<DataType>::fromMemory(value, static_cast<char*>(buffer));

      if (!m_arraySetter) {
        GE_EXCEPT(InternalErrorException,
                  "Specified field (" + m_name + ") has no setter.");
      }

      (rttiObject->*m_arraySetter)(castObject, index, value);
    }
  };
}
