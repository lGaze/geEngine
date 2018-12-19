/*****************************************************************************/
/**
 * @file    geCoreObjectSync.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/12/14
 * @brief   Helper methods used for syncing data between sim and core threads.
 *
 * Helper methods used for syncing data between the sim and core threads.
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

namespace geEngineSDK {
  using std::true_type;
  using std::false_type;
  using std::decay_t;
  using std::enable_if_t;
  using std::forward;
  using std::is_class;
  using std::is_base_of;

  namespace detail {
    /**
     * @brief Checks is the provided type a shared pointer
     */
    template<typename T>
    struct is_shared_ptr : false_type {};

    template<typename T>
    struct is_shared_ptr<SPtr<T>> : true_type {};

    /**
     * @brief Checks is the provided type a resource handle
     */
    template<typename T>
    struct is_resource_handle : false_type {};

    template<typename T>
    struct is_resource_handle<ResourceHandle<T>> : true_type {};

    /**
     * @brief Returns the underlying type if the provided type is a resource
     *        handle, or itself otherwise
     */
    template<typename T>
    struct decay_handle {
      using value = T;
    };

    template<typename T>
    struct decay_handle<ResourceHandle<T>> {
      using value = T;
    };

    /**
     * @brief Returns the underlying type if the provided type is a shared
     *        pointer, or itself otherwise
     */
    template<typename T>
    struct decay_sptr {
      using value = T;
    };

    template<typename T>
    struct decay_sptr<SPtr<T>> {
      using value = typename SPtr<T>::element_type;
    };

    template<typename T>
    using decay_all_t = typename decay_sptr<typename decay_handle<decay_t<T>>::value>::value;

    /**
     * @brief Converts a ResourceHandle to an underlying SPtr, or if the type
     *        is not a ResourceHandle it just passes it through as is.
     */

    /**
     * @brief Pass non-resource-handle types as is.
     */
    template<class T>
    T&&
    remove_handle(T&& value,
                  enable_if_t<!is_resource_handle<decay_t<T>>::value>* = 0) {
      return forward<T>(value);
    }

    /**
     * @brief Convert a resource handle to the underlying resource SPtr.
     */
    template<class T>
    decltype(((decay_t<T>*)nullptr)->getInternalPtr())
    remove_handle(T&& handle, enable_if_t<is_resource_handle<decay_t<T>>::value>* = 0) {
      if (handle.isLoaded(false)) {
        return handle.getInternalPtr();
      }

      return nullptr;
    }

    /**
     * @brief Converts a sim thread CoreObject into a core thread CoreObject.
     *        If the type is not a core-object, it is just passed through as is
     */

    /**
     * @brief Pass non-shared-pointers as is, they aren't core objects.
     */
    template<class T>
    T&&
    get_core_object(T&& value,
                    enable_if_t<!is_shared_ptr<decay_t<T>>::value>* = 0) {
      return forward<T>(value);
    }

    /**
     * @brief Pass shared-pointers to non-classes as is, they aren't core objects.
     */
    template<class T>
    T&&
    get_core_object(T&& value,
                    enable_if_t<is_shared_ptr<decay_t<T>>::value &&
                    !is_class<decay_t<typename decay_t<T>::element_type>>::value>* = 0) {
      return forward<T>(value);
    }

    /**
     * @brief Pass shared-pointers to classes that don't derive from CoreObject
     *        as is, they aren't core objects.
     */
    template<class T>
    T&&
    get_core_object(T&& value,
          enable_if_t<is_shared_ptr<decay_t<T>>::value &&
          (is_class<decay_t<typename decay_t<T>::element_type>>::value &&
          !is_base_of<CoreObject, decay_t<typename decay_t<T>::element_type>>::value)>* = 0) {
      return std::forward<T>(value);
    }

    /**
     * @brief Convert shared-pointers with classes that derive from CoreObject
     *        to their core thread variants.
     */
    template<class T>
    decltype(((decay_t<typename decay_t<T>::element_type>*)nullptr)->getCore())
    get_core_object(T&& value,
        enable_if_t<is_shared_ptr<decay_t<T>>::value &&
        (is_class<decay_t<typename decay_t<T>::element_type>>::value &&
         is_base_of<CoreObject, decay_t<typename decay_t<T>::element_type>>::value)>* = 0) {
      if (value) {
        return value->getCore();
      }

      return nullptr;
    }
  }

  /**
   * @brief Writes the provided values into the underlying buffer using
   *        rttiWriteElem(). Each write advances the buffer to the next write
   *        location. Caller is responsible for not writing out of range.
   *        As input accepts any trivially copyable types, types with
   *        RTTIPlainType specializations, any shared pointer, as well as any
   *        resource handle or CoreObject shared ptr which will automatically
   *        get converted to their core thread variants.
   *        Types that provide a rttiEnumFields() method will have that method
   *        executed with this object provided as the parameter, * allowing the
   *        type to recurse over its fields.
   */
  struct RttiCoreSyncWriter
  {
    using MyType = RttiCoreSyncWriter;

    RttiCoreSyncWriter(char** data)
      : m_writePtr(data)
    {}

    /**
     * @brief If the type offers a rttiEnumFields method, recurse into it.
     */
    template<class T>
    void
    operator()(T&& value, enable_if_t<has_rttiEnumFields<T>::value>* = 0) {
      value.rttiEnumFields(*this);
    }

    /**
     * @brief If the type doesn't offer a rttiEnumFields method,
     *        perform the write using plain serialization.
     */
    template<class T>
    void
    operator()(T&& value, enable_if_t<!has_rttiEnumFields<T>::value>* = 0) {
      writeInternal(detail::get_core_object(detail::remove_handle(forward<T>(value))));
    }

   private:
    template<class T>
    void
    writeInternal(T&& value,
                  enable_if_t<!detail::is_shared_ptr<decay_t<T>>::value>* = 0){
      (*m_writePtr) = rttiWriteElement(value, *m_writePtr);
    }

    template<class T>
    void
    writeInternal(T&& value,
                  enable_if_t<detail::is_shared_ptr<decay_t<T>>::value>* = 0) {
      using SPtrType = decay_t<T>;

      auto sptrPtr = new (*m_writePtr) SPtrType;
      *sptrPtr = (value);

      (*m_writePtr) += sizeof(SPtrType);
    }

    char** m_writePtr;
  };

  /**
   * @brief Reads values from the underlying buffer and writes them to the
   *        output object using rttiReadElem(). Each read advances the buffer
   *        to the next value. Caller is responsible for not reading out of
   *        range.
   *        
   *        As output accepts any trivially copyable types, types with
   *        RTTIPlainType specializations and any shared pointers.
   * 
   *        Types that provide a rttiEnumFields() method will have that method
   *        executed with this object provided as the parameter, allowing the
   *        type to recurse over its fields.
   */
  struct RttiCoreSyncReader
  {
    RttiCoreSyncReader(char** data)
      : m_readPtr(data)
    {}

    /**
     * @brief If the type offers a rttiEnumFields method, recurse into it.
     */
    template<class T>
    void
    operator()(T&& value,
               enable_if_t<has_rttiEnumFields<T>::value>* = 0) {
      value.rttiEnumFields(*this);
    }

    /**
     * @brief If the type doesn't offer a rttiEnumFields method, perform the
     *        read using plain serialization.
     */
    template<class T>
    void
    operator()(T&& value,
               enable_if_t<!has_rttiEnumFields<T>::value>* = 0) {
      readInternal(forward<T>(value));
    }

   private:
    template<class T>
    void
    readInternal(T&& value,
                 enable_if_t<!detail::is_shared_ptr<decay_t<T>>::value>* = 0) {
      (*m_readPtr) = rttiReadElement(value, *m_readPtr);
    }

    template<class T>
    void
    readInternal(T&& value,
                 enable_if_t<detail::is_shared_ptr<decay_t<T>>::value>* = 0) {
      using SPtrType = decay_t<T>;

      auto sptr = reinterpret_cast<SPtrType*>(*m_readPtr);
      value = *sptr;
      sptr->~SPtrType();

      (*m_readPtr) += sizeof(SPtrType);
    }

    char** m_readPtr;
  };

  /**
   * @brief Calculates size of provided values using rttiGetElemSize(). All
   *        sizes are accumulated in the location provided upon construction.
   *
   *        As input accepts any trivially copyable types, types with
   *        RTTIPlainType specializations, any shared pointers, as well as any
   *        resource handle or CoreObject shared ptr which will automatically
   *        get converted to their core thread variants.
   *
   *        Types that provide a rttiEnumFields() method will have that method
   *        executed with this object provided as the parameter, allowing the
   *        type to recurse over its fields.
   */
  struct RttiCoreSyncSize
  {
    RttiCoreSyncSize(uint32& size)
      : m_size(size)
    {}

    /**
     * @brief If the type offers a rttiEnumFields method, recurse into it.
     */
    template<class T>
    void
    operator()(T&& value,
               enable_if_t<has_rttiEnumFields<T>::value>* = 0) {
      value.rttiEnumFields(*this);
    }

    /**
     * @brief If the type doesn't offer a rttiEnumFields method, perform the
     *        read using plain serialization.
     */
    template<class T>
    void
    operator()(T&& value,
               enable_if_t<!has_rttiEnumFields<T>::value>* = 0) {
      getSizeInternal(detail::get_core_object(detail::remove_handle(forward<T>(value))));
    }

   private:
    template<class T>
    void
    getSizeInternal(T&& value,
                    enable_if_t<!detail::is_shared_ptr<decay_t<T>>::value>* = 0) {
      m_size += rttiGetElementSize(value);
    }

    template<class T>
    void
    getSizeInternal(T&& /*value*/,
                    enable_if_t<detail::is_shared_ptr<decay_t<T>>::value>* = 0) {
      using SPtrType = decay_t<T>;
      m_size += sizeof(SPtrType);
    }

    uint32& m_size;
  };

  /**
   * @brief Calculates the size of the provided object, using rules for core
   *        object syncing. Object must have rttiEnumFields() method
   *        implementation.
   */
  template<class T>
  uint32
  coreSyncGetElemSize(T& v) {
    uint32 size = 0;
    v.rttiEnumFields(RttiCoreSyncSize(size));
    return size;
  }

  /**
   * @brief Writes the provided object into the provided memory location, using
   *        rules for core object syncing. Returns the memory location after
   *        the end of the written object.
   */
  template<class T>
  char*
  coreSyncWriteElem(T& v, char* memory) {
    v.rttiEnumFields(RttiCoreSyncWriter(&memory));
    return memory;
  }

  /**
   * @brief Decodes information from the provided memory buffer and writes it
   *        into the provided object, using rules for core object syncing.
   *        Returns the memory location after the end of the read object.
   */
  template<class T>
  char*
  coreSyncReadElem(T& v, char* memory) {
    v.rttiEnumFields(RttiCoreSyncReader(&memory));
    return memory;
  }
}
