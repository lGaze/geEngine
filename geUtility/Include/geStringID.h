/*****************************************************************************/
/**
 * @file    geStringID.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2016/03/07
 * @brief   A string identifier for very fast comparisons
 *
 * A string identifier that provides very fast comparisons to other string ids.
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
#include "geSpinLock.h"

namespace geEngineSDK {
  /***************************************************************************/
  /**
   * @class StringID
   * @brief A string identifier that provides very fast comparisons to other string ids.
   * @note  Essentially a unique ID is generated for each string and then the ID is used
   *        for comparisons as if you were using an integer or an enum.
   * @note  Thread safe.
   */
  /***************************************************************************/
  class GE_UTILITY_EXPORT StringID
  {
   public:
    static const StringID NONE;

   private:
    static constexpr const int32 HASH_TABLE_SIZE = 4096;
    static constexpr const int32 MAX_CHUNK_COUNT = 50;
    static constexpr const int32 ELEMENTS_PER_CHUNK = 256;
    static constexpr const int32 STRING_SIZE = 256;

   private:
    /**
     * @brief	Helper class that performs string actions on both null terminated
     *        character arrays and standard strings.
     */
    template<class T>
    class StringIDUtil
    {
     public:
      static uint32
      size(T const&) {
        return 0;
      }

      static void
      copy(T const&, ANSICHAR*) {}
      
      static bool
      compare(T const&, ANSICHAR*) {
        return false;
      }
    };

    /**	Internal data that is shared by all instances for a specific string. */
    struct InternalData
    {
      uint32 m_id;
      InternalData* m_next;
      ANSICHAR m_chars[STRING_SIZE];
    };

    /**
     * @brief	Performs initialization of static members as soon as the library is loaded.
     */
    struct InitStatics
    {
      InitStatics();
    };

   public:
    StringID() = default;

    StringID(const ANSICHAR* name) {
      construct(name);
    }

    StringID(const String& name) {
      construct(name);
    }

    template<int N>
    StringID(const ANSICHAR name[N]) {
      construct(static_cast<const ANSICHAR*>(name));
    }

    /**
     * @brief Compare to string ids for equality. Uses fast integer comparison.
     */
    bool
    operator==(const StringID& rhs) const {
      return m_data == rhs.m_data;
    }

    /**
     * @brief Compare to string ids for inequality. Uses fast integer comparison.
     */
    bool
    operator!=(const StringID& rhs) const {
      return m_data != rhs.m_data;
    }

    /**
     * @brief Returns true if the string id has no value assigned.
     */
    bool
    empty() const {
      return m_data == nullptr;
    }

    /**
     * @brief Returns the null-terminated name of the string id.
     */
    const ANSICHAR*
    c_str() const {
      if (nullptr == m_data) {
        return nullptr;
      }

      return m_data->m_chars;
    }

    /** Returns the unique identifier of the string. */
    uint32
    id() const {
      return m_data ? m_data->m_id : -1;
    }

   private:
    /**
     * @brief Constructs a StringID object in a way that works for pointers to
     *        character arrays and standard strings.
     */
    template<class T>
    void
    construct(T const& name);

    /**
     * @brief	Calculates a hash value for the provided null-terminated string.
     */
    template<class T>
    uint32
    calcHash(T const& input) const;

    /**
     * @brief Allocates a new string entry and assigns it a unique ID.
     *        Optionally expands the chunks buffer if the new entry doesn't fit.
     */
    InternalData*
    allocEntry();

   private:
    static volatile InitStatics m_initStatics;
    static InternalData* m_stringHashTable[HASH_TABLE_SIZE];
    static InternalData* m_chunks[MAX_CHUNK_COUNT];

    static uint32 m_nextId;
    static uint32 m_numChunks;
    static SpinLock m_sync;

    InternalData* m_data = nullptr;
  };

  template<>
  struct RTTIPlainType<StringID>
  {
    enum { kID = TYPEID_UTILITY::kID_StringID }; enum { kHasDynamicSize = 1 };

    static void
    toMemory(const StringID& data, char* memory) {
      uint32 size = getDynamicSize(data);

      uint32 curSize = sizeof(uint32);
      memcpy(memory, &size, static_cast<SIZE_T>(curSize));
      memory += curSize;

      bool isEmpty = data.empty();
      memory = rttiWriteElement(isEmpty, memory);

      if (!isEmpty) {
        auto length = static_cast<uint32>(strlen(data.c_str()));
        memcpy(memory, data.c_str(), static_cast<SIZE_T>(length) * sizeof(ANSICHAR));
      }
    }

    static uint32
    fromMemory(StringID& data, char* memory) {
      uint32 size;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      bool empty = false;
      memory = rttiReadElement(empty, memory);

      if (!empty) {
        uint32 length = (size - sizeof(uint32) - sizeof(bool)) / sizeof(ANSICHAR);

        auto name = reinterpret_cast<ANSICHAR*>
                      (ge_alloc(static_cast<SIZE_T>(length) + 1));
        memcpy(name, memory, static_cast<SIZE_T>(length));
        name[length] = '\0';

        data = StringID(name);
        ge_free(name);
      }

      return size;
    }

    static uint32
    getDynamicSize(const StringID& data) {
      uint32 dataSize = sizeof(bool) + sizeof(uint32);

      bool isEmpty = data.empty();
      if (!isEmpty) {
        uint32 length = static_cast<uint32>(strlen(data.c_str()));
        dataSize += length * sizeof(ANSICHAR);
      }

      return dataSize;
    }
  };
}

namespace std {
  /**
   * Hash value generator for StringID.
   */
  template<>
  struct hash<geEngineSDK::StringID>
  {
    size_t
    operator()(const geEngineSDK::StringID& value) const {
      return static_cast<size_t>(value.id());
    }
  };
}
