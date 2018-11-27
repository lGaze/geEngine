/*****************************************************************************/
/**
 * @file    geRTTIPrerequisites.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2015/02/09
 * @brief   Run-time type information utilities
 *
 * Utilities and functions created for RTTI purposes
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "geMemoryAllocator.h"
#include "geFwdDeclUtil.h"
#include "gePlatformTypes.h"


namespace geEngineSDK {
  using std::declval;
  using std::true_type;
  using std::false_type;
  using std::is_same;

  /**
   * @brief Template that you may specialize with a class if you want to provide simple
   *        serialization for it.
   *
   *        Any type that uses the "plain" field in the RTTI system must
   *        specialize this class.
   *
   * @note  Normally you will want to implement IReflectable interface if you want to provide
   *        serialization as that interface properly handles versioning, nested objects,
   *        pointer handling and more.
   *
   *        This class is useful for types you can easily serialize using a memcpy
   *        (built-in types like int/float/etc), or types you cannot modify so they implement
   *        IReflectable interface (like std::string or std::vector).
   *
   * @see   RTTIType
   * @see   RTTIField
   */
  template<class T>
  struct RTTIPlainType
  {
    static_assert(std::is_pod<T>::value,
                  "Provided type isn't plain-old-data. You need to specialize RTTIPlainType" \
                  "template in order to serialize this type."                                \
                  "(Or call GE_ALLOW_MEMCPY_SERIALIZATION(type) macro if you are sure the"   \
                  "type can be properly serialized using just memcpy.)");

    enum {
      /**
       * Unique id for the serializable type.
       */
      kID = 0
    };

    enum {
      /**
       * 0 (Object has static size less than 255 bytes, e.g. int)
       * 1 (Dynamic size with no size restriction, e.g. string)
       */
      kHasDynamicSize = 0
    };

    /**
     * @brief Serializes the provided object into the provided pre-allocated memory buffer.
     */
    static void
    toMemory(const T& data, char* memory) {
      memcpy(memory, &data, sizeof(T));
    }

    /**
     * @brief Deserializes a previously allocated object from the provided memory buffer.
     *        Return the number of bytes read from the memory buffer.
     */
    static uint32
    fromMemory(T& data, char* memory) {
      memcpy(&data, memory, sizeof(T));
      return static_cast<uint32>(sizeof(T));
    }

    /**
     * @brief Returns the size of the provided object.
     *        (Works for both static and dynamic size types)
     */
    static uint32
    getDynamicSize(const T&) {
      return static_cast<uint32>(sizeof(T));
    }
  };

  /**
   * @brief Helper method when serializing known data types that have valid
   *        RTTIPlainType specialization.
   *
   *        Returns the size of the element. If elements serializable type is
   *        specialized with hasDynamicSize == true, the dynamic size is calculated,
   *        otherwise sizeof() is used.
   */
  template<class ElemType>
  uint32
  rttiGetElementSize(const ElemType& data) {
#if GE_COMPILER == GE_COMPILER_MSVC
#	pragma warning( disable : 4127 )
#endif
    if (RTTIPlainType<ElemType>::kHasDynamicSize == 1) {
      return RTTIPlainType<ElemType>::getDynamicSize(data);
    }

    return static_cast<uint32>(sizeof(ElemType));
#if GE_COMPILER == GE_COMPILER_MSVC
#	pragma warning( default: 4127 )
#endif
  }

  /**
   * @brief	Helper method when serializing known data types that have valid
   *        RTTIPlainType specialization.
   *
   *        Writes the specified data into memory, advances the memory pointer by the
   *        bytes written and returns pointer to new memory.
   */
  template<class ElemType>
  char*
  rttiWriteElement(const ElemType& data, char* memory) {
    RTTIPlainType<ElemType>::toMemory(data, memory);
    return memory + rttiGetElementSize(data);
  }

  /**
   * @brief Helper method when serializing known data types that have valid
   *        RTTIPlainType specialization.
   *
   *        Writes the specified data into memory, advances the memory pointer
   *        by the bytes written and returns pointer to new memory. Also
   *        increases the size value by the size of the written element.
   */
  template<class ElemType>
  char*
  rttiWriteElement(const ElemType& data, char* memory, uint32& size) {
    RTTIPlainType<ElemType>::toMemory(data, memory);

    uint32 elemSize = rttiGetElementSize(data);
    size += elemSize;

    return memory + elemSize;
  }

  /**
   * @brief	Helper method when serializing known data types that have valid
   *        RTTIPlainType specialization.
   *
   *        Reads the specified data into memory, advances the memory pointer
   *        by the bytes read and returns pointer to new memory.
   */
  template<class ElemType>
  char*
  rttiReadElement(ElemType& data, char* memory) {
    RTTIPlainType<ElemType>::fromMemory(data, memory);
    return memory + rttiGetElementSize(data);
  }

  /**
   * @brief Helper method when serializing known data types that have valid
   *        RTTIPlainType specialization.
   *
   *        Reads the specified data into memory, advances the memory pointer
   *        by the bytes read and returns pointer to new memory. Also increases
   *        the size value by the size of the read element.
   */
  template<class ElemType>
  char*
  rttiReadElement(ElemType& data, char* memory, uint32& size) {
    RTTIPlainType<ElemType>::fromMemory(data, memory);

    uint32 elemSize = rttiGetElementSize(data);
    size += elemSize;

    return memory + elemSize;
  }

  /**
   * @brief Helper for checking for existence of rttiEnumFields method on a
   *        class.
   */
  template<class T>
  struct has_rttiEnumFields
  {
    struct dummy {};

    template<typename C, typename P>
    static auto
    test(P* p) -> decltype(declval<C>().rttiEnumFields(*p), true_type());

    template<typename, typename>
    static std::false_type
    test(...);

    using type = decltype(test<T, dummy>(nullptr));
    static const bool value = is_same<true_type, decltype(test<T, dummy>(nullptr))>::value;
  };

  /**
   * @brief Tell the RTTI system that the specified type may be serialized
   *        just by using a memcpy.
   *
   * @note  Internally this creates a basic RTTIPlainType specialization for the type.
   *
   * @see   RTTIPlainType
   */
