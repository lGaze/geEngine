/*****************************************************************************/
/**
 * @file    geCoreObjectManager.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   Manager that keeps track of all active CoreObject's.
 *
 * Manager that keeps track of all active CoreObject's.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geCoreObjectManager.h"
#include "geCoreObject.h"
#include "geCoreObjectCore.h"
#include <geException.h>
#include <geMath.h>
#include <geFrameAlloc.h>
#include <geCoreThread.h>

namespace geEngineSDK {
  using std::find;
  using std::sort;
  using std::set_difference;
  using std::inserter;
  using std::bind;
  using std::function;

  CoreObjectManager::~CoreObjectManager() {
# if GE_DEBUG_MODE
    Lock lock(m_objectsMutex);

    if (m_objects.size() > 0) {
      //All objects MUST be destroyed at this point, otherwise there might be
      //memory corruption. (Reason: This is called on application shutdown and
      //at that point we also unload any dynamic libraries, which will
      //invalidate any pointers to objects created from those libraries.
      //Therefore we require of the user to clean up all objects manually
      //before shutting down the application).
      GE_EXCEPT(InternalErrorException,
                "Core object manager shut down, but not all objects were "
                "released. Application must release ALL engine objects before "
                "shutdown.");
    }
# endif
  }

  uint64
  CoreObjectManager::generateId() {
    Lock lock(m_objectsMutex);
    return m_nextAvailableID++;
  }

  void
  CoreObjectManager::registerObject(CoreObject* object) {
    //GE_ASSERT(nullptr != object);
    Lock lock(m_objectsMutex);

    uint64 objId = object->getInternalID();
    m_objects[objId] = object;
    m_dirtyObjects[objId] = { object, -1 };
  }

  void
  CoreObjectManager::unregisterObject(CoreObject* object) {
    GE_ASSERT(nullptr != object);

    uint64 internalId = object->getInternalID();

    //If dirty, we generate sync data before it is destroyed
    {
      Lock lock(m_objectsMutex);
      bool isDirty = object->isCoreDirty() ||
                     (m_dirtyObjects.find(internalId) != m_dirtyObjects.end());

      if (isDirty) {
        SPtr<geCoreThread::CoreObject> coreObject = object->getCore();
        if (nullptr != coreObject) {
          CoreSyncData objSyncData = object->syncToCore(g_coreThread().getFrameAlloc());

          m_destroyedSyncData.emplace_back(coreObject,
                                           internalId,
                                           objSyncData);

          DirtyObjectData& dirtyObjData = m_dirtyObjects[internalId];
          dirtyObjData.syncDataId = static_cast<int32>(m_destroyedSyncData.size()) - 1;
          dirtyObjData.object = nullptr;
        }
        else {
          DirtyObjectData& dirtyObjData = m_dirtyObjects[internalId];
          dirtyObjData.syncDataId = -1;
          dirtyObjData.object = nullptr;
        }
      }

      m_objects.erase(internalId);
    }

    updateDependencies(object, nullptr);

    //Clear dependencies from dependants
    {
      Lock lock(m_objectsMutex);

      auto iterFind = m_dependants.find(internalId);
      if (iterFind != m_dependants.end()) {
        Vector<CoreObject*>& dependants = iterFind->second;
        for (auto& entry : dependants) {
          auto iterFind2 = m_dependencies.find(entry->getInternalID());
          if (iterFind2 != m_dependencies.end()) {
            Vector<CoreObject*>& dependencies = iterFind2->second;
            auto iterFind3 = find(dependencies.begin(), dependencies.end(), object);

            if (iterFind3 != dependencies.end()) {
              dependencies.erase(iterFind3);
            }

            if (dependencies.empty()) {
              m_dependencies.erase(iterFind2);
            }
          }
        }

        m_dependants.erase(iterFind);
      }

      m_dependencies.erase(internalId);
    }
  }

  void
  CoreObjectManager::notifyCoreDirty(CoreObject* object) {
    uint64 id = object->getInternalID();
    Lock lock(m_objectsMutex);
    m_dirtyObjects[id] = { object, -1 };
  }

  void
  CoreObjectManager::notifyDependenciesDirty(CoreObject* object) {
    Vector<CoreObject*> dependencies;
    object->getCoreDependencies(dependencies);
    updateDependencies(object, &dependencies);
  }

  void
  CoreObjectManager::updateDependencies(CoreObject* object,
                                        Vector<CoreObject*>* dependencies) {
    uint64 id = object->getInternalID();

    ge_frame_mark();
    {
      FrameVector<CoreObject*> toRemove;
      FrameVector<CoreObject*> toAdd;

      Lock lock(m_objectsMutex);

      //Add dependencies and clear old dependencies from dependants
      {
        if (nullptr != dependencies) {
          sort(dependencies->begin(), dependencies->end());
        }

        auto iterFind = m_dependencies.find(id);
        if (iterFind != m_dependencies.end()) {
          const Vector<CoreObject*>& oldDependencies = iterFind->second;

          if (nullptr != dependencies)
          {
            set_difference(oldDependencies.begin(),
                           oldDependencies.end(),
                           dependencies->begin(),
                           dependencies->end(),
                           inserter(toRemove, toRemove.begin()));

            set_difference(dependencies->begin(),
                           dependencies->end(),
                           oldDependencies.begin(),
                           oldDependencies.end(),
                           inserter(toAdd, toAdd.begin()));
          }
          else {
            for (auto& dependency : oldDependencies) {
              toRemove.push_back(dependency);
            }
          }

          for (auto& dependency : toRemove) {
            uint64 dependencyId = dependency->getInternalID();
            auto iterFind2 = m_dependants.find(dependencyId);

            if (iterFind2 != m_dependants.end()) {
              Vector<CoreObject*>& dependants = iterFind2->second;
              auto findIter3 = find(dependants.begin(), dependants.end(), object);
              dependants.erase(findIter3);

              if (dependants.empty()) {
                m_dependants.erase(iterFind2);
              }
            }
          }

          if (nullptr != dependencies && !dependencies->empty()) {
            m_dependencies[id] = *dependencies;
          }
          else {
            m_dependencies.erase(id);
          }
        }
        else {
          if (nullptr != dependencies && !dependencies->empty()) {
            for (auto& dependency : *dependencies) {
              toAdd.push_back(dependency);
            }
            m_dependencies[id] = *dependencies;
          }
        }
      }

      //Register dependants
      {
        for (auto& dependency : toAdd) {
          uint64 dependencyId = dependency->getInternalID();
          Vector<CoreObject*>& dependants = m_dependants[dependencyId];
          dependants.push_back(object);
        }
      }
    }
    ge_frame_clear();
  }

  void
  CoreObjectManager::syncToCore() {
    syncDownload(g_coreThread().getFrameAlloc());
    g_coreThread().queueCommand(bind(&CoreObjectManager::syncUpload, this));
  }

  void
  CoreObjectManager::syncToCore(CoreObject* object) {
    struct IndividualCoreSyncData
    {
      SPtr<geCoreThread::CoreObject> destination;
      CoreSyncData syncData;
      FrameAlloc* allocator;
    };

    Lock lock(m_objectsMutex);

    FrameAlloc* allocator = g_coreThread().getFrameAlloc();
    Vector<IndividualCoreSyncData> syncData;

    function<void(CoreObject*)> syncObject = [&](CoreObject* curObj) {
      if (!curObj->isCoreDirty()) {
        return; //We already processed it as some other object's dependency
      }

      //Sync dependencies before dependants
      //Note: I don't check for recursion. Possible infinite loop if two
      //objects are dependent on one another.

      uint64 id = curObj->getInternalID();
      auto iterFind = m_dependencies.find(id);

      if (iterFind != m_dependencies.end()) {
        const Vector<CoreObject*>& dependencies = iterFind->second;
        for (auto& dependency : dependencies) {
          syncObject(dependency);
        }
      }

      SPtr<geCoreThread::CoreObject> objectCore = curObj->getCore();
      if (nullptr == objectCore) {
        curObj->markCoreClean();
        m_dirtyObjects.erase(id);
        return;
      }

      syncData.emplace_back();
      IndividualCoreSyncData& data = syncData.back();
      data.allocator = allocator;
      data.destination = objectCore;
      data.syncData = curObj->syncToCore(allocator);

      curObj->markCoreClean();
      m_dirtyObjects.erase(id);
    };

    syncObject(object);

    function<void(const Vector<IndividualCoreSyncData>&)> callback =
      [](const Vector<IndividualCoreSyncData>& data) {
      //Traverse in reverse to sync dependencies before dependants
      for (auto riter = data.rbegin(); riter != data.rend(); ++riter) {
        const IndividualCoreSyncData& entry = *riter;
        entry.destination->syncToCore(entry.syncData);

        uint8* dataPtr = entry.syncData.getBuffer();

        if (nullptr != dataPtr) {
          entry.allocator->free(dataPtr);
        }
      }
    };

    if (!syncData.empty()) {
      g_coreThread().queueCommand(bind(callback, syncData));
    }
  }

  void
  CoreObjectManager::syncDownload(FrameAlloc* allocator) {
    Lock lock(m_objectsMutex);

    m_coreSyncData.emplace_back();
    CoreStoredSyncData& syncData = m_coreSyncData.back();

    syncData.alloc = allocator;

    //Add all objects dependant on the dirty objects
    ge_frame_mark();
    {
      FrameSet<CoreObject*> dirtyDependants;
      for (auto& objectData : m_dirtyObjects) {
        auto iterFind = m_dependants.find(objectData.first);
        if (m_dependants.end() != iterFind) {
          const Vector<CoreObject*>& dependants = iterFind->second;
          for (auto& dependant : dependants) {
            const bool wasDirty = dependant->isCoreDirty();
            
            //Let the dependant objects know their dependency changed
            CoreObject* dependency = objectData.second.object;
            dependant->onDependencyDirty(dependency, dependency->getCoreDirtyFlags());

            if (!wasDirty && dependant->isCoreDirty()) {
              dirtyDependants.insert(dependant);
            }
          }
        }
      }

      for (auto& dirtyDependant : dirtyDependants) {
        uint64 id = dirtyDependant->getInternalID();
        m_dirtyObjects[id] = { dirtyDependant, -1 };
      }
    }
    ge_frame_clear();

    //Order in which objects are recursed in matters, ones with lower ID will
    //have been created before ones with higher ones and should be updated
    //first.
    for (auto& objectData : m_dirtyObjects) {
      function<void(CoreObject*)> syncObject = [&](CoreObject* curObj) {
        if (!curObj->isCoreDirty()) {
          return; // We already processed it as some other object's dependency
        }
        //Sync dependencies before dependants
        //Note: I don't check for recursion. Possible infinite loop if two
        //objects are dependent on one another.

        uint64 id = curObj->getInternalID();
        auto iterFind = m_dependencies.find(id);

        if (iterFind != m_dependencies.end()) {
          const Vector<CoreObject*>& dependencies = iterFind->second;
          for (auto& dependency : dependencies) {
            syncObject(dependency);
          }
        }

        SPtr<geCoreThread::CoreObject> objectCore = curObj->getCore();
        if (nullptr == objectCore) {
          curObj->markCoreClean();
          return;
        }

        CoreSyncData objSyncData = curObj->syncToCore(allocator);
        curObj->markCoreClean();

        syncData.entries.emplace_back(objectCore,
                                      curObj->getInternalID(),
                                      objSyncData);
      };

      CoreObject* object = objectData.second.object;
      if (nullptr != object) {
        syncObject(object);
      }
      else {
        //Object was destroyed but we still need to sync its modifications
        //before it was destroyed
        if (-1 != objectData.second.syncDataId) {
          syncData.entries.push_back(m_destroyedSyncData[objectData.second.syncDataId]);
        }
      }
    }

    m_dirtyObjects.clear();
    m_destroyedSyncData.clear();
  }

  void
  CoreObjectManager::syncUpload() {
    Lock lock(m_objectsMutex);

    if (m_coreSyncData.empty()) {
      return;
    }

    CoreStoredSyncData& syncData = m_coreSyncData.front();

    for (auto& objSyncData : syncData.entries) {
      SPtr<geCoreThread::CoreObject> destinationObj = objSyncData.destinationObj;
      if (nullptr != destinationObj) {
        destinationObj->syncToCore(objSyncData.syncData);
      }

      uint8* data = objSyncData.syncData.getBuffer();

      if (nullptr != data) {
        syncData.alloc->free(data);
      }
    }

    syncData.entries.clear();
    m_coreSyncData.pop_front();
  }
}
