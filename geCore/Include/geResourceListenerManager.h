/*****************************************************************************/
/**
 * @file    geResourceListenerManager.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/30
 * @brief   Handles all active implementations of IResourceListener interface
 *          and notifies them when events they're listening to occur.
 *
 * Handles all active implementations of IResourceListener interface and
 * notifies them when events they're listening to occur.
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
#include <geModule.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT ResourceListenerManager
    : public Module<ResourceListenerManager>
  {
   public:
    ResourceListenerManager();
    ~ResourceListenerManager();

    /**
     * @brief Register a new listener to notify for events.
     * @note  Thread safe
     */
    void
    registerListener(IResourceListener* listener);

    /**
     * @brief Unregister a listener so it will no longer receive notifications.
     * @note  Thread safe
     */
    void
    unregisterListener(IResourceListener* listener);

    /**
     * @brief Marks the listener as dirty which forces the manager to updates
     *        its internal list of resources for the listener.
     * @note  Thread safe
     */
    void
    markListenerDirty(IResourceListener* listener);

    /**
     * @brief Refreshes the resource maps based on dirty listeners and sends
     *        out the necessary events.
     */
    void
    update();

    /**
     * @brief Forces the listener to send out events about the specified
     *        resource immediately, instead of waiting for the next update()
     *        call.
     */
    void
    notifyListeners(const UUID& resourceUUID);

   private:
    /**
     * @brief Refreshes the listener mapping for any listeners marked as dirty.
     */
    void
    updateListeners();

    /**
     * @brief Triggered by the resources system when a resource has finished
     *        loading.
     */
    void
    onResourceLoaded(const HResource& resource);

    /**
     * @brief Triggered by the resources system after a resource handle is
     *        modified (points to a new resource).
     */
    void
    onResourceModified(const HResource& resource);

    /**
     * @brief Sends resource loaded event to all listeners referencing this
     *        resource.
     */
    void
    sendResourceLoaded(const HResource& resource);

    /**
     * @brief Sends resource modified event to all listeners referencing this
     *        resource.
     */
    void
    sendResourceModified(const HResource& resource);

    /**
     * @brief Clears all the stored dependencies for the listener.
     */
    void
    clearDependencies(IResourceListener* listener);

    /**
     * @brief Registers all the resource dependencies for the listener.
     */
    void
    addDependencies(IResourceListener* listener);

    HEvent m_resourceLoadedConn;
    HEvent m_resourceModifiedConn;

    Set<IResourceListener*> m_dirtyListeners;
    Map<uint64, Vector<IResourceListener*>> m_resourceToListenerMap;
    Map<IResourceListener*, Vector<uint64>> m_listenerToResourceMap;

    Map<UUID, HResource> m_loadedResources;
    Map<UUID, HResource> m_modifiedResources;

    Vector<HResource> m_tempResourceBuffer;
    Vector<IResourceListener*> m_tempListenerBuffer;

    RecursiveMutex m_mutex;

# if GE_DEBUG_MODE
    Set<IResourceListener*> m_activeListeners;
# endif
  };
}
