/*****************************************************************************/
/**
 * @file    geCoreApplication.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/27
 * @brief   Represents the primary entry point for the core systems.
 *
 * Represents the primary entry point for the core systems. Handles start-up,
 * shutdown, primary loop and allows you to load and unload plugins.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geCoreApplication.h"

#include "geRenderAPI.h"
#include "geRenderAPIManager.h"
#include "geRenderWindow.h"
#include "geViewport.h"
#include "geGPUProgram.h"

#include "gePlatform.h"
#include "geResources.h"

#include "geRenderWindowManager.h"
#include "geResourceListenerManager.h"
#include "geRenderStateManager.h"
#include "geHardwareBufferManager.h"
#include "geGPUProgramManager.h"
//#include "geMeshManager.h"
#include "geQueryManager.h"

#include "geCoreObjectManager.h"
#include "geCoreThread.h"

#include "geGameObjectManager.h"
#include "geSceneObject.h"
#include "geSceneManager.h"

#include "geDeferredCallManager.h"
#include "geStringTableManager.h"

//#include "Renderer/geRendererManager.h"
//#include "Renderer/geRenderer.h"
//#include "Renderer/geParamBlocks.h"

//#include "Importer/geImporter.h"
//#include "Mesh/geMesh.h"

//#include "Input/geInput.h"

#include "geProfilerCPU.h"
#include "geProfilingManager.h"
#include "geProfilerGPU.h"
#include "geRenderStats.h"

//#include "Material/geMaterialManager.h"
//#include "Material/geShaderManager.h"
//#include "Physics/gePhysicsManager.h"
//#include "Physics/gePhysics.h"
//#include "Audio/geAudioManager.h"
//#include "Audio/geAudio.h"
//#include "Animation/geAnimationManager.h"

#include <geVector2.h>
#include <geDynLib.h>
#include <geDynLibManager.h>
#include <geTime.h>
#include <geMessageHandler.h>
#include <geThreadPool.h>
#include <geTaskScheduler.h>

namespace geEngineSDK {
  using std::bind;
  using std::move;

  CoreApplication::CoreApplication(START_UP_DESC desc)
    : m_primaryWindow(nullptr),
      m_startUpDesc(std::move(desc)),
      m_rendererPlugin(nullptr),
      m_isFrameRenderingFinished(true),
      m_simThreadId(GE_THREAD_CURRENT_ID),
      m_runMainLoop(false) {
    //Ensure all errors are reported properly
    CrashHandler::startUp();
  }

  CoreApplication::~CoreApplication() {
    m_primaryWindow->destroy();
    m_primaryWindow = nullptr;

    ProfilerGPU::shutDown();

    SceneManager::shutDown();

    StringTableManager::shutDown();
    Resources::shutDown();
    GameObjectManager::shutDown();

    ResourceListenerManager::shutDown();

    //All CoreObject related modules should be shut down now. They have likely
    //queued CoreObjects for destruction, so we need to wait for those objects
    //to get destroyed before continuing.
    CoreObjectManager::instance().syncToCore();
    g_coreThread().update();
    g_coreThread().submitAll(true);

    //unloadPlugin(m_rendererPlugin);

    RenderAPIManager::shutDown();

    //Must shut down before DynLibManager to ensure all objects are destroyed
    //before unloading their libraries
    CoreObjectManager::shutDown();

    DynLibManager::shutDown();
    Time::shutDown();
    DeferredCallManager::shutDown();

    CoreThread::shutDown();
    TaskScheduler::shutDown();
    ThreadPool::shutDown();
    MessageHandler::shutDown();

    ProfilerCPU::shutDown();
    ProfilingManager::shutDown();

    MemStack::endThread();
    Platform::_shutDown();

    CrashHandler::shutDown();
  }

  void
  CoreApplication::onStartUp() {
    // Number of cores while excluding current thread.
    uint32 numWorkerThreads = GE_THREAD_HARDWARE_CONCURRENCY - 1;

    Platform::_startUp();
    MemStack::beginThread();

    MessageHandler::startUp();
    ProfilerCPU::startUp();
    ProfilingManager::startUp();
    ThreadPool::startUp<TThreadPool<geEngineThreadPolicy>>((numWorkerThreads));
    TaskScheduler::startUp();
    TaskScheduler::instance().removeWorker();
    CoreThread::startUp();
    StringTableManager::startUp();
    DeferredCallManager::startUp();
    Time::startUp();
    DynLibManager::startUp();
    CoreObjectManager::startUp();
    GameObjectManager::startUp();
    Resources::startUp();
    ResourceListenerManager::startUp();
    RenderAPIManager::startUp();

    m_primaryWindow = RenderAPIManager::instance().
                        initialize(m_startUpDesc.renderAPI, m_startUpDesc.primaryWindowDesc);

    loadPlugin(m_startUpDesc.renderer, &m_rendererPlugin);

    SceneManager::startUp();
    startUpRenderer();

    ProfilerGPU::startUp();

    for (auto& importerName : m_startUpDesc.importers) {
      loadPlugin(importerName);
    }
  }

  void CoreApplication::runMainLoop() {
    m_runMainLoop = true;

    while (m_runMainLoop) {
      //Limit FPS if needed
      if (m_frameStep > 0) {
        uint64 currentTime = g_time().getTimePrecise();
        uint64 nextFrameTime = m_lastFrameTime + m_frameStep;
        while (nextFrameTime > currentTime) {
          auto waitTime = static_cast<uint32>(nextFrameTime - currentTime);

          //If waiting for longer, sleep
          if (waitTime >= 2000) {
            Platform::sleep(waitTime / 1000);
            currentTime = g_time().getTimePrecise();
          }
          else {
            //Otherwise we just spin, sleep timer granularity is too low and we
            //might end up wasting a millisecond otherwise.
            //NOTE: For mobiles where power might be more important than input
            //latency, consider using sleep (but seriously... mobiles?).
            while (nextFrameTime > currentTime) {
              currentTime = g_time().getTimePrecise();
            }
          }
        }

        m_lastFrameTime = currentTime;
      }

      g_profilerCPU().beginThread("Sim");

      Platform::_update();
      DeferredCallManager::instance()._update();
      g_time()._update();

      //RenderWindowManager::update needs to happen after Input::update and
      //before Input::_triggerCallbacks, so that all input is properly captured
      //in case there is a focus change, and so that focus change is registered
      //before input events are sent out (mouse press can result in code
      //checking if a window is in focus, so it has to be up to date)
      RenderWindowManager::instance()._update();
      g_Debug()._triggerCallbacks();

      //Send the preUpdate
      preUpdate();

      //Trigger fixed updates if required
      {
        uint64 step;
        const uint32 numIterations = g_time()._getFixedUpdateStep(step);

        const float stepSeconds = step / 1000000.0f;

        for (uint32 i = 0; i < numIterations; ++i) {
          fixedUpdate();
          PROFILE_CALL(g_sceneManager()._fixedUpdate(), "Scene fixed update");
          //PROFILE_CALL(g_physics().fixedUpdate(stepSeconds), "Physics simulation");

          g_time()._advanceFixedUpdate(step);
        }
      }

      PROFILE_CALL(g_sceneManager()._update(), "Scene update");

      //Update plug ins
      for (auto& pluginUpdateFunc : m_pluginUpdateFunctions) {
        pluginUpdateFunc.second();
      }

      postUpdate();

      //Send out resource events in case any were loaded/destroyed/modified
      ResourceListenerManager::instance().update();

      g_sceneManager()._updateCoreObjectTransforms();

      //Core and sim thread run in lockstep. This will result in a larger input
      //latency than if I was  running just a single thread. Latency becomes
      //worse if the core thread takes longer than sim thread, in which case
      //sim thread needs to wait. Optimal solution would be to get an average
      //difference between sim/core thread and start the sim thread a bit later
      //so they finish at nearly the same time.
      {
        Lock lock(m_frameRenderingFinishedMutex);
        while (!m_isFrameRenderingFinished) {
          TaskScheduler::instance().addWorker();
          m_frameRenderingFinishedCondition.wait(lock);
          TaskScheduler::instance().removeWorker();
        }
        m_isFrameRenderingFinished = false;
      }

      g_coreThread().queueCommand(bind(&CoreApplication::beginCoreProfiling, this),
                                  CTQF::kInternalQueue);
      g_coreThread().queueCommand(&Platform::_coreUpdate, CTQF::kInternalQueue);
      g_coreThread().queueCommand(bind(&geCoreThread::RenderWindowManager::_update,
                                       geCoreThread::RenderWindowManager::instancePtr()),
                                  CTQF::kInternalQueue);

      g_coreThread().update();
      g_coreThread().submitAll();

      g_coreThread().queueCommand(bind(&CoreApplication::frameRenderingFinishedCallback,
                                       this),
                                  CTQF::kInternalQueue);

      g_coreThread().queueCommand(bind(&geCoreThread::QueryManager::_update,
                                       geCoreThread::QueryManager::instancePtr()),
                                  CTQF::kInternalQueue);

      g_coreThread().queueCommand(bind(&CoreApplication::endCoreProfiling, this),
                                  CTQF::kInternalQueue);

      g_profilerCPU().endThread();
      g_profiler()._update();
    }

    // Wait until last core frame is finished before exiting
    {
      Lock lock(m_frameRenderingFinishedMutex);
      while (!m_isFrameRenderingFinished) {
        TaskScheduler::instance().addWorker();
        m_frameRenderingFinishedCondition.wait(lock);
        TaskScheduler::instance().removeWorker();
      }
    }
  }

  void
  CoreApplication::preUpdate() {}

  void
  CoreApplication::postUpdate() {}

  void
  CoreApplication::fixedUpdate() {}

  void
  CoreApplication::stopMainLoop() {
    //No sync primitives needed, in that rare case of a race condition we might
    //run the loop one extra iteration which is acceptable
    m_runMainLoop = false;
  }

  void
  CoreApplication::quitRequested() {
    stopMainLoop();
  }

  void
  CoreApplication::setFPSLimit(uint32 limit) {
    if (limit > 0) {
      m_frameStep = (uint64)1000000 / limit;
    }
    else {
      m_frameStep = 0;
    }
  }

  void
  CoreApplication::frameRenderingFinishedCallback() {
    Lock lock(m_frameRenderingFinishedMutex);
    m_isFrameRenderingFinished = true;
    m_frameRenderingFinishedCondition.notify_one();
  }

  void
  CoreApplication::startUpRenderer() {
    //RendererManager::instance().initialize();
  }

  void
  CoreApplication::beginCoreProfiling() {
    g_profilerCPU().beginThread("Core");
  }

  void
  CoreApplication::endCoreProfiling() {
    ProfilerGPU::instance()._update();
    g_profilerCPU().endThread();
    g_profiler()._updateCore();
  }

  void*
  CoreApplication::loadPlugin(const String& pluginName,
                              DynLib** library,
                              void* passThrough) {
    DynLib* loadedLibrary = g_dynLibManager().load(pluginName);
    if (nullptr != library) {
      *library = loadedLibrary;
    }

    void* retVal = nullptr;
    if (nullptr != loadedLibrary) {
      if (nullptr == passThrough) {
        using loadPlgnFnc = void* (*)();

        loadPlgnFnc lpf = reinterpret_cast<loadPlgnFnc>
                            (loadedLibrary->getSymbol("loadPlugin"));

        if (nullptr != lpf) {
          retVal = lpf();
        }
      }
      else {
        using loadPluginFunc = void* (*)(void*);

        loadPluginFunc lpf = reinterpret_cast<loadPluginFunc>
                               (loadedLibrary->getSymbol("loadPlugin"));

        if (nullptr != lpf) {
          retVal = lpf(passThrough);
        }
      }

      updatePluginFunc lpf = reinterpret_cast<updatePluginFunc>
                               (loadedLibrary->getSymbol("updatePlugin"));

      if (nullptr != lpf) {
        m_pluginUpdateFunctions[loadedLibrary] = lpf;
      }
    }

    return retVal;
  }

  void
  CoreApplication::unloadPlugin(DynLib* library) {
    using unloadPluginFunc = void(*)();

    unloadPluginFunc upf = reinterpret_cast<unloadPluginFunc>
                             (library->getSymbol("unloadPlugin"));

    if (nullptr != upf) {
      upf();
    }

    m_pluginUpdateFunctions.erase(library);
    g_dynLibManager().unload(library);
  }

  /*
  SPtr<IShaderIncludeHandler>
  CoreApplication::getShaderIncludeHandler() const {
    return ge_shared_ptr_new<DefaultShaderIncludeHandler>();
  }
  */

  CoreApplication&
  g_coreApplication() {
    return CoreApplication::instance();
  }
}
