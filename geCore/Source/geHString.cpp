/*****************************************************************************/
/**
 * @file    geHString.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/01
 * @brief   Wrapper around an Unicode string for localization purposes.
 *
 * String handle. Provides a wrapper around an Unicode string, primarily for
 * localization purposes.
 * Actual value for this string is looked up in a global string table based on
 * the provided identifier string and currently active language. If such value
 * doesn't exist then the identifier is used as is.
 *
 * Use {0}, {1}, etc. in the string for values that might change dynamically.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geHString.h"
#include "geStringTableManager.h"

namespace geEngineSDK {
  HString::HString() {
    m_stringData = StringTableManager::instance().getTable(0)->getStringData(u8"");

    if (m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
    }
  }

  HString::HString(uint32 stringTableId) {
    m_stringData = StringTableManager::instance().
                     getTable(stringTableId)->getStringData(u8"");
    if (m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
    }
  }

  HString::HString(const String& identifierString, uint32 stringTableId) {
    m_stringData = StringTableManager::instance().
                    getTable(stringTableId)->getStringData(identifierString);
    if (m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
    }
  }

  HString::HString(const String& identifierString,
                   const String& defaultString,
                   uint32 stringTableId) {
    HStringTable table = StringTableManager::instance().getTable(stringTableId);
    table->setString(identifierString, StringTable::DEFAULT_LANGUAGE, defaultString);

    m_stringData = table->getStringData(identifierString);

    if (m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
    }
  }

  HString::HString(const HString& copy)
    : m_stringData(copy.m_stringData),
      m_isDirty(copy.m_isDirty),
      m_cachedString(copy.m_cachedString) {
    if (copy.m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
      if (nullptr != copy.m_parameters) {
        for (uint32 i = 0; i < m_stringData->numParameters; ++i) {
          m_parameters[i] = copy.m_parameters[i];
        }
      }
      m_stringPtr = &m_cachedString;
    }
    else {
      m_parameters = nullptr;
      m_stringPtr = &m_stringData->string;
    }
  }

  HString::~HString() {
    if (nullptr != m_parameters) {
      ge_deleteN(m_parameters, m_stringData->numParameters);
    }
  }

  HString::operator const String& () const {
    return getValue();
  }

  HString&
  HString::operator=(const HString& rhs) {
    if (nullptr != m_parameters) {
      ge_deleteN(m_parameters, m_stringData->numParameters);
      m_parameters = nullptr;
    }

    m_stringData = rhs.m_stringData;
    m_isDirty = rhs.m_isDirty;
    m_cachedString = rhs.m_cachedString;

    if (rhs.m_stringData->numParameters > 0) {
      m_parameters = ge_newN<String>(m_stringData->numParameters);
      if (nullptr != rhs.m_parameters) {
        for (uint32 i = 0; i < m_stringData->numParameters; ++i) {
          m_parameters[i] = rhs.m_parameters[i];
        }
      }

      m_stringPtr = &m_cachedString;
    }
    else {
      m_parameters = nullptr;
      m_stringPtr = &m_stringData->string;
    }

    return *this;
  }

  const String&
  HString::getValue() const {
    if (m_isDirty) {
      if (nullptr != m_parameters) {
        m_stringData->concatenateString(m_cachedString,
                                        m_parameters,
                                        m_stringData->numParameters);
        m_stringPtr = &m_cachedString;
      }
      else {
        m_stringPtr = &m_stringData->string;
      }

      m_isDirty = false;
    }

    return *m_stringPtr;
  }

  void
  HString::setParameter(uint32 idx, const String& value) {
    if (idx >= m_stringData->numParameters) {
      return;
    }

    m_parameters[idx] = value;
    m_isDirty = true;
  }

  const HString&
  HString::dummy() {
    static HString dummyVal;
    return dummyVal;
  }
}
