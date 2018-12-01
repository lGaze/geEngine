/*****************************************************************************/
/**
 * @file    geResources.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/30
 * @brief   Manager for dealing with all engine resources. It allows you to
 *          save new resources and load existing ones.
 *
 * Manager for dealing with all engine resources. It allows you to save new
 * resources and load existing ones.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geResources.h"
#include "geResource.h"
#include "geResourceManifest.h"
#include "geUtility.h"
#include "geSavedResourceData.h"
#include "geResourceListenerManager.h"

#include <geException.h>
#include <geFileSerializer.h>
#include <geFileSystem.h>
#include <geTaskScheduler.h>
#include <geUUID.h>
#include <geDebug.h>
#include <geMemorySerializer.h>
#include <geCompression.h>
#include <geDataStream.h>
#include <geBinarySerializer.h>
#include <geNumericLimits.h>

namespace geEngineSDK {
  using std::ios;
  using std::static_pointer_cast;
  using std::find;
  using std::find_if;
  using std::bind;
  using std::memory_order_relaxed;
  using std::ofstream;

  Resources::Resources() {
    {
      Lock lock(m_defaultManifestMutex);
      m_defaultResourceManifest = ResourceManifest::create("Default");
      m_resourceManifests.push_back(m_defaultResourceManifest);
    }
  }

  Resources::~Resources() {
    unloadAll();
  }

  HResource
  Resources::load(const Path& filePath, ResourceLoadFlags loadFlags) {
    if (!FileSystem::isFile(filePath)) {
      LOGWRN("Cannot load resource. Specified file: " +
             filePath.toString() +
             " doesn't exist.");
      return HResource();
    }

    UUID uuid;
    bool foundUUID = getUUIDFromFilePath(filePath, uuid);

    if (!foundUUID) {
      uuid = UUIDGenerator::generateRandom();
    }

    return loadInternal(uuid, filePath, true, loadFlags);
  }

  HResource
  Resources::load(const WeakResourceHandle<Resource>& handle,
                  ResourceLoadFlags loadFlags) {
    if (nullptr == handle.m_data) {
      return HResource();
    }

    UUID uuid = handle.getUUID();
    return loadFromUUID(uuid, false, loadFlags);
  }

  HResource
  Resources::loadAsync(const Path& filePath, ResourceLoadFlags loadFlags) {
    if (!FileSystem::isFile(filePath)) {
      LOGWRN("Cannot load resource. Specified file: " +
             filePath.toString() +
             " doesn't exist.");
      return HResource();
    }

    UUID uuid;
    bool foundUUID = getUUIDFromFilePath(filePath, uuid);

    if (!foundUUID) {
      uuid = UUIDGenerator::generateRandom();
    }

    return loadInternal(uuid, filePath, false, loadFlags);
  }

  HResource
  Resources::loadFromUUID(const UUID& uuid,
                          bool async,
                          ResourceLoadFlags loadFlags) {
    Path filePath;

    //Default manifest is at 0th index but all other take priority since
    //Default manifest could contain obsolete data.
    for (auto iter = m_resourceManifests.rbegin();
         iter != m_resourceManifests.rend();
         ++iter) {
      if ((*iter)->uuidToFilePath(uuid, filePath)) {
        break;
      }
    }

    return loadInternal(uuid, filePath, !async, loadFlags);
  }

  HResource
  Resources::loadInternal(const UUID& uuid,
                          const Path& filePath,
                          bool synchronous,
                          ResourceLoadFlags loadFlags) {
    HResource outputResource;

    //Retrieve/create resource handle, and register with the system
    bool loadInProgress = false;
    bool loadFailed = false;
    bool initiateLoad = false;
    Vector<UUID> dependenciesToLoad;
    {
      bool alreadyLoading = false;

      //Check if the resource is being loaded on a worker thread
      Lock inProgressLock(m_inProgressResourcesMutex);
      Lock loadedLock(m_loadedResourceMutex);

      auto iterFind2 = m_inProgressResources.find(uuid);
      if (iterFind2 != m_inProgressResources.end()) {
        LoadedResourceData& resData = iterFind2->second->resData;
        outputResource = resData.resource.lock();

        //Increase ref count
        if (loadFlags.isSet(RLF::kKeepInternalRef)) {
          ++resData.numInternalRefs;
          outputResource.addInternalRef();
        }

        loadInProgress = true;
        alreadyLoading = true;
      }

      //Check if the resource is already loaded
      auto iterFind = m_loadedResources.find(uuid);
      if (iterFind != m_loadedResources.end()) {
        LoadedResourceData& resData = iterFind->second;
        outputResource = resData.resource.lock();

        //Increase ref. count
        if (loadFlags.isSet(RLF::kKeepInternalRef)) {
          ++resData.numInternalRefs;
          outputResource.addInternalRef();
        }

        alreadyLoading = true;
      }

      //Not loaded and not in progress, register a new handle or find a
      //pre-registered one
      if (!alreadyLoading) {
        auto iterFindHndl = m_handles.find(uuid);
        if (iterFindHndl != m_handles.end()) {
          outputResource = iterFindHndl->second.lock();
        }
        else {
          outputResource = HResource(uuid);
          m_handles[uuid] = outputResource.getWeak();
        }
      }

      //If we have nowhere to load from, warn and complete load if a file path
      //was provided, otherwise pass through as we might just want to complete
      //a previously queued load 
      if (filePath.isEmpty()) {
        if (!alreadyLoading) {
          LOGWRN("Cannot load resource. Resource with UUID '" +
                 uuid.toString() +
                 "' doesn't exist.");
          loadFailed = true;
        }
      }
      else if (!FileSystem::isFile(filePath)) {
        LOGWRN("Cannot load resource. Specified file: " +
               filePath.toString() +
               " doesn't exist.");
        loadFailed = true;
      }

      if (!loadFailed) {
        //Load dependency data if a file path is provided
        SPtr<SavedResourceData> savedResourceData;
        if (!filePath.isEmpty()) {
          FileDecoder fs(filePath);
          savedResourceData = static_pointer_cast<SavedResourceData>(fs.decode());
        }

        //Register an in-progress load unless there is an existing load
        //operation, or the resource is already loaded
        if (!alreadyLoading) {
          auto loadData = ge_new<ResourceLoadData>(outputResource.getWeak(), 0);
          m_inProgressResources[uuid] = loadData;
          loadData->resData = outputResource.getWeak();

          if (loadFlags.isSet(RLF::kKeepInternalRef)) {
            ++loadData->resData.numInternalRefs;
            outputResource.addInternalRef();
          }

          loadData->remainingDependencies = 1; //Self

          //Make resource listener trigger before exit if loading synchronously
          loadData->notifyImmediately = synchronous;

          //Register dependencies and count them so we know when the resource
          //is fully loaded
          if (loadFlags.isSet(RLF::kLoadDependencies) &&
              nullptr != savedResourceData) {
            for (auto& dependency : savedResourceData->getDependencies()) {
              if (dependency != uuid) {
                m_dependantLoads[dependency].push_back(loadData);
                ++loadData->remainingDependencies;
                dependenciesToLoad.push_back(dependency);
              }
            }
          }
        }

        //The resource is already being loaded, or is loaded, but we might
        //still need to load some dependencies
        else if (nullptr != savedResourceData) {
          const Vector<UUID>& dependencies = savedResourceData->getDependencies();
          if (!dependencies.empty()) {
            ResourceLoadData* loadData = nullptr;

            //If load not in progress, register the resource for load
            if (!loadInProgress) {
              loadData = ge_new<ResourceLoadData>(outputResource.getWeak(), 0);
              loadData->resData = outputResource.getWeak();
              loadData->remainingDependencies = 0;

              //Make resource listener trigger before exit if loading
              //synchronously
              loadData->notifyImmediately = synchronous;
            }
            else {
              loadData = m_inProgressResources[uuid];
            }

            //Find dependencies that aren't already queued for loading
            for (auto& dependency : dependencies) {
              if (dependency != uuid) {
                bool registerDependency = true;

                auto iterFindDep = m_dependantLoads.find(dependency);
                if (iterFindDep != m_dependantLoads.end()) {
                  Vector<ResourceLoadData*>& dependantData = iterFindDep->second;
                  auto iterFind3 = find_if(dependantData.begin(), dependantData.end(),
                    [&](ResourceLoadData* x) {
                      return x->resData.resource.getUUID() == outputResource.getUUID();
                    });

                  registerDependency = iterFind3 == dependantData.end();
                }

                if (registerDependency) {
                  m_dependantLoads[dependency].push_back(loadData);
                  ++loadData->remainingDependencies;
                  dependenciesToLoad.push_back(dependency);
                }
              }
            }

            if (!loadInProgress) {
              if (!dependenciesToLoad.empty()) {
                m_inProgressResources[uuid] = loadData;
              }
              else {
                ge_delete(loadData);
              }
            }
          }
        }

        initiateLoad = !alreadyLoading && !filePath.isEmpty();

        if (nullptr != savedResourceData) {
          synchronous = synchronous & savedResourceData->allowAsyncLoading();
        }
      }
    }

    //Previously being loaded as async but now we want it synced, so we wait
    if (loadInProgress && synchronous) {
      outputResource.blockUntilLoaded();
    }

    //Something went wrong, clean up and exit
    if (loadFailed) {
      //Clean up in-progress state
      loadComplete(outputResource);
      return outputResource;
    }

    //Actually start the file read operation if not already loaded or in
    //progress
    if (initiateLoad) {
      //Synchronous or the resource doesn't support async, read the file
      //immediately
      if (synchronous) {
        loadCallback(filePath, outputResource, loadFlags.isSet(RLF::kKeepSourceData));
      }
      else {//Asynchronous, read the file on a worker thread
        String fileName = filePath.getFilename();
        String taskName = "Resource load: " + fileName;

        bool keepSourceData = loadFlags.isSet(RLF::kKeepSourceData);
        SPtr<Task> task = Task::create(taskName,
                                       bind(&Resources::loadCallback,
                                            this,
                                            filePath,
                                            outputResource,
                                            keepSourceData));
        TaskScheduler::instance().addTask(task);
      }
    }
    else {
      if (!loadInProgress) {
        //Already loaded, decrement dependency count
        loadComplete(outputResource);
      }
    }

    //Load dependencies
    SIZE_T numDependencies = dependenciesToLoad.size();
    if (numDependencies > 0) {
      ResourceLoadFlags depLoadFlags = RLF::kLoadDependencies;
      if (loadFlags.isSet(RLF::kKeepSourceData)) {
        depLoadFlags |= RLF::kKeepSourceData;
      }

      Vector<HResource> dependencies(numDependencies);

      //Keep dependencies alive until the parent is done loading
      {
        //Note the resource is still guaranteed to be in the in-progress map
        //because it can't be removed until its dependency count is reduced to
        //zero.
        Lock inProgressLock(m_inProgressResourcesMutex);
        m_inProgressResources[uuid]->dependencies = dependencies;
      }

      for (SIZE_T i = 0; i < numDependencies; ++i) {
        dependencies[i] = loadFromUUID(dependenciesToLoad[i], !synchronous, depLoadFlags);
      }
    }

    return outputResource;
  }

  SPtr<Resource>
  Resources::loadFromDiskAndDeserialize(const Path& filePath, bool loadWithSaveData) {
    Lock fileLock = FileScheduler::getLock(filePath);

    SPtr<DataStream> stream = FileSystem::openFile(filePath, true);
    if (nullptr == stream) {
      return nullptr;
    }

    if (stream->size() > NumLimit::MAX_UINT32) {
      GE_EXCEPT(InternalErrorException,
                "File size is larger that uint32 can hold. "
                "Ask a programmer to use a bigger data type.");
    }

    CoreSerializationContext serzContext;
    serzContext.flags = loadWithSaveData ? SERIALIZATION_FLAGS::kKeepResourceSourceData : 0;

    //Read meta-data
    SPtr<SavedResourceData> metaData;
    {
      if (!stream->isEOF()) {
        uint32 objectSize = 0;
        stream->read(&objectSize, sizeof(objectSize));

        BinarySerializer bs;
        metaData = static_pointer_cast<SavedResourceData>(bs.decode(stream,
                                                                    objectSize,
                                                                    &serzContext));
      }
    }

    //Read resource data
    SPtr<IReflectable> loadedData;
    {
      if (metaData && !stream->isEOF()) {
        uint32 objectSize = 0;
        stream->read(&objectSize, sizeof(objectSize));

        if (metaData->getCompressionMethod() != 0) {
          stream = Compression::decompress(stream);
        }

        BinarySerializer bs;
        loadedData = static_pointer_cast<SavedResourceData>(bs.decode(stream,
                                                                      objectSize,
                                                                      &serzContext));
      }
    }

    if (nullptr == loadedData) {
      LOGERR("Unable to load resource at path \"" + filePath.toString() + "\"");
    }
    else {
      if (!loadedData->isDerivedFrom(Resource::getRTTIStatic())) {
        GE_EXCEPT(InternalErrorException,
                  "Loaded class doesn't derive from Resource.");
      }
    }

    SPtr<Resource> resource = static_pointer_cast<Resource>(loadedData);
    return resource;
  }

  void
  Resources::release(ResourceHandleBase& resource) {
    const UUID& uuid = resource.getUUID();
    {
      bool loadInProgress = false;
      {
        Lock inProgressLock(m_inProgressResourcesMutex);
        auto iterFind2 = m_inProgressResources.find(uuid);
        if (iterFind2 != m_inProgressResources.end()) {
          loadInProgress = true;
        }
      }

      //Technically we should be able to just cancel a load in progress instead
      //of blocking until it finishes. However that would mean the last
      //reference could get lost on whatever thread did the loading, which
      //isn't something that's supported. If this ends up being a problem
      //either make handle counting atomic or add a separate queue for objects
      //destroyed from the load threads.
      if (loadInProgress) {
        resource.blockUntilLoaded();
      }

      bool lostLastRef = false;
      {
        Lock loadedLock(m_loadedResourceMutex);
        auto iterFind = m_loadedResources.find(uuid);
        if (iterFind != m_loadedResources.end()) {
          LoadedResourceData& resData = iterFind->second;

          GE_ASSERT(resData.numInternalRefs > 0);
          --resData.numInternalRefs;
          resource.removeInternalRef();

          uint32 refCount = resource.getHandleData()->m_refCount.load(memory_order_relaxed);
          lostLastRef = refCount == 0;
        }
      }

      if (lostLastRef) {
        destroy(resource);
      }
    }
  }

  void
  Resources::unloadAllUnused() {
    Vector<HResource> resourcesToUnload;
    {
      Lock lock(m_loadedResourceMutex);
      for (auto& loadedResource : m_loadedResources) {
        const LoadedResourceData& resData = loadedResource.second;

        uint32 refCount = resData.resource.m_data->m_refCount.load(memory_order_relaxed);
        GE_ASSERT(refCount > 0); //No references but kept in m_loadedResources list?

        if (refCount == resData.numInternalRefs) {//Only internal references exist, free it
          resourcesToUnload.push_back(resData.resource.lock());
        }
      }
    }

    //NOTE: When unloading multiple resources it's possible that unloading one
    //will also unload another resource in "resourcesToUnload". This is fine
    //because "unload" deals with invalid handles gracefully.
    for (auto & iter : resourcesToUnload) {
      release(iter);
    }
  }

  void
  Resources::unloadAll() {
    //Unload and invalidate all resources
    UnorderedMap<UUID, LoadedResourceData> loadedResourcesCopy;
    {
      Lock lock(m_loadedResourceMutex);
      loadedResourcesCopy = m_loadedResources;
    }

    for (auto& loadedResourcePair : loadedResourcesCopy) {
      destroy(loadedResourcePair.second.resource);
    }
  }

  void
  Resources::destroy(ResourceHandleBase& resource) {
    if (nullptr == resource.m_data) {
      return;
    }

    RecursiveLock lock(m_destroyMutex);

    const UUID& uuid = resource.getUUID();
    if (!resource.isLoaded(false)) {
      bool loadInProgress = false;
      {
        Lock lockPRM(m_inProgressResourcesMutex);
        auto iterFind2 = m_inProgressResources.find(uuid);
        if (iterFind2 != m_inProgressResources.end()) {
          loadInProgress = true;
        }
      }

      if (loadInProgress) {// If it's still loading wait until that finishes
        resource.blockUntilLoaded();
      }
      else {
        return; //Already unloaded
      }
    }

    //Notify external systems before we actually destroy it
    onResourceDestroyed(uuid);
    resource.m_data->m_ptr->destroy();

    {
      Lock lockRM(m_loadedResourceMutex);
      auto iterFind = m_loadedResources.find(uuid);
      if (iterFind != m_loadedResources.end()) {
        LoadedResourceData& resData = iterFind->second;
        while (resData.numInternalRefs > 0) {
          --resData.numInternalRefs;
          resData.resource.removeInternalRef();
        }

        m_loadedResources.erase(iterFind);
      }
      else {
        //This should never happen but in case it does fail silently in Release
        GE_ASSERT(false);
      }
    }

    resource.clearHandleData();
  }

  void
  Resources::save(const HResource& resource,
                  const Path& filePath,
                  bool overwrite,
                  bool compress) {
    if (nullptr == resource) {
      return;
    }

    if (!resource.isLoaded(false))
    {
      bool loadInProgress = false;
      {
        Lock lock(m_inProgressResourcesMutex);
        auto iterFind2 = m_inProgressResources.find(resource.getUUID());
        if (iterFind2 != m_inProgressResources.end()) {
          loadInProgress = true;
        }
      }

      if (loadInProgress) {//If it's still loading wait until that finishes
        resource.blockUntilLoaded();
      }
      else {
        return; //Nothing to save
      }
    }

    const bool fileExists = FileSystem::isFile(filePath);
    if (fileExists && !overwrite) {
      LOGERR("Another file exists at the specified location. Not saving.");
      return;
    }

    if (!resource->m_keepSourceData) {
      LOGWRN("Saving a resource that was created/loaded without "
             "ResourceLoadFlag::kKeepSourceData. Some data might not be "
             "available for saving. File path: " + filePath.toString());
    }

    {
      Lock lock(m_defaultManifestMutex);
      m_defaultResourceManifest->registerResource(resource.getUUID(), filePath);
    }

    _save(resource.getInternalPtr(), filePath, compress);
  }

  void
  Resources::save(const HResource& resource, bool compress) {
    if (nullptr == resource) {
      return;
    }

    Path path;
    if (getFilePathFromUUID(resource.getUUID(), path)) {
      save(resource, path, true, compress);
    }
  }

  void
  Resources::_save(const SPtr<Resource>& resource,
                   const Path& filePath,
                   bool compress) {
    if (!resource->m_keepSourceData) {
      LOGWRN("Saving a resource that was created/loaded without "
             "RESOURCE_LOAD_FLAG::kKeepSourceData. Some data might not be "
             "available for saving. File path: " + filePath.toString());
    }

    auto dependencyList = Utility::findResourceDependencies(*resource);
    Vector<UUID> dependencyUUIDs(dependencyList.size());

    for (SIZE_T i = 0; i < dependencyList.size(); ++i) {
      dependencyUUIDs[i] = dependencyList[i].resource.getUUID();
    }

    uint32 compressionMethod = (compress && resource->isCompressible()) ? 1 : 0;
    auto resourceData = ge_shared_ptr_new<SavedResourceData>(dependencyUUIDs,
                                                             resource->allowAsyncLoading(),
                                                             compressionMethod);

    Path parentDir = filePath.getDirectory();
    if (!FileSystem::exists(parentDir)) {
      FileSystem::createDir(parentDir);
    }

    Path savePath;
    const bool fileExists = FileSystem::isFile(filePath);
    if (fileExists) {
      //If a file exists, save to a temporary location, then copy over only
      //after a save was successful. This guards against data loss in case the
      //save process fails.

      savePath = FileSystem::getTempDirectoryPath();
      savePath.setFilename(UUIDGenerator::generateRandom().toString());

      uint32 safetyCounter = 0;
      while (FileSystem::exists(savePath)) {
        if (safetyCounter > 10) {
          LOGERR("Internal error. Unable to save resource due to not being "
                 "able to find a unique filename.");
          return;
        }

        savePath.setFilename(UUIDGenerator::generateRandom().toString());
        ++safetyCounter;
      }
    }
    else {
      savePath = filePath;
    }

    Lock fileLock = FileScheduler::getLock(filePath);

    ofstream stream;
    stream.open(savePath.toPlatformString().c_str(), ios::out | ios::binary);
    if (stream.fail()) {
      LOGWRN("Failed to save file: \"" + filePath.toString() +
             "\". Error: " + strerror(errno) + ".");
    }

    //Write meta-data
    {
      MemorySerializer ms;
      uint32 numBytes = 0;
      uint8* bytes = ms.encode(resourceData.get(), numBytes);

      stream.write(reinterpret_cast<char*>(&numBytes), sizeof(numBytes));
      stream.write(reinterpret_cast<char*>(bytes), numBytes);
      ge_free(bytes);
    }

    //Write object data
    {
      MemorySerializer ms;
      uint32 numBytes = 0;
      uint8* bytes = ms.encode(resource.get(), numBytes);

      auto objStream = ge_shared_ptr_new<MemoryDataStream>(bytes, numBytes);
      if (0 != compressionMethod) {
        auto srcStream = static_pointer_cast<DataStream>(objStream);
        objStream = Compression::compress(srcStream);
      }

      stream.write(reinterpret_cast<char*>(&numBytes), sizeof(numBytes));
      stream.write(reinterpret_cast<char*>(objStream->getPtr()), objStream->size());
    }

    stream.close();
    stream.clear();

    if (fileExists) {
      FileSystem::remove(filePath);
      FileSystem::move(savePath, filePath);
    }
  }

  void
  Resources::update(HResource& handle, const SPtr<Resource>& resource) {
    const UUID& uuid = handle.getUUID();
    handle.setHandleData(resource, uuid);

    {
      Lock lock(m_loadedResourceMutex);
      auto iterFind = m_loadedResources.find(uuid);
      if (iterFind == m_loadedResources.end()) {
        LoadedResourceData& resData = m_loadedResources[uuid];
        resData.resource = handle.getWeak();
      }
    }

    onResourceModified(handle);
    ResourceListenerManager::instance().notifyListeners(uuid);
  }

  Vector<UUID>
  Resources::getDependencies(const Path& filePath) {
    SPtr<SavedResourceData> savedResourceData;
    if (!filePath.isEmpty()) {
      FileDecoder fs(filePath);
      savedResourceData = static_pointer_cast<SavedResourceData>(fs.decode());
    }

    return savedResourceData->getDependencies();
  }

  void
  Resources::registerResourceManifest(const SPtr<ResourceManifest>& manifest) {
    auto findIter = find(m_resourceManifests.begin(), m_resourceManifests.end(), manifest);
    if (findIter == m_resourceManifests.end()) {
      m_resourceManifests.push_back(manifest);
    }
    else {
      *findIter = manifest;
    }
  }

  void
  Resources::unregisterResourceManifest(const SPtr<ResourceManifest>& manifest) {
    if (manifest->getName() == "Default") {
      return;
    }

    auto findIter = find(m_resourceManifests.begin(), m_resourceManifests.end(), manifest);
    if (findIter != m_resourceManifests.end()) {
      m_resourceManifests.erase(findIter);
    }
  }

  SPtr<ResourceManifest>
  Resources::getResourceManifest(const String& name) const {
    for (auto iter = m_resourceManifests.rbegin();
         iter != m_resourceManifests.rend();
         ++iter) {
      if (name == (*iter)->getName()) {
        return (*iter);
      }
    }

    return nullptr;
  }

  bool
  Resources::isLoaded(const UUID& uuid, bool checkInProgress) {
    if (checkInProgress) {
      Lock inProgressLock(m_inProgressResourcesMutex);
      auto iterFind2 = m_inProgressResources.find(uuid);
      if (iterFind2 != m_inProgressResources.end()) {
        return true;
      }

      {
        Lock loadedLock(m_loadedResourceMutex);
        auto iterFind = m_loadedResources.find(uuid);
        if (iterFind != m_loadedResources.end()) {
          return true;
        }
      }
    }

    return false;
  }

  HResource
  Resources::_createResourceHandle(const SPtr<Resource>& obj) {
    UUID uuid = UUIDGenerator::generateRandom();
    return _createResourceHandle(obj, uuid);
  }

  HResource
  Resources::_createResourceHandle(const SPtr<Resource>& obj, const UUID& uuid) {
    HResource newHandle(obj, uuid);
    {
      Lock lock(m_loadedResourceMutex);
      LoadedResourceData& resData = m_loadedResources[uuid];
      resData.resource = newHandle.getWeak();
      m_handles[uuid] = newHandle.getWeak();
    }

    return newHandle;
  }

  HResource
  Resources::_getResourceHandle(const UUID& uuid) {
    Lock lock(m_loadedResourceMutex);
    auto iterFind3 = m_handles.find(uuid);
    if (iterFind3 != m_handles.end()) {//Not loaded, but handle does exist
      return iterFind3->second.lock();
    }

    //Create new handle
    HResource handle(uuid);
    m_handles[uuid] = handle.getWeak();

    return handle;
  }

  bool
  Resources::getFilePathFromUUID(const UUID& uuid, Path& filePath) const {
    for (auto iter = m_resourceManifests.rbegin();
         iter != m_resourceManifests.rend();
         ++iter) {
      if ((*iter)->uuidToFilePath(uuid, filePath)) {
        return true;
      }
    }

    return false;
  }

  bool Resources::getUUIDFromFilePath(const Path& path, UUID& uuid) const
  {
    Path manifestPath = path;
    if (!manifestPath.isAbsolute()) {
      manifestPath.makeAbsolute(FileSystem::getWorkingDirectoryPath());
    }

    for (auto iter = m_resourceManifests.rbegin();
         iter != m_resourceManifests.rend();
         ++iter) {
      if ((*iter)->filePathToUUID(manifestPath, uuid)) {
        return true;
      }
    }

    return false;
  }

  void
  Resources::loadComplete(HResource& resource) {
    UUID uuid = resource.getUUID();

    ResourceLoadData* myLoadData = nullptr;
    bool finishLoad = true;
    Vector<ResourceLoadData*> dependantLoads;
    {
      Lock inProgresslock(m_inProgressResourcesMutex);

      auto iterFind = m_inProgressResources.find(uuid);
      if (iterFind != m_inProgressResources.end()) {
        myLoadData = iterFind->second;
        finishLoad = myLoadData->remainingDependencies == 0;

        if (finishLoad) {
          m_inProgressResources.erase(iterFind);
        }
      }

      auto iterFind2 = m_dependantLoads.find(uuid);

      if (iterFind2 != m_dependantLoads.end()) {
        dependantLoads = iterFind2->second;
      }

      if (finishLoad) {
        m_dependantLoads.erase(uuid);

        //If loadedData is null then we're probably completing load on an
        //already loaded resource, triggered by its dependencies.
        if (nullptr != myLoadData && nullptr != myLoadData->loadedData) {
          Lock loadedLock(m_loadedResourceMutex);

          m_loadedResources[uuid] = myLoadData->resData;
          resource.setHandleData(myLoadData->loadedData, uuid);
        }

        for (auto& dependantLoad : dependantLoads) {
          --dependantLoad->remainingDependencies;
        }
      }
    }

    for (auto& dependantLoad : dependantLoads) {
      HResource dependant = dependantLoad->resData.resource.lock();
      loadComplete(dependant);
    }

    if (finishLoad && nullptr != myLoadData ) {
      onResourceLoaded(resource);

      //This should only ever be true on the main thread
      if (myLoadData->notifyImmediately) {
        ResourceListenerManager::instance().notifyListeners(uuid);
      }

      ge_delete(myLoadData);
    }
  }

  void
  Resources::loadCallback(const Path& filePath, HResource& resource, bool loadWithSaveData) {
    SPtr<Resource> rawResource = loadFromDiskAndDeserialize(filePath, loadWithSaveData);
    {
      Lock lock(m_inProgressResourcesMutex);

      //Check if all my dependencies are loaded
      ResourceLoadData* myLoadData = m_inProgressResources[resource.getUUID()];
      myLoadData->loadedData = rawResource;
      --myLoadData->remainingDependencies;
    }

    loadComplete(resource);
  }

  GE_CORE_EXPORT Resources&
  g_resources() {
    return Resources::instance();
  }
}
