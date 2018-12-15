/*****************************************************************************/
/**
 * @file    geResourceHandle.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/23
 * @brief   Represents a handle to a resource.
 *
 * Represents a handle to a resource. Handles are similar to smart pointers,
 * but they have two advantages:
 * - When loading a resource asynchronously you can be immediately returned the
 *   handle that you may use throughout the engine. The handle will be made
 *   valid as soon as the resource is loaded.
 * - Handles can be serialized and deserialized, therefore saving / restoring
 *   references to their original resource.
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
#include <geIReflectable.h>
#include <geUUID.h>

namespace geEngineSDK {
  using std::nullptr_t;
  using std::atomic;
  using std::memory_order_relaxed;
  using std::static_pointer_cast;

  /**
   * @brief Data that is shared between all resource handles.
   */
  struct GE_CORE_EXPORT ResourceHandleData
  {
    SPtr<Resource> m_ptr;
    UUID m_uuid;
    bool m_isCreated = false;
    atomic<uint32> m_refCount{0};
  };

  /**
   * @brief Represents a handle to a resource. Handles are similar to a smart
   *        pointers, but they have two advantages:
   *        - When loading a resource asynchronously you can be immediately
   *          returned the handle that you may use throughout the engine. The
   *          handle will be made valid as soon as the resource is loaded.
   *        - Handles can be serialized and deserialized, therefore
   *          saving/restoring references to their original resource.
   */
  class GE_CORE_EXPORT ResourceHandleBase : public IReflectable
  {
   public:
    /**
     * @brief Checks if the resource is loaded. Until resource is loaded this
     *        handle is invalid and you may not get the internal resource from
     *        it.
     * @param[in] checkDependencies If true, and if resource has any
     *            dependencies, this method will also check if they are loaded.
     */
    bool
    isLoaded(bool checkDependencies = true) const;

    /**
     * @brief Blocks the current thread until the resource is fully loaded.
     * @note  Careful not to call this on the thread that does the loading.
     */
    void
    blockUntilLoaded(bool waitForDependencies = true) const;

    /**
     * @brief Releases an internal reference to this resource held by the
     *        resources system, if there is one.
     * @see Resources::release(ResourceHandleBase&)
     */
    void
    release();

    /**
     * @brief Returns the UUID of the resource the handle is referring to.
     */
    const UUID&
    getUUID() const {
      return nullptr != m_data ? m_data->m_uuid : UUID::EMPTY;
    }

   public:
    /**
     * @brief Gets the handle data. For internal use only.
     */
    const SPtr<ResourceHandleData>&
    getHandleData() const {
      return m_data;
    }

   protected:
    ResourceHandleBase() = default;

    /**
     * @brief Destroys the resource the handle is pointing to.
     */
    void
    destroy();

    /**
     * @brief Sets the created flag to true and assigns the resource pointer.
     *        Called by the constructors, or if you constructed just using a
     *        UUID, then you need to call this manually before you can access
     *        the resource from this handle.
     * @note This is needed because two part construction is required due to
     *        multithreaded nature of resource loading.
     * @note Internal method.
     */
    void
    setHandleData(const SPtr<Resource>& ptr, const UUID& uuid);

    /**
     * @brief Clears the created flag and the resource pointer, making the
     *        handle invalid until the resource is loaded again and assigned
     *        through setHandleData().
     */
    void
    clearHandleData();

    /**
     * @brief Increments the reference count of the handle. Only to be used by
     *        Resources for keeping internal references.
     */
    void
    addInternalRef();

    /**
     * @brief Decrements the reference count of the handle. Only to be used by
     *        Resources for keeping internal references.
     */
    void
    removeInternalRef();

    /**
     * @note  All handles to the same source must share this same handle data.
     *        Otherwise things like counting number of references or replacing
     *        pointed to resource become impossible without additional logic.
     */
    SPtr<ResourceHandleData> m_data;

   private:
    friend class Resources;

    static Signal m_resourceCreatedCondition;
    static Mutex m_resourceCreatedMutex;

   protected:
    void
    throwIfNotLoaded() const;
  };

  /**
   * @copydoc ResourceHandleBase
   * Handles differences in reference counting depending if the handle is
   * normal or weak.
   */
  template<bool WeakHandle>
  class GE_CORE_EXPORT TResourceHandleBase : public ResourceHandleBase {};

  /**
   * @brief Specialization of TResourceHandleBase for weak handles.
   *        Weak handles do no reference counting.
   */
  template<>
  class GE_CORE_EXPORT TResourceHandleBase<true> : public ResourceHandleBase
  {
   protected:
    void
    addRef() {};
    
    void
    releaseRef() {};

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/
   public:
    friend class WeakResourceHandleRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  /**
   * @brief Specialization of TResourceHandleBase for normal (non-weak) handles
   */
  template<>
  class GE_CORE_EXPORT TResourceHandleBase<false> : public ResourceHandleBase
  {
   protected:
    void
    addRef() {
      if (m_data) {
        m_data->m_refCount.fetch_add(1, memory_order_relaxed);
      }
    };

    void
    releaseRef() {
      if (m_data) {
        uint32 refCount = m_data->m_refCount.fetch_sub(1, memory_order_relaxed);

        if (1 == refCount) {
          destroy();
        }
      }
    };

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/
   public:
    friend class WeakResourceHandleRTTI;
    friend class ResourceHandleRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  /**
   * @copydoc ResourceHandleBase
   */
  template<typename T, bool WeakHandle>
  class TResourceHandle : public TResourceHandleBase<WeakHandle>
  {
   public:
    TResourceHandle() = default;

    /**
     * @brief Copy constructor.
     */
    TResourceHandle(const TResourceHandle& other) {
      this->m_data = other.getHandleData();
      this->addRef();
    }

    /**
     * @brief Move constructor.
     */
    TResourceHandle(TResourceHandle&& other) = default;

    ~TResourceHandle() {
      this->releaseRef();
    }

    /**
     * @brief Converts a specific handle to generic Resource handle.
     */
    operator TResourceHandle<Resource, WeakHandle>() const {
      TResourceHandle<Resource, WeakHandle> handle;
      handle.setHandleData(this->getHandleData());
      return handle;
    }

    /**
     * @brief Returns internal resource pointer.
     * @note  Throws exception if handle is invalid.
     */
    T*
    operator->() const {
      return get();
    }

    /**
     * @brief Returns internal resource pointer and dereferences it.
     * @note  Throws exception if handle is invalid.
     */
    T&
    operator*() const {
      return *get();
    }

    /**
     * @brief Clears the handle making it invalid and releases any references
     *        held to the resource.
     */
    TResourceHandle<T, WeakHandle>&
    operator=(nullptr_t ptr) {
      this->releaseRef();
      this->m_data = nullptr;
      return *this;
    }

    /**
     * @brief Normal assignment operator.
     */
    TResourceHandle<T, WeakHandle>&
    operator=(const TResourceHandle<T, WeakHandle>& rhs) {
      setHandleData(rhs.getHandleData());
      return *this;
    }

    /**
     * @brief Move assignment.
     */
    TResourceHandle&
    operator=(TResourceHandle&& other) {
      if (this == &other) {
        return *this;
      }

      this->releaseRef();
      this->m_data = std::exchange(other.m_data, nullptr);

      return *this;
    }

    template<class _Ty>
    struct Bool_struct
    {
      int _member;
    };

    /**
     * @brief Allows direct conversion of handle to bool.
     * @note  This is needed because we can't directly convert to bool since
     *        then we can assign pointer to bool and that's weird.
     */
    operator int
    Bool_struct<T>::*() const {
      return ((nullptr != this->m_data && !this->m_data->m_uuid.empty()) ?
              &Bool_struct<T>::_member : 0);
    }

    /**
     * @brief Returns internal resource pointer and dereferences it.
     * @note  Throws exception if handle is invalid.
     */
    T*
    get() const {
      this->throwIfNotLoaded();
      return reinterpret_cast<T*>(this->m_data->m_ptr.get());
    }

    /**
     * @brief Returns the internal shared pointer to the resource.
     * @note  Throws exception if handle is invalid.
     */
    SPtr<T>
    getInternalPtr() const {
      this->throwIfNotLoaded();
      return static_pointer_cast<T>(this->m_data->m_ptr);
    }

    /**
     * @brief Converts a handle into a weak handle.
     */
    TResourceHandle<T, true>
    getWeak() const {
      TResourceHandle<T, true> handle;
      handle.setHandleData(this->getHandleData());
      return handle;
    }

   protected:
    friend Resources;
    
    template<class _T, bool _Weak>
    friend class TResourceHandle;
    
    template<class _Ty1, class _Ty2, bool Weak>
    friend TResourceHandle<_Ty1, Weak>
    static_resource_cast(const TResourceHandle<_Ty2, Weak>& other);

    /**
     * @brief Constructs a new valid handle for the provided resource with the
     *        provided UUID.
     * @note  Handle will take ownership of the provided resource pointer, so
     *        make sure you don't delete it elsewhere.
     */
    explicit TResourceHandle(T* ptr, const UUID& uuid)
      : TResourceHandleBase<WeakHandle>() {
      this->m_data = ge_shared_ptr_new<ResourceHandleData>();
      this->addRef();
      this->setHandleData(SPtr<Resource>(ptr), uuid);
    }

    /**
     * @brief Constructs an invalid handle with the specified UUID. You must
     *        call setHandleData() with the actual resource pointer to make the
     *        handle valid.
     */
    TResourceHandle(const UUID& uuid) {
      this->m_data = ge_shared_ptr_new<ResourceHandleData>();
      this->m_data->m_uuid = uuid;
      this->addRef();
    }

    /**
     * @brief Constructs a new valid handle for the provided resource with the
     *        provided UUID.
     */
    TResourceHandle(const SPtr<T> ptr, const UUID& uuid) {
      this->m_data = ge_shared_ptr_new<ResourceHandleData>();
      this->addRef();
      setHandleData(ptr, uuid);
    }

    /**
     * @brief Replaces the internal handle data pointer, effectively
     *        transforming the handle into a different handle.
     */
    void
    setHandleData(const SPtr<ResourceHandleData>& data) {
      this->releaseRef();
      this->m_data = data;
      this->addRef();
    }

    /**
     * @brief Converts a weak handle into a normal handle.
     */
    TResourceHandle<T, false>
    lock() const {
      TResourceHandle<Resource, false> handle;
      handle.setHandleData(this->getHandleData());
      return handle;
    }

    using ResourceHandleBase::setHandleData;
  };

  /**
   * @brief Checks if two handles point to the same resource.
   */
  template<class _Ty1, bool _Weak1, class _Ty2, bool _Weak2>
  bool
  operator==(const TResourceHandle<_Ty1, _Weak1>& _Left,
             const TResourceHandle<_Ty2, _Weak2>& _Right) {
    if (_Left.getHandleData() != nullptr && _Right.getHandleData() != nullptr) {
      return _Left.getHandleData()->m_ptr == _Right.getHandleData()->m_ptr;
    }
    return _Left.getHandleData() == _Right.getHandleData();
  }

  /**
   * @brief Checks if a handle is null.
   */
  template<class _Ty1, bool _Weak1, class _Ty2, bool _Weak2>
  bool
  operator==(const TResourceHandle<_Ty1, _Weak1>& _Left,
             nullptr_t  _Right) {
    return _Left.getHandleData() == nullptr ||
           _Left.getHandleData()->m_uuid.empty();
  }

  template<class _Ty1, bool _Weak1, class _Ty2, bool _Weak2>
  bool
  operator!=(const TResourceHandle<_Ty1, _Weak1>& _Left,
             const TResourceHandle<_Ty2, _Weak2>& _Right) {
    return (!(_Left == _Right));
  }

  /**
   * @copydoc ResourceHandleBase
   */
  template<typename T>
  using ResourceHandle = TResourceHandle<T, false>;

  /**
   * @copydoc ResourceHandleBase
   * Weak handles don't prevent the resource from being unloaded.
   */
  template<typename T>
  using WeakResourceHandle = TResourceHandle<T, true>;

  /**
   * @brief Casts one resource handle to another.
   */
  template<class _Ty1, class _Ty2, bool Weak>
  TResourceHandle<_Ty1, Weak>
  static_resource_cast(const TResourceHandle<_Ty2, Weak>& other) {
    TResourceHandle<_Ty1, Weak> handle;
    handle.setHandleData(other.getHandleData());
    return handle;
  }
}
