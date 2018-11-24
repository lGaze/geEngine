/*****************************************************************************/
/**
 * @file    geRTTIType.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Provides an interface for accessing fields of a certain class.
 *
 * Provides an interface for accessing fields of a certain class.
 * Data can be easily accessed by getter and setter methods.
 *
 * Supported data types:
 * - Plain types - All types defined in geRTTIField.h, mostly native types and
 *   POD (plain old data) structures. Data is parsed byte by byte.
 *   No pointers to plain types are supported. Data is passed around by value.
 *
 * - Reflectable types - Any class deriving from IReflectable. Data is parsed
 *   based on fields in its RTTI class. Can be pointer or value type.
 *
 * - Arrays of both plain and reflectable types are supported
 *
 * - Data blocks - A managed or unmanaged block of data. See ManagedDataBlock.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geRTTIType.h"
#include "geException.h"

namespace geEngineSDK {
  using std::find_if;

  RTTITypeBase::~RTTITypeBase() {
    for (const auto& item : m_fields) {
      ge_delete(item);
    }
  }

  RTTIField*
  RTTITypeBase::findField(const String& name) {
    auto foundElement = find_if(m_fields.begin(),
                                m_fields.end(),
                                [&name](RTTIField* x) {
                                  return x->m_name == name;
                                });

    if (m_fields.end() == foundElement) {
      GE_EXCEPT(InternalErrorException,
                "Cannot find a field with the specified name: " + name);
    }

    return *foundElement;
  }

  RTTIField*
  RTTITypeBase::findField(int32 uniqueFieldId) {
    auto foundElement = find_if(m_fields.begin(),
                                m_fields.end(),
                                [&uniqueFieldId](RTTIField* x) {
                                  return x->m_uniqueId == uniqueFieldId;
                                });

    if (m_fields.end() == foundElement) {
      return nullptr;
    }

    return *foundElement;
  }

  void
  RTTITypeBase::addNewField(RTTIField* field) {
    if (nullptr == field) {
      GE_EXCEPT(InvalidParametersException, "Field argument can't be null.");
    }

    uint32 uniqueId = field->m_uniqueId;
    auto foundElementById = find_if(m_fields.begin(),
                                    m_fields.end(),
                                    [uniqueId](RTTIField* x) {
                                      return x->m_uniqueId == uniqueId;
                                    });

    if (m_fields.end() != foundElementById) {
      GE_EXCEPT(InternalErrorException,
                "Field with the same ID already exists.");
    }

    String& name = field->m_name;
    auto foundElementByName = find_if(m_fields.begin(),
                                      m_fields.end(),
                                      [&name](RTTIField* x) {
                                        return x->m_name == name;
                                      });

    if (m_fields.end() != foundElementByName) {
      GE_EXCEPT(InternalErrorException,
                "Field with the same name already exists.");
    }

    m_fields.push_back(field);
  }

  class SerializationContextRTTI
    : public RTTIType<SerializationContext,
                      IReflectable,
                      SerializationContextRTTI>
  {
    const String&
    getRTTIName() override {
      static String name = "SerializationContext";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_UTILITY::kID_SerializationContext;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      GE_EXCEPT(InternalErrorException,
                "Cannot instantiate an abstract class.");
      return nullptr;
    }
  };

  RTTITypeBase*
  SerializationContext::getRTTIStatic() {
    return SerializationContextRTTI::instance();
  }

  RTTITypeBase*
  SerializationContext::getRTTI() const {
    return getRTTIStatic();
  }

  SPtr<IReflectable> rtti_create(uint32 rttiId) {
    return IReflectable::createInstanceFromTypeId(rttiId);
  }
}
