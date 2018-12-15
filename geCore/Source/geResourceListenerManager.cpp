/*****************************************************************************/
/**
 * @file    geResourceListenerManager.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geResourceListenerManager.h"
#include "geResources.h"
#include "geIResourceListener.h"
#include "geCoreThread.h"

namespace geEngineSDK {
  using namespace std::placeholders;
  using std::bind;
  using std::find;
  using std::move;

  ResourceListenerManager::ResourceListenerManager() {
    m_resourceLoadedConn = g_resources().onResourceLoaded.connect(
      bind(&ResourceListenerManager::onResourceLoaded, this, _1));
    m_resourceModifiedConn = g_resources().onResourceModified.connect(
      bind(&ResourceListenerManager::onResourceModified, this, _1));
  }

  ResourceListenerManager::~ResourceListenerManager() {
    GE_ASSERT(m_resourceToListenerMap.empty() &&
              "Not all resource listeners had their resources unregistered "
              "properly.");

    m_resourceLoadedConn.disconnect();
    m_resourceModifiedConn.disconnect();
  }

  void
  ResourceListenerManager::registerListener(IResourceListener* listener) {
# if GE_DEBUG_MODE
    RecursiveLock lock(m_mutex);
    m_activeListeners.insert(listener);
# else
    GE_UNREFERENCED_PARAMETER(listener);
# endif
  }

  void
  ResourceListenerManager::unregisterListener(IResourceListener* listener) {
# if GE_DEBUG_MODE
    {
      RecursiveLock lock(m_mutex);
      m_activeListeners.erase(listener);
    }
# endif
    {
      RecursiveLock lock(m_mutex);
      m_dirtyListeners.erase(listener);
    }

    clearDependencies(listener);
  }

  void
  ResourceListenerManager::markListenerDirty(IResourceListener* listener) {
    RecursiveLock lock(m_mutex);
    m_dirtyListeners.insert(listener);
  }

  void
  ResourceListenerManager::update() {
    updateListeners();

    {
      RecursiveLock lock(m_mutex);

      for (auto& entry : m_loadedResources) {
        sendResourceLoaded(entry.second);
      }

      for (auto& entry : m_modifiedResources) {
        sendResourceModified(entry.second);
      }

      m_loadedResources.clear();
      m_modifiedResources.clear();
    }
  }

  void
  ResourceListenerManager::updateListeners() {
    {
      RecursiveLock lock(m_mutex);

      for (auto& listener : m_dirtyListeners) {
        m_tempListenerBuffer.push_back(listener);
      }

      m_dirtyListeners.clear();
    }

    for (auto& listener : m_tempListenerBuffer) {
      clearDependencies(listener);
      addDependencies(listener);
    }

    m_tempListenerBuffer.clear();
  }

  void
  ResourceListenerManager::notifyListeners(const UUID& resourceUUID) {
    updateListeners();
    
    HResource loadedResource;
    {
      RecursiveLock lock(m_mutex);

      const auto iterFind = m_loadedResources.find(resourceUUID);
      if (m_loadedResources.end() != iterFind) {
        loadedResource = move(iterFind->second);
        m_loadedResources.erase(iterFind);
      }
    }

    if (loadedResource) {
      sendResourceLoaded(loadedResource);
    }

    HResource modifiedResource;
    {
      RecursiveLock lock(m_mutex);

      const auto iterFind = m_modifiedResources.find(resourceUUID);
      if (iterFind != m_modifiedResources.end()) {
        modifiedResource = move(iterFind->second);
        m_modifiedResources.erase(iterFind);
      }
    }

    if (modifiedResource) {
      sendResourceModified(modifiedResource);
    }
  }

  void
  ResourceListenerManager::onResourceLoaded(const HResource& resource) {
    RecursiveLock lock(m_mutex);
    m_loadedResources[resource.getUUID()] = resource;
  }

  void
  ResourceListenerManager::onResourceModified(const HResource& resource) {
    RecursiveLock lock(m_mutex);
    m_modifiedResources[resource.getUUID()] = resource;
  }

  void
  ResourceListenerManager::sendResourceLoaded(const HResource& resource) {
    auto handleId = reinterpret_cast<uint64>(resource.getHandleData().get());

    auto iterFind = m_resourceToListenerMap.find(handleId);
    if (iterFind == m_resourceToListenerMap.end()) {
      return;
    }

    const Vector<IResourceListener*> relevantListeners = iterFind->second;
    for (auto& listener : relevantListeners) {
# if GE_DEBUG_MODE
      GE_ASSERT(m_activeListeners.find(listener) != m_activeListeners.end() &&
                "Attempting to notify a destroyed IResourceListener");
# endif
      listener->notifyResourceLoaded(resource);
    }
  }

  void
  ResourceListenerManager::sendResourceModified(const HResource& resource) {
    auto handleId = (uint64)resource.getHandleData().get();

    auto iterFind = m_resourceToListenerMap.find(handleId);
    if (iterFind == m_resourceToListenerMap.end()) {
      return;
    }

    const Vector<IResourceListener*> relevantListeners = iterFind->second;
    for (auto& listener : relevantListeners) {
# if GE_DEBUG_MODE
      GE_ASSERT(m_activeListeners.find(listener) != m_activeListeners.end() &&
                "Attempting to notify a destroyed IResourceListener");
# endif
      listener->notifyResourceChanged(resource);
    }
  }

  void
  ResourceListenerManager::clearDependencies(IResourceListener* listener) {
    auto iterFind = m_listenerToResourceMap.find(listener);
    if (iterFind == m_listenerToResourceMap.end()) {
      return;
    }

    const Vector<uint64>& dependantResources = iterFind->second;
    for (auto& resourceHandleId : dependantResources) {
      auto iterFind2 = m_resourceToListenerMap.find(resourceHandleId);
      if (iterFind2 != m_resourceToListenerMap.end()) {
        Vector<IResourceListener*>& listeners = iterFind2->second;
        auto iterFind3 = find(listeners.begin(), listeners.end(), listener);

        if (iterFind3 != listeners.end()) {
          listeners.erase(iterFind3);
        }

        if (listeners.empty()) {
          m_resourceToListenerMap.erase(iterFind2);
        }
      }
    }

    m_listenerToResourceMap.erase(iterFind);
  }

  void
  ResourceListenerManager::addDependencies(IResourceListener* listener) {
    listener->getListenerResources(m_tempResourceBuffer);

    if (!m_tempResourceBuffer.empty()) {
      Vector<uint64> resourceHandleIds(m_tempResourceBuffer.size());
      uint32 idx = 0;
      for (auto& resource : m_tempResourceBuffer) {
        auto handleId = reinterpret_cast<uint64>(resource.getHandleData().get());
        resourceHandleIds[idx] = handleId;
        m_resourceToListenerMap[handleId].push_back(listener);
        ++idx;
      }

      m_listenerToResourceMap[listener] = resourceHandleIds;
    }

    m_tempResourceBuffer.clear();
  }
}
