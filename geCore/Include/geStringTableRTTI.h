/*****************************************************************************/
/**
 * @file    geStringTableRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/01
 * @brief   RTTI Objects for geStringTable.
 *
 * RTTI Objects for geStringTable.
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
#include "gePrerequisitesCore.h"
#include "geRTTIType.h"
#include "geStringTable.h"
#include "geNumericLimits.h"

namespace geEngineSDK {
  class GE_CORE_EXPORT StringTableRTTI
    : public RTTIType<StringTable, Resource, StringTableRTTI>
  {
   private:
    Language&
    getActiveLanguage(StringTable* obj) {
      return obj->m_activeLanguage;
    }

    void
    setActiveLanguage(StringTable* obj, Language& val) {
      obj->m_activeLanguage = val;
    }

    LanguageData&
    getLanguageData(StringTable* obj, uint32 idx) {
      return obj->m_allLanguages[idx];
    }

    void
    setLanguageData(StringTable* obj, uint32 idx, LanguageData& val) {
      obj->m_allLanguages[idx] = val;
    }

    uint32
    getNumLanguages(StringTable* /*obj*/) {
      return static_cast<uint32>(Language::Count);
    }

    void
    setNumLanguages(StringTable* /*obj*/, uint32 /*val*/) {
      //Do nothing
    }

    UnorderedSet<String>&
    getIdentifiers(StringTable* obj) {
      return obj->m_identifiers;
    }

    void
    setIdentifiers(StringTable* obj, UnorderedSet<String>& val) {
      obj->m_identifiers = val;
    }

   public:
    StringTableRTTI() {
      addPlainField("m_activeLanguage",
                    0,
                    &StringTableRTTI::getActiveLanguage,
                    &StringTableRTTI::setActiveLanguage);
      addPlainArrayField("m_languageData",
                         1,
                         &StringTableRTTI::getLanguageData,
                         &StringTableRTTI::getNumLanguages,
                         &StringTableRTTI::setLanguageData,
                         &StringTableRTTI::setNumLanguages);
      addPlainField("m_identifiers",
                    2,
                    &StringTableRTTI::getIdentifiers,
                    &StringTableRTTI::setIdentifiers);
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto stringTable = static_cast<StringTable*>(obj);
      stringTable->setActiveLanguage(stringTable->m_activeLanguage);
    }

    const String&
    getRTTIName() override {
      static String name = "StringTable";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_StringTable;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return StringTable::_createPtr();
    }
  };

  /***************************************************************************/
  /**
   * RTTIPlainType for LanguageData.
   * @see RTTIPlainType
   */
  /***************************************************************************/
  template<>
  struct RTTIPlainType<LanguageData>
  {
    enum { kID = TYPEID_CORE::kID_LanguageData }; enum { kHasDynamicSize = 1 };

    /**
     * @copydoc RTTIPlainType::toMemory
     */
    static void
    toMemory(const LanguageData& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      auto numElements = static_cast<uint32>(data.strings.size());
      memory = rttiWriteElement(numElements, memory, size);

      for (auto& entry : data.strings) {
        memory = rttiWriteElement(entry.first, memory, size);
        memory = rttiWriteElement(*entry.second, memory, size);
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(LanguageData& data, char* memory) {
      uint32 size = 0;
      memory = rttiReadElement(size, memory);

      uint32 numElements = 0;
      memory = rttiReadElement(numElements, memory);

      data.strings.clear();
      for (uint32 i = 0; i < numElements; ++i) {
        String identifier;
        memory = rttiReadElement(identifier, memory);

        SPtr<LocalizedStringData> entryData = ge_shared_ptr_new<LocalizedStringData>();
        memory = rttiReadElement(*entryData, memory);

        data.strings[identifier] = entryData;
      }

      return size;
    }

    /**
     * @copydoc RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const LanguageData& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (auto& entry : data.strings) {
        dataSize += rttiGetElementSize(entry.first);
        dataSize += rttiGetElementSize(*entry.second);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /***************************************************************************/
  /**
   * RTTIPlainType for LocalizedStringData.
   * @see RTTIPlainType
   */
  /***************************************************************************/
  template<>
  struct RTTIPlainType<LocalizedStringData>
  {
    enum { kID = TYPEID_CORE::kID_LocalizedStringData }; enum { kHasDynamicSize = 1 };

    /**
     * @copydoc RTTIPlainType::toMemory
     */
    static void
    toMemory(const LocalizedStringData& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      memory = rttiWriteElement(data.string, memory, size);
      memory = rttiWriteElement(data.numParameters, memory, size);

      for (uint32 i = 0; i < data.numParameters; ++i) {
        memory = rttiWriteElement(data.parameterOffsets[i], memory, size);
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(LocalizedStringData& data, char* memory) {
      if (nullptr != data.parameterOffsets) {
        ge_deleteN(data.parameterOffsets, data.numParameters);
      }

      uint32 size = 0;
      memory = rttiReadElement(size, memory);

      memory = rttiReadElement(data.string, memory);
      memory = rttiReadElement(data.numParameters, memory);

      data.parameterOffsets = ge_newN<LocalizedStringData::ParamOffset>(data.numParameters);
      for (uint32 i = 0; i < data.numParameters; ++i) {
        memory = rttiReadElement(data.parameterOffsets[i], memory);
      }

      return size;
    }

    /**
     * @copydoc RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const LocalizedStringData& data) {
      uint64 dataSize = sizeof(uint32);

      dataSize += rttiGetElementSize(data.string);
      dataSize += rttiGetElementSize(data.numParameters);

      for (uint32 i = 0; i < data.numParameters; ++i) {
        dataSize = rttiGetElementSize(data.parameterOffsets[i]);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  GE_ALLOW_MEMCPY_SERIALIZATION(LocalizedStringData::ParamOffset);
}
