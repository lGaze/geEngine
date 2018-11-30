/*****************************************************************************/
/**
 * @file    geGameObjectManager.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   Tracks GameObject creation and destructions.
 *
 * Tracks GameObject creation and destructions. Also resolves GameObject
 * references from GameObject handles.
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
#include "geGameObject.h"
#include <geModule.h>

namespace geEngineSDK {
  using std::function;
  using std::atomic;

  /**
   * @brief Possible modes to use when deserializing games objects.
   */
  namespace GAME_OBJECT_DESERIALIZATION_MODE {
    enum E {
      /**
       * All handles will point to old ID that were restored from the
       * deserialized file.
       */
      kUseOriginalIds = 0x01,

      /**
       * All handles will point to new IDs that were given to the deserialized
       * GameObjects.
       */
      kUseNewIds = 0x02,

      /**
       * Handles pointing to GameObjects outside of the currently deserialized
       * set will attempt to be restored in case those objects are still
       * active.
       */
      kRestoreExternal = 0x04,

      /**
       * Handles pointing to GameObjects outside of the currently deserialized
       * set will be broken.
       */
      kBreakExternal = 0x08,

      /**
       * Handles pointing to GameObjects that cannot be found will not be set
       * to null.
       */
      kKeepMissing = 0x10
    };
  }

  using GODM = GAME_OBJECT_DESERIALIZATION_MODE::E;

  /**
   * @brief Tracks GameObject creation and destructions. Also resolves
   *        GameObject references from GameObject handles.
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT GameObjectManager : public Module<GameObjectManager>
  {
   public:
    GameObjectManager() = default;
    ~GameObjectManager();

    /**
     * @brief Registers a new GameObject and returns the handle to the object.
     * @param[in] object      Constructed GameObject to wrap in the handle and
     *            initialize.
     * @return  Handle to the GameObject.
     */
    GameObjectHandleBase
    registerObject(const SPtr<GameObject>& object);

    /**
     * @brief Unregisters a GameObject. Handles to this object will no longer
     *        be valid after this call. This should be called whenever a
     *        GameObject is destroyed.
     */
    void
    unregisterObject(GameObjectHandleBase& object);

    /**
     * @brief Attempts to find a GameObject handle based on the GameObject
     *        instance ID. Returns empty handle if ID cannot be found.
     */
    GameObjectHandleBase
    getObject(uint64 id) const;

    /**
     * @brief Attempts to find a GameObject handle based on the GameObject
     *        instance ID. Returns true if object with the specified ID is
     *        found, false otherwise.
     */
    bool
    tryGetObject(uint64 id, GameObjectHandleBase& object) const;

    /**
     * @brief Checks if the GameObject with the specified instance ID exists.
     */
    bool
    objectExists(uint64 id) const;

    /**
     * @brief Changes the instance ID by which an object can be retrieved by.
     * @note  Caller is required to update the object itself with the new ID.
     */
    void
    remapId(uint64 oldId, uint64 newId);

    /**
     * @brief Allocates a new unique game object ID.
     * @note  Thread safe.
     */
    uint64
    reserveId();

    /**
     * @brief Queues the object to be destroyed at the end of a GameObject
     *        update cycle.
     */
    void
    queueForDestroy(const GameObjectHandleBase& object);

    /**
     * @brief Destroys any GameObjects that were queued for destruction.
     */
    void
    destroyQueuedObjects();

    /**
     * @brief Triggered when a game object is being destroyed.
     */
    Event<void(const HGameObject&)> onDestroyed;

   private:
    atomic<uint64> m_nextAvailableID = { 1 } ; // 0 is not a valid ID
		Map<uint64, GameObjectHandleBase> m_objects;
		Map<uint64, GameObjectHandleBase> m_queuedForDestroy;

		mutable Mutex m_mutex;
  };

  /**
   * @brief Resolves game object handles and ID during deserialization of a
   *        game object hierarchy.
   */
  class GE_CORE_EXPORT GameObjectDeserializationState
  {
   private:
    /**
     * @brief Contains data for an yet unresolved game object handle.
     */
    struct UnresolvedHandle
    {
      uint64 originalInstanceId;
      GameObjectHandleBase handle;
    };

   public:
    /**
     * @brief Starts game object deserialization.
     * @param[in] options One or a combination of
     *            GameObjectDeserializationModeFlags, controlling how are game
     *            objects deserialized.
     */
    GameObjectDeserializationState(uint32 options = GODM::kUseNewIds |
                                                    GODM::kBreakExternal)
      : m_options(options)
    {}

    ~GameObjectDeserializationState();

    /**
     * @brief Queues the specified handle and resolves it when deserialization
     *        ends.
     */
    void
    registerUnresolvedHandle(uint64 originalId, GameObjectHandleBase& object);

    /**
     * @brief Notifies the system about a new deserialized game object and its
     *        original ID.
     */
    void
    registerObject(uint64 originalId, GameObjectHandleBase& object);

    /**
     * @brief Registers a callback that will be triggered when GameObject
     *        serialization ends.
     */
    void
    registerOnDeserializationEndCallback(function<void()> callback);

    /**
     * @brief Resolves all registered handles and objects, and triggers end
     *        callbacks.
     */
    void
    resolve();

   private:
    UnorderedMap<uint64, uint64> m_idMapping;
    UnorderedMap<uint64, SPtr<GameObjectHandleData>> m_unresolvedHandleData;
    UnorderedMap<uint64, GameObjectHandleBase> m_deserializedObjects;
    Vector<UnresolvedHandle> m_unresolvedHandles;
    Vector<function<void()>> m_endCallbacks;
    uint32 m_options;
  };
}
