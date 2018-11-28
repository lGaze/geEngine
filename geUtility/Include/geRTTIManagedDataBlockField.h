/*****************************************************************************/
/**
 * @file    geRTTIManagedDataBlockField.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/06/11
 * @brief   Base class with common functionality for a managed data block field
 *
 * Base class containing common functionality for a managed data block class
 * field. Managed data blocks are just blocks of memory that may, or may not be
 * released automatically when they are no longer referenced. They are useful
 * when wanting to return some temporary data only for serialization purposes.
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

namespace geEngineSDK {
  using std::function;

  /**
   * @brief Base class containing common functionality for a managed data block
   *        class field.
   * @note  Managed data blocks are just blocks of memory that may, or may not
   *        be released automatically when they are no longer referenced. They
   *        are useful when wanting to return some temporary data only for
   *        serialization purposes.
   */
  struct RTTIManagedDataBlockFieldBase : public RTTIField
  {
    /**
     * @brief Retrieves a managed data block from the specified instance.
     */
    virtual SPtr<DataStream>
    getValue(RTTITypeBase* rtti, void* object, uint32& size) = 0;

    /**
     * @brief Sets a managed data block on the specified instance.
     */
    virtual void
    setValue(RTTITypeBase* rtti,
             void* object,
             const SPtr<DataStream>& data,
             uint32 size) = 0;
  };

  /**
   * @brief Class containing a managed data block field containing a specific
   *        type.
   */
  template <class InterfaceType, class DataType, class ObjectType>
  struct RTTIManagedDataBlockField : public RTTIManagedDataBlockFieldBase
  {
    using GetterType = SPtr<DataStream>(InterfaceType::*)(ObjectType*, uint32&);
    using SetterType = void (InterfaceType::*)(ObjectType*, const SPtr<DataStream>&, uint32);

    /**
     * @brief Initializes a field that returns a block of bytes.
     *        Can be used for serializing pretty much anything.
     * @param[in] name      Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *                      also a unique identifier we want a small data type
     *                      that can be used for efficiently serializing data
     *                      to disk and similar. It is primarily used for
     *                      compatibility between different versions of
     *                      serialized data.
     * @param[in] getter    The getter method for the field. Must be a specific
     *                      signature: SerializableDataBlock(ObjectType*)
     * @param[in] setter    The setter method for the field. Must be a specific
     *                      signature: void(ObjectType*, SerializableDataBlock)
     * @param[in] flags     Various flags you can use to specialize how systems
     *                      handle this field. See RTTIFieldFlag.
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
           SERIALIZABLE_FIELD_TYPE::kDataBlock,
           flags);
    }

    /**
     * @copydoc RTTIField::getTypeSize
     */
    uint32
    getTypeSize() override {
      return 0; //Data block types don't store size the conventional way
    }

    /**
     * @copydoc RTTIField::hasDynamicSize
     */
    bool
    hasDynamicSize() override {
      return true;
    }

    /**
     * @copydoc RTTIField::getArraySize
     */
    uint32
    getArraySize(RTTITypeBase* /*rtti*/, void* /*object*/) override {
      GE_EXCEPT(InternalErrorException,
                "Data block types don't support arrays.");
      //return 0; //Unreachable code
    }

    /**
     * @copydoc RTTIField::setArraySize
     */
    void
    setArraySize(RTTITypeBase* /*rtti*/,
                 void* /*object*/,
                 uint32 /*size*/) override {
      GE_EXCEPT(InternalErrorException,
                "Data block types don't support arrays.");
    }

    /**
     * @copydoc RTTIManagedDataBlockFieldBase::getValue
     */
    SPtr<DataStream>
    getValue(RTTITypeBase* rtti, void* object, uint32& size) override {
      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObj = static_cast<ObjectType*>(object);

      return (rttiObject->*m_valueGetter)(castObj, size);
    }

    /**
     * @copydoc RTTIManagedDataBlockFieldBase::setValue
     */
    void
    setValue(RTTITypeBase* rtti,
             void* object, const SPtr<DataStream>& value,
             uint32 size) override {
      auto rttiObject = static_cast<InterfaceType*>(rtti);
      auto castObj = static_cast<ObjectType*>(object);

      (rttiObject->*m_valueSetter)(castObj, value, size);
    }

   private:
    GetterType m_valueGetter;
    SetterType m_valueSetter;
  };
}
