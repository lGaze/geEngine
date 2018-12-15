/*****************************************************************************/
/**
 * @file    geCoreObject.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/29
 * @brief   Provides functionality for dealing with objects that need to exist
 *          on both simulation and core thread.
 *
 * Core objects provides functionality for dealing with objects that need to
 * exist on both simulation and core thread.
 * It handles cross-thread initialization, destruction as well as syncing data
 * between the two threads.
 *
 * It also provides a standardized way to initialize/destroy objects, and a way
 * to specify dependent CoreObject's. For those purposes it might also be used
 * for objects that only exist on the core thread.
 *
 * @note  geCoreThread::CoreObject is a counterpart to CoreObject that is used
 *        exclusively on the core thread. CoreObject on the other hand should
 *        be used exclusively on the simulation thread. Types that exist on
 *        both threads need to implement both of these.
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
#include "geCoreObjectCore.h"
#include <geAsyncOp.h>

namespace geEngineSDK {
  using std::function;
  using std::weak_ptr;
  using std::forward;

  /**
   * @brief Values that represent current state of the core object
   */
  namespace CGO_FLAGS {
    enum E {
      kDestroyed = 0x01,
      kInitOnCoreThread = 0x02,
      kInitialized = 0x04,
    };
  }

  class GE_CORE_EXPORT CoreObject
  {
   public:
    /**
     * @brief Frees all the data held by this object.
     * @note  If this object require initialization on core thread destruction
     *        is not done immediately, and is instead just scheduled on the
     *        core thread. Otherwise the object is destroyed immediately.
     */
    virtual void
    destroy();

    /**
     * @brief Initializes all the internal resources of this object. Must be
     *        called right after construction. Generally you should call this
     *        from a factory method to avoid the issue where user forgets to
     *        call it.
     * @note  If this object require initialization on core thread
     *        initialization is not done immediately, and is instead just
     *        scheduled on the core thread. Otherwise the object is initialized
     *        immediately.
     */
    virtual void
    initialize();

    /**
     * @brief Returns true if the object has been initialized. Non-initialized
     *        object should not be used.
     */
    bool
    isInitialized() const {
      return (m_flags & CGO_FLAGS::kInitialized) != 0;
    }

    /**
     * @brief Returns true if the object has been destroyed. Destroyed object
     *        should not be used.
     */
    bool
    isDestroyed() const {
      return (m_flags & CGO_FLAGS::kDestroyed) != 0;
    }

    /**
     * @brief Blocks the current thread until the resource is fully initialized
     * @note  If you call this without calling initialize first a deadlock will
     *        occur. You should not call this from core thread.
     */
    void
    blockUntilCoreInitialized() const;

    /**
     * @brief Returns an unique identifier for this object.
     */
    uint64
    getInternalID() const {
      return m_internalID;
    }

    /**
     * @brief Returns a shared_ptr version of "this" pointer.
     */
    SPtr<CoreObject>
    getThisPtr() const {
      return m_this.lock();
    }

    /**
     * @brief Returns an object that contains a core thread specific
     *        implementation of this CoreObject. NULL is a valid return value
     *        in case object requires no core thread implementation.
     * @note  Thread safe to retrieve, but its data is only valid on the core
     *        thread.
     */
    SPtr<geCoreThread::CoreObject>
    getCore() const {
      return m_coreSpecific;
    }

    /**
     * @brief Ensures all dirty syncable data is send to the core thread
     *        counterpart of this object (if any).
     * @note  Call this if you have modified the object and need to make sure
     *        core thread has an up to date version. Normally this is done
     *        automatically at the end of a frame.
     * @note  This is an @ref asyncMethod "asynchronous method".
     */
    void
    syncToCore();

   public:
    /**
     * @brief Sets a shared this pointer to this object. This must be called
     *        immediately after construction, but before initialize().
     * @note  This should be called by the factory creation methods so user
     *        doesn't have to call it manually.
     */
    void
    _setThisPtr(SPtr<CoreObject> ptrThis);

    /**
     * @brief Schedules the object to be destroyed, and then deleted.
     */
    template<class T, class MemAlloc>
    static void
    _delete(CoreObject* obj) {
      if (!obj->isDestroyed()) {
        obj->destroy();
      }

      ge_delete<T, MemAlloc>(reinterpret_cast<T*>(obj));
    }

   protected:
    /**
     * @brief Constructs a new core object.
     * @param[in] requiresCoreInit (optional) Determines if the
     *            geCoreThread::CoreObject counterpart of this object (if it
     *            has any, see createCore()) requires initialization and
     *            destruction on the core thread.
     */
    CoreObject(bool requiresCoreInit = true);
    virtual ~CoreObject();

    /**
     * @brief Queues a command to be executed on the core thread, without a
     *        return value.
     * @note  Requires a shared pointer to the object this function will be
     *        executed on, in order to make sure the object is not deleted
     *        before the command executes. Can be null if the function is
     *        static or global.
     */
    static void
    queueGPUCommand(const SPtr<geCoreThread::CoreObject>& obj,
                    function<void()> func);

    /**
     * @brief Queues a command to be executed on the core thread, with a return
     *        value in the form of AsyncOp.
     * @see   AsyncOp
     * @note  Requires a shared pointer to the object this function will be
     *        executed on, in order to make sure the object is not deleted 
     *        before the command executes. Can be null if the function is
     *        static or global.
     */
    static AsyncOp
    queueReturnGPUCommand(const SPtr<geCoreThread::CoreObject>& obj,
                          function<void(AsyncOp&)> func);

    bool
    requiresInitOnCoreThread() const {
      return (m_flags & CGO_FLAGS::kInitOnCoreThread) != 0;
    }

    void
    setIsDestroyed(bool destroyed) {
      m_flags = destroyed ? 
        m_flags | CGO_FLAGS::kDestroyed :
        m_flags & ~CGO_FLAGS::kDestroyed;
    }

   private:
    friend class CoreObjectManager;

    volatile uint8 m_flags;
    uint32 m_coreDirtyFlags;
    uint64 m_internalID;  //0 is not a valid ID
    weak_ptr<CoreObject> m_this;

    /**
     * @brief Queues object initialization command on the core thread. The
     *        command is added to the primary core thread queue and will be
     *        executed as soon as the core thread is ready.
     */
    static void
    queueInitializeGPUCommand(const SPtr<geCoreThread::CoreObject>& obj);

    /**
     * @brief Queues object destruction command on the core thread. The command
     *        is added to the core thread queue of this thread and will be
     *        executed after these commands are submitted and any previously
     *        queued commands are executed.
     * @note	It is up to the caller to ensure no other threads attempt to use
     *        this object.
     */
    static void
    queueDestroyGPUCommand(const SPtr<geCoreThread::CoreObject>& obj);

    /**
     * @brief Helper wrapper method used for queuing commands with no return
     *        value on the core thread.
     */
    static void
    executeGPUCommand(const SPtr<geCoreThread::CoreObject>& obj,
                      function<void()> func);

    /**
     * @brief Helper wrapper method used for queuing commands with a return
     *        value on the core thread.
     */
    static void
    executeReturnGPUCommand(const SPtr<geCoreThread::CoreObject>& obj,
                            function<void(AsyncOp&)> func,
                            AsyncOp& op);

   protected:
    /*************************************************************************/
    /**
     * Core Thread Sync
     */
    /*************************************************************************/

    /**
     * @brief Creates an object that contains core thread specific data and
     *        methods for this CoreObject. Can be null if such object is not
     *        required.
     */
    virtual SPtr<geCoreThread::CoreObject>
    createCore() const {
      return nullptr;
    }

    /**
     * @brief Marks the core data as dirty. This causes the syncToCore() method
     *        to trigger the next time objects are synced between core and sim
     *        threads.
     * @param[in] flags (optional) Flags in case you want to signal that only
     *            part of the internal data is dirty.
     *            syncToCore() will be called regardless and it's up to the
     *            implementation to read the flags value if needed.
     */
    void
    markCoreDirty(uint32 flags = 0xFFFFFFFF);

    /**
     * @brief Marks the core data as clean. Normally called right after
     *        syncToCore() has been called.
     */
    void
    markCoreClean() {
      m_coreDirtyFlags = 0;
    }

    /**
     * @brief Notifies the core object manager that this object is dependant on
     *        some other CoreObject(s), and the dependencies changed since the
     *        last call to this method. This will trigger a call to
     *        getCoreDependencies() to collect the new dependencies.
     */
    void
    markDependenciesDirty();

    /**
     * @brief Checks is the core dirty flag set. This is used by external
     *        systems to know when internal data has changed and core thread
     *        potentially needs to be notified.
     */
    bool
    isCoreDirty() const {
      return m_coreDirtyFlags != 0;
    }

    /**
     * @brief Returns the exact value of the internal flag that signals whether
     *        an object needs to be synced with the core thread.
     */
    uint32
    getCoreDirtyFlags() const {
      return m_coreDirtyFlags;
    }

    /**
     * @brief Copy internal dirty data to a memory buffer that will be used for
     *        updating core thread version of that data.
     * @note  This generally happens at the end of every sim thread frame.
     *        Synced data becomes available to the core thread the start of the
     *        next core thread frame.
     */
    virtual CoreSyncData
    syncToCore(FrameAlloc* /*allocator*/) {
      return CoreSyncData();
    }

    /**
     * @brief Populates the provided array with all core objects that this core
     *        object depends upon. Dependencies are required for syncing to the
     *        core thread, so the system can be aware to update the dependant
     *        objects if a dependency is marked as dirty (for example updating
     *        a camera's viewport should also trigger an update on camera so it
     *        has a chance to potentially update its data).
     */
    virtual void
    getCoreDependencies(Vector<CoreObject*>& /*dependencies*/) {}

    /**
     * @brief Gets called on an object when one of the dependencies
     *        (as returned from getCoreDependencies()) is marked as dirty.
     *        It gives the dependant object a chance to determine should it
     *        mark itself as dirty due to the dependency change. Dirty flags of
     *        the dependency object can be examined for more information on
     *        what part of the dependency was modified.
     */
    virtual void
    onDependencyDirty(CoreObject* /*dependency*/, uint32 /*dirtyFlags*/) {
      //By default any changes on a dependency mark the parent dirty as well
      m_coreDirtyFlags |= DIRTY_DEPENDENCY_MASK;
    }

   protected:
    SPtr<geCoreThread::CoreObject> m_coreSpecific;
  };

  /**
   * @brief Creates a new core object using the specified allocators and
   *        returns a shared pointer to it.
   * @note  All core thread object shared pointers must be created using this
   *        method or its overloads and you should not create them manually.
   */
  template<class Type, class MainAlloc, class PtrDataAlloc, class... Args>
  SPtr<Type>
  ge_core_ptr_new(Args &&...args) {
    return SPtr<Type>(ge_new<Type, MainAlloc>(forward<Args>(args)...),
                                              &CoreObject::_delete<Type, MainAlloc>,
                                              StdAlloc<Type, PtrDataAlloc>());
  }

  /**
   * @brief Creates a new core object using the specified allocator and returns
   *        a shared pointer to it.
   * @note  All core thread object shared pointers must be created using this
   *        method or its overloads and you should not create them manually.
   */
  template<class Type, class MainAlloc, class... Args>
  SPtr<Type>
  ge_core_ptr_new(Args &&...args) {
    return SPtr<Type>(ge_new<Type, MainAlloc>(forward<Args>(args)...),
                                              &CoreObject::_delete<Type, MainAlloc>,
                                              StdAlloc<Type, GenAlloc>());
  }

  /**
   * @brief Creates a new core object and returns a shared pointer to it.
   * @note  All core thread object shared pointers must be created using this
   *        method or its overloads and you should not create them manually.
   */
  template<class Type, class... Args>
  SPtr<Type>
  ge_core_ptr_new(Args &&...args) {
    return SPtr<Type>(ge_new<Type, GenAlloc>(forward<Args>(args)...),
                                             &CoreObject::_delete<Type, GenAlloc>,
                                             StdAlloc<Type, GenAlloc>());
  }

  /**
   * @brief Creates a core object shared pointer using a previously constructed
   *        object.
   * @note  All core thread object shared pointers must be created using this
   *        method or its overloads and you should not create them manually.
   */
  template<class Type, class MainAlloc = GenAlloc, class PtrDataAlloc = GenAlloc>
  SPtr<Type>
  ge_core_ptr(Type* data) {
    return SPtr<Type>(data,
                      &CoreObject::_delete<Type, MainAlloc>,
                      StdAlloc<Type, PtrDataAlloc>());
  }
}