#define GE_ALLOW_MEMCPY_SERIALIZATION(type)                                   \
  static_assert(std::is_trivially_copyable<type>() == true,                   \
                #type " is not trivially copyable");                          \
  template<> struct RTTIPlainType<type> {                                     \
    enum {kID=0}; enum {kHasDynamicSize = 0};                                 \
    static void                                                               \
    toMemory(const type& data, char* memory) {                                \
      memcpy(memory, &data, sizeof(type));                                    \
    }                                                                         \
    static uint32                                                             \
    fromMemory(type& data, char* memory) {                                    \
      memcpy(&data, memory, sizeof(type));                                    \
      return static_cast<uint32>(sizeof(type));                               \
    }                                                                         \
    static uint32                                                             \
    getDynamicSize(const type&) {                                             \
      return static_cast<uint32>(sizeof(type));                               \
    }                                                                         \
  };

  /**
   * @brief RTTIPlainType for std::vector.
   * @see   RTTIPlainType
   */
  template<class T>
  struct RTTIPlainType<std::vector<T, StdAlloc<T>>>
  {
    enum { kID = TYPEID_UTILITY::kID_Vector }; enum { kHasDynamicSize = 1 };

    /**
     * @copydoc	RTTIPlainType::toMemory
     */
    static void
    toMemory(const std::vector<T, StdAlloc<T>>& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 numElements = static_cast<uint32>(data.size());
      memcpy(memory, &numElements, sizeof(uint32));
      memory += sizeof(uint32);
      size += sizeof(uint32);

      for (const auto& item : data) {
        uint32 elementSize = rttiGetElementSize(item);
        RTTIPlainType<T>::toMemory(item, memory);

        memory += elementSize;
        size += elementSize;
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(std::vector<T, StdAlloc<T>>& data, char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 numElements;
      memcpy(&numElements, memory, sizeof(uint32));
      memory += sizeof(uint32);

      for (uint32 i = 0; i < numElements; ++i) {
        T element;
        uint32 elementSize = RTTIPlainType<T>::fromMemory(element, memory);
        data.push_back(element);

        memory += elementSize;
      }

      return size;
    }

    /**
     * @copydoc  RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const std::vector<T, StdAlloc<T>>& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (const auto& item : data) {
        dataSize += rttiGetElementSize(item);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /**
   * @brief RTTIPlainType for std::set.
   * @see   RTTIPlainType
   */
  template<class T>
  struct RTTIPlainType<std::set<T, std::less<T>, StdAlloc<T>>>
  {
    enum { kID = TYPEID_UTILITY::kID_Set }; enum { kHasDynamicSize = 1 };

    /**
     * @copydoc RTTIPlainType::toMemory
     */
    static void
    toMemory(const std::set<T, std::less<T>, StdAlloc<T>>& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 numElements = static_cast<uint32>(data.size());
      memcpy(memory, &numElements, sizeof(uint32));
      memory += sizeof(uint32);
      size += sizeof(uint32);

      for (const auto& item : data) {
        uint32 elementSize = rttiGetElementSize(item);
        RTTIPlainType<T>::toMemory(item, memory);

        memory += elementSize;
        size += elementSize;
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc	RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(std::set<T, std::less<T>, StdAlloc<T>>& data, char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 numElements;
      memcpy(&numElements, memory, sizeof(uint32));
      memory += sizeof(uint32);

      for (uint32 i = 0; i < numElements; ++i) {
        T element;
        uint32 elementSize = RTTIPlainType<T>::fromMemory(element, memory);
        data.insert(element);

        memory += elementSize;
      }

      return size;
    }

    /**
     * @copydoc	RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const std::set<T, std::less<T>, StdAlloc<T>>& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (const auto& item : data) {
        dataSize += rttiGetElementSize(item);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /**
   * @brief RTTIPlainType for std::map.
   * @see   RTTIPlainType
   */
  template<class Key, class Value>
  struct RTTIPlainType<std::map<Key,
                                Value,
                                std::less<Key>,
                                StdAlloc<std::pair<const Key, Value>>>>
  {
    enum { kID = TYPEID_UTILITY::kID_Map }; enum { kHasDynamicSize = 1 };

    using MapType = std::map<Key,
                             Value,
                             std::less<Key>,
                             StdAlloc<std::pair<const Key, Value>>>;

    /**
     * @copydoc		RTTIPlainType::toMemory
     */
    static void
    toMemory(const std::map<Key, Value, std::less<Key>,
             StdAlloc<std::pair<const Key, Value>>>& data,
             char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 numElements = static_cast<uint32>(data.size());
      memcpy(memory, &numElements, sizeof(uint32));
      memory += sizeof(uint32);
      size += sizeof(uint32);

      for (const auto& item : data) {
        uint32 keySize = rttiGetElementSize(item.first);
        RTTIPlainType<Key>::toMemory(item.first, memory);

        memory += keySize;
        size += keySize;

        uint32 valueSize = rttiGetElementSize(item.second);
        RTTIPlainType<Value>::toMemory(item.second, memory);

        memory += valueSize;
        size += valueSize;
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc   RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(std::map<Key, Value, std::less<Key>,
               StdAlloc<std::pair<const Key, Value>>>& data,
               char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 numElements;
      memcpy(&numElements, memory, sizeof(uint32));
      memory += sizeof(uint32);

      for (uint32 i = 0; i < numElements; ++i) {
        Key key;
        uint32 keySize = RTTIPlainType<Key>::fromMemory(key, memory);
        memory += keySize;

        Value value;
        uint32 valueSize = RTTIPlainType<Value>::fromMemory(value, memory);
        memory += valueSize;

        data[key] = value;
      }

      return size;
    }

    /**
     * @copydoc   RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const std::map<Key, Value, std::less<Key>,
                   StdAlloc<std::pair<const Key, Value>>>& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (const auto& item : data) {
        dataSize += rttiGetElementSize(item.first);
        dataSize += rttiGetElementSize(item.second);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /**
   * @brief RTTIPlainType for std::unordered_map.
   * @see   RTTIPlainType
   */
  template<class Key, class Value>
  struct RTTIPlainType<std::unordered_map<Key,
                                          Value,
                                          std::hash<Key>,
                                          std::equal_to<Key>,
                                          StdAlloc<std::pair<const Key, Value>>>>
  {
    enum { kID = TYPEID_UTILITY::kID_UnorderedMap }; enum { kHasDynamicSize = 1 };

    using UnorderedMapType = std::unordered_map<Key,
                                                Value,
                                                std::hash<Key>,
                                                std::equal_to<Key>,
                                                StdAlloc<std::pair<const Key, Value>>>;

    /**
     * @copydoc   RTTIPlainType::toMemory
     */
    static void
    toMemory(const UnorderedMapType& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 numElements = static_cast<uint32>(data.size());
      memcpy(memory, &numElements, sizeof(uint32));
      memory += sizeof(uint32);
      size += sizeof(uint32);

      for (const auto& item : data) {
        uint32 keySize = rttiGetElementSize(item.first);
        RTTIPlainType<Key>::toMemory(item.first, memory);

        memory += keySize;
        size += keySize;

        uint32 valueSize = rttiGetElementSize(item.second);
        RTTIPlainType<Value>::toMemory(item.second, memory);

        memory += valueSize;
        size += valueSize;
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc   RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(UnorderedMapType& data, char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 numElements;
      memcpy(&numElements, memory, sizeof(uint32));
      memory += sizeof(uint32);

      for (uint32 i = 0; i < numElements; ++i) {
        Key key;
        uint32 keySize = RTTIPlainType<Key>::fromMemory(key, memory);
        memory += keySize;

        Value value;
        uint32 valueSize = RTTIPlainType<Value>::fromMemory(value, memory);
        memory += valueSize;

        data[key] = value;
      }

      return size;
    }

    /**
     * @copydoc   RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const UnorderedMapType& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (const auto& item : data) {
        dataSize += RTTIPlainType<Key>::getDynamicSize(item.first);
        dataSize += RTTIPlainType<Value>::getDynamicSize(item.second);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /**
   * @brief RTTIPlainType for std::unordered_set.
   * @see   RTTIPlainType
   */
  template<class Key>
  struct RTTIPlainType<std::unordered_set<Key,
                                          std::hash<Key>,
                                          std::equal_to<Key>,
                                          StdAlloc<Key>>>
  {
    enum { kID = TYPEID_UTILITY::kID_UnorderedSet }; enum { kHasDynamicSize = 1 };

    using UnorderedSetType = std::unordered_set<Key,
                                                std::hash<Key>,
                                                std::equal_to<Key>,
                                                StdAlloc<Key>>;

    /**
     * @copydoc    RTTIPlainType::toMemory
     */
    static void
    toMemory(const UnorderedSetType& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 numElements = static_cast<uint32>(data.size());
      memcpy(memory, &numElements, sizeof(uint32));
      memory += sizeof(uint32);
      size += sizeof(uint32);

      for (const auto& item : data) {
        uint32 keySize = rttiGetElementSize(item);
        RTTIPlainType<Key>::toMemory(item, memory);

        memory += keySize;
        size += keySize;
      }

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc   RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(UnorderedSetType& data, char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 numElements;
      memcpy(&numElements, memory, sizeof(uint32));
      memory += sizeof(uint32);

      for (uint32 i = 0; i<numElements; ++i) {
        Key key;
        uint32 keySize = RTTIPlainType<Key>::fromMemory(key, memory);
        memory += keySize;

        data.insert(key);
      }

      return size;
    }

    /**
     * @copydoc   RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const UnorderedSetType& data) {
      uint64 dataSize = sizeof(uint32) * 2;

      for (const auto& item : data) {
        dataSize += rttiGetElementSize(item);
      }

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };

  /**
   * @brief RTTIPlainType for std::pair.
   * @see   RTTIPlainType
   */
  template<class A, class B>
  struct RTTIPlainType<std::pair<A, B>>
  {
    enum { kID = TYPEID_UTILITY::kID_Pair }; enum { kHasDynamicSize = 1 };

    /**
     * @copydoc   RTTIPlainType::toMemory
     */
    static void
    toMemory(const std::pair<A, B>& data, char* memory) {
      uint32 size = sizeof(uint32);
      char* memoryStart = memory;
      memory += sizeof(uint32);

      uint32 firstSize = rttiGetElementSize(data.first);
      RTTIPlainType<A>::toMemory(data.first, memory);

      memory += firstSize;
      size += firstSize;

      uint32 secondSize = rttiGetElementSize(data.second);
      RTTIPlainType<B>::toMemory(data.second, memory);

      memory += secondSize;
      size += secondSize;

      memcpy(memoryStart, &size, sizeof(uint32));
    }

    /**
     * @copydoc   RTTIPlainType::fromMemory
     */
    static uint32
    fromMemory(std::pair<A, B>& data, char* memory) {
      uint32 size = 0;
      memcpy(&size, memory, sizeof(uint32));
      memory += sizeof(uint32);

      uint32 firstSize = RTTIPlainType<A>::fromMemory(data.first, memory);
      memory += firstSize;

      uint32 secondSize = RTTIPlainType<B>::fromMemory(data.second, memory);
      memory += secondSize;

      return size;
    }

    /**
     * @copydoc   RTTIPlainType::getDynamicSize
     */
    static uint32
    getDynamicSize(const std::pair<A, B>& data) {
      uint64 dataSize = sizeof(uint32);

      dataSize += rttiGetElementSize(data.first);
      dataSize += rttiGetElementSize(data.second);

      GE_ASSERT(NumLimit::MAX_UINT32 >= dataSize);

      return static_cast<uint32>(dataSize);
    }
  };
}
