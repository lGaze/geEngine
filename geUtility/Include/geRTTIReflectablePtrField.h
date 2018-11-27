/*****************************************************************************/
/**
 * @file    geRTTIReflectablePtrField.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Base class containing common functionality for a reflectable
 *          pointer class field.
 *
 * Reflectable fields are fields containing complex types deriving from
 * IReflectable. They are serialized recursively and you may add/remove fields
 * from them without breaking the serialized data.
 *
 * ReflectablePtr fields are different from Reflectable fields because other
 * types may reference the same Reflectable object using a ReflectablePtr,
 * while normal Reflectable fields are only referenced by a single field
 * they're declared on.
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
#include "geIReflectable.h"

namespace geEngineSDK {
  using std::function;
  using std::static_pointer_cast;

  struct RTTIReflectablePtrFieldBase : public RTTIField
  {
    /**
     * @brief Retrieves the IReflectable value from the provided instance.
     * @note  Field type must not be an array.
     */
    virtual SPtr<IReflectable>
    getValue(RTTITypeBase* rtti, void* object) = 0;

    /**
     * @brief Retrieves the IReflectable value from an array on the provided
     *        instance and index.
     * @note  Field type must be an array.
     */
    virtual SPtr<IReflectable>
    getArrayValue(RTTITypeBase* rtti, void* object, uint32 index) = 0;

    /**
     * @brief Sets the IReflectable value in the provided instance.
     * @note  Field type must not be an array.
     */
    virtual void
    setValue(RTTITypeBase* rtti, void* object, SPtr<IReflectable> value) = 0;

    /**
     * @brief Sets the IReflectable value in an array on the provided instance
     *        and index.
     * @note  Field type must be an array.
     */
    virtual void
    setArrayValue(RTTITypeBase* rtti,
                  void* object,
                  uint32 index,
                  SPtr<IReflectable> value) = 0;

    /**
     * @brief Creates a new object of the field type.
     */
    virtual SPtr<IReflectable>
    newObject() = 0;

    /**
     * @brief Returns the RTTI identifier of the class owning the field.
     */
    virtual uint32
    getRTTIId() = 0;

    /**
     * @brief Returns the name of the class owning the field.
     */
    virtual const String&
    getRTTIName() = 0;

    /**
     * @copydoc RTTIField::hasDynamicSize
     */
    bool
    hasDynamicSize() override {
      return true;
    }

    /**
     * @brief Retrieves the RTTI object for the type the field contains.
     */
    virtual RTTITypeBase*
    getType() = 0;
  };

  /**
   * @brief Reflectable field containing a pointer to a specific type with RTTI
   *        implemented.
   */
  template <class InterfaceType, class DataType, class ObjectType>
  struct RTTIReflectablePtrField : public RTTIReflectablePtrFieldBase
  {
    using GetterType = SPtr<DataType>(InterfaceType::*)(ObjectType*);
    using SetterType = void (InterfaceType::*)(ObjectType*, SPtr<DataType>);

    using ArrayGetterType = SPtr<DataType>(InterfaceType::*)(ObjectType*, uint32);
    using ArraySetterType = void (InterfaceType::*)(ObjectType*, uint32, SPtr<DataType>);
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

    /**
     * @brief Initializes a field pointing to a single data type implementing
     *        IReflectable interface.
     * @param[in] name      Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter    The getter method for the field. Must be a specific
     *            signature: DataType*(ObjectType*)
     * @param[in] setter    The setter method for the field. Must be a specific
     *            signature: void(ObjectType*, DataType*)
     * @param[in] flags     Various flags you can use to specialize how systems
     *            handle this field. See "RTTIFieldFlag".
     */
    void
    initSingle(String name,
               uint16 uniqueId,
               GetterType getter,
               SetterType setter,
               uint64 flags) {
      m_valueGetter = getter;
      m_valueSetter = setter;

      init(std::move(name),
           uniqueId,
           false,
           SERIALIZABLE_FIELD_TYPE::kReflectablePtr,
           flags);
    }

    /**
     * @brief Initializes a field containing an array of pointers to data types
     *        implementing IReflectable interface.
     * @param[in] name      Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter    The getter method for the field. Must be a specific
     *            signature: DataType*(ObjectType*, uint32)
     * @param[in] getSize   Getter method that returns the size of an array.
     *            Must be a specific signature: uint32(ObjectType*)
     * @param[in] setter    The setter method for the field. Must be a specific
     *            signature: void(ObjectType*, uint32, DataType*)
     * @param[in] setSize   Setter method that allows you to resize an array.
     *            Can be null. Must be a specific signature:
     *            void(ObjectType*, uint32)
     * @param[in] flags     Various flags you can use to specialize how systems
     *            handle this field. See "RTTIFieldFlag".
     */
    void initArray(String name,
                   uint16 uniqueId,
                   ArrayGetterType getter,
                   ArrayGetSizeType getSize,
                   ArraySetterType setter,
                   ArraySetSizeType setSize,
                   uint64 flags) {
      m_arrayGetter = getter;
      m_arraySetter = setter;
      m_arraySizeGetter = getSize;
      m_arraySizeSetter = setSize;

      init(std::move(name)
           uniqueId,
           true,
           SERIALIZABLE_FIELD_TYPE::kReflectablePtr,
           flags);
    }

    /**
     * @copydoc RTTIField::getTypeSize
     */
    uint32
    getTypeSize() override {
      return 0; //Complex types don't store size the conventional way
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::getValue
     */
    SPtr<IReflectable>
    getValue(RTTITypeBase* rtti, void* object) override {
      checkIsArray(false);

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObjType = static_cast<ObjectType*>(object);

      SPtr<IReflectable> castDataType = (rttiObject->*m_valueGetter)(castObjType);
      return castDataType;
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::getArrayValue
     */
    SPtr<IReflectable>
    getArrayValue(RTTITypeBase* rtti, void* object, uint32 index) override {
      checkIsArray(true);

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObjType = static_cast<ObjectType*>(object);

      SPtr<IReflectable> castDataType = (rttiObject->*m_arrayGetter)(castObjType, index);
      return castDataType;
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::setValue
     */
    void
    setValue(RTTITypeBase* rtti, void* object, SPtr<IReflectable> value) override {
      checkIsArray(false);

      if (!m_valueSetter) {
        GE_EXCEPT(InternalErrorException, "Specified field (" + m_name + ") has no setter.");
      }

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObjType = static_cast<ObjectType*>(object);
      SPtr<DataType> castDataObj = std::static_pointer_cast<DataType>(value);

      (rttiObject->*m_valueSetter)(castObjType, castDataObj);
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::setArrayValue
     */
    void
    setArrayValue(RTTITypeBase* rtti,
                  void* object,
                  uint32 index,
                  SPtr<IReflectable> value) override {
      checkIsArray(true);

      if (!m_arraySetter) {
        GE_EXCEPT(InternalErrorException,
                  "Specified field (" + m_name + ") has no setter.");
      }

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObjType = static_cast<ObjectType*>(object);
      SPtr<DataType> castDataObj = std::static_pointer_cast<DataType>(value);

      (rttiObject->*m_arraySetter)(castObjType, index, castDataObj);
    }

    /**
     * @copydoc RTTIField::setArraySize
     */
    uint32
    getArraySize(RTTITypeBase* rtti, void* object) override {
      checkIsArray(true);

      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObject = static_cast<ObjectType*>(object);

      return (rttiObject->*m_arraySizeGetter)(castObject);
    }

    /**
     * @copydoc RTTIField::setArraySize
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
     * @copydoc RTTIReflectablePtrFieldBase::newObject
     */
    SPtr<IReflectable>
    newObject() override {
      return SPtr<IReflectable>(DataType::getRTTIStatic()->newRTTIObject());
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::getRTTIId
     */
    uint32
    getRTTIId() override {
      return DataType::getRTTIStatic()->getRTTIId();
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::getRTTIName
     */
    const String&
    getRTTIName() override {
      return DataType::getRTTIStatic()->getRTTIName();
    }

    /**
     * @copydoc RTTIReflectablePtrFieldBase::getType
     */
    RTTITypeBase*
    getType() override {
      return DataType::getRTTIStatic();
    }
  };
}
