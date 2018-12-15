/*****************************************************************************/
/**
 * @file    geCoreApplication.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geRenderWindow.h"

#include <geModule.h>
#include <geEvent.h>

namespace geEngineSDK {
  /**
   * @brief Structure containing parameters for starting the application.
   */
  struct START_UP_DESC
  {
    /**
     * Name of the render system plugin to use.
     */
    String renderAPI;

    /**
     * Name of the renderer plugin to use.
     */
    String renderer;

    /**
     * Name of physics plugin to use.
     */
    String physics;

    /**
     * Name of the audio plugin to use.
     */
    String audio;

    /**
     * Name of the input plugin to use.
     */
    String input;

    /**
     * True to load the scripting system.
     */
    bool scripting = false;

    /**
     * Describes the window to create during start-up.
     */
    RENDER_WINDOW_DESC primaryWindowDesc;

    /**
     * A list of importer plugins to load.
     */
    Vector<String> importers;
  };

  /**
   * @brief Represents the primary entry point for the core systems. Handles
   *        start-up, shutdown, primary loop and allows you to load and unload
   *        plugins.
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT CoreApplication : public Module<CoreApplication>
  {
   public:
    CoreApplication(START_UP_DESC desc);
    virtual ~CoreApplication();

    /**
     * @brief Executes the main loop. This will update your components and
     *        modules, queue objects for rendering and run the simulation.
     *        Usually called immediately after startUp().
     * @note  This will run infinitely until stopMainLoop is called
     *        (usually from another thread or internally).
     */
    void
    runMainLoop();

    /**
     * @brief Stops the (infinite) main loop from running. The loop will
     *        complete its current cycle before stopping.
     */
    void
    stopMainLoop();

    /**
     * @brief Changes the maximum FPS the application is allowed to run in.
     *        Zero means unlimited.
     */
    void
    setFPSLimit(uint32 limit);

    /**
     * @brief Issues a request for the application to close. Application may
     *        choose to ignore the request depending on the circumstances and
     *        the implementation.
     */
    virtual void
    quitRequested();

    /**
     * @brief Returns the main window that was created on application start-up.
     */
    SPtr<RenderWindow>
    getPrimaryWindow() const {
      return m_primaryWindow;
    }

    /**
     * @brief Returns the id of the simulation thread.
     * @note  Thread safe.
     */
    ThreadId
    getSimThreadId() {
      return m_simThreadId;
    }

    /**
     * @brief Returns true if the application is running in an editor,
     *        false if standalone.
     */
    virtual bool
    isEditor() const {
      return false;
    }

    /**
     * @brief Loads a plugin.
     * @param[in] pluginName  Name of the plugin to load, without extension.
     * @param[out] library    Specify as not null to receive a reference to the
     *                        loaded library.
     * @param[in] passThrough Optional parameter that will be passed to the
     *                        loadPlugin function.
     * @return  Value returned from the plugin start-up method.
     */
    void*
    loadPlugin(const String& pluginName,
               DynLib** library = nullptr,
               void* passThrough = nullptr);

    /**
     * @brief Unloads a previously loaded plugin.
     */
    void
    unloadPlugin(DynLib* library);

   protected:
    /**
     * @copydoc Module::onStartUp
     */
    void
    onStartUp() override;

    /**
     * @brief Called for each iteration of the main loop.
     *        Called before any game objects or plugins are updated.
     */
    virtual void
    preUpdate();

    /**
     * @brief Called for each iteration of the main loop.
     *        Called after all game objects and plugins are updated.
     */
    virtual void
    postUpdate();

    /**
     * @brief Called during the fixed update of the main loop.
     *        Called after preUpdate and before postUpdate.
     */
    virtual void
    fixedUpdate();

    /**
     * @brief Initializes the renderer specified during construction.
     *        Called during initialization.
     */
    virtual void
    startUpRenderer();

    /**
     * @brief Returns a handler that is used for resolving shader include file
     *        paths.
     */
    /*
    virtual SPtr<IShaderIncludeHandler>
    getShaderIncludeHandler() const;
    */

   private:
    /**
     * @brief Called when the frame finishes rendering.
     */
    void
    frameRenderingFinishedCallback();

    /**
     * @brief Called by the core thread to begin profiling.
     */
    void
    beginCoreProfiling();

    /**
     * @brief Called by the core thread to end profiling.
     */
    void
    endCoreProfiling();

   protected:
    using updatePluginFunc = void(*)();

    SPtr<RenderWindow> m_primaryWindow;
    START_UP_DESC m_startUpDesc;

    //Frame limiting
    uint64 m_frameStep = 16666;  //60 Times a second in microseconds
    uint64 m_lastFrameTime = 0;  //Microseconds

    DynLib* m_rendererPlugin;

    Map<DynLib*, updatePluginFunc> m_pluginUpdateFunctions;

    bool m_isFrameRenderingFinished;
    Mutex m_frameRenderingFinishedMutex;
    Signal m_frameRenderingFinishedCondition;
    ThreadId m_simThreadId;

    volatile bool m_runMainLoop;
  };

  /**
   * @brief Provides easy access to CoreApplication.
   */
  GE_CORE_EXPORT CoreApplication&
  g_coreApplication();
}
