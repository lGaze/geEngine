/*****************************************************************************/
/**
 * @file    geRenderWindow.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/25
 * @brief   Operating system window with a specific position, size and style.
 *
 * Operating system window with a specific position, size and style.
 * Each window serves as a surface that can be rendered into by RenderAPI
 * operations.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geRenderWindow.h"
#include "geCoreThread.h"
#include "geRenderWindowManager.h"
#include "geViewport.h"
#include "gePlatform.h"

namespace geEngineSDK {
  using std::function;
  using std::bind;
  using std::cref;
  using std::static_pointer_cast;

  RenderWindowProperties::RenderWindowProperties(const RENDER_WINDOW_DESC& desc) {
    m_width = desc.videoMode.getWidth();
    m_height = desc.videoMode.getHeight();
    m_hwGamma = desc.gamma;
    m_vsync = desc.vsync;
    m_vsyncInterval = desc.vsyncInterval;
    m_multisampleCount = desc.multisampleCount;
    m_left = desc.left;
    m_top = desc.top;
    m_isFullScreen = desc.fullscreen;
    m_isHidden = desc.hidden;
    m_isModal = desc.modal;
    m_isWindow = true;
    m_requiresTextureFlipping = false;
  }

  void
  RenderWindow::destroy() {
    RenderWindowManager::instance().notifyWindowDestroyed(this);
    RenderTarget::destroy();
  }

  RenderWindow::RenderWindow(const RENDER_WINDOW_DESC& desc, uint32 windowId)
    : m_desc(desc),
      m_windowId(windowId)
  {}

  void
  RenderWindow::resize(uint32 width, uint32 height) {
    function<void(SPtr<geCoreThread::RenderWindow>, uint32, uint32)>
    resizeFunc = [](SPtr<geCoreThread::RenderWindow> renderWindow,
                    uint32 width,
                    uint32 height)
    {
      renderWindow->resize(width, height);
    };

    getMutableProperties().m_width = width;
    getMutableProperties().m_height = height;

    g_coreThread().queueCommand(bind(resizeFunc, getCore(), width, height));
  }

  void
  RenderWindow::move(int32 left, int32 top) {
    function<void(SPtr<geCoreThread::RenderWindow>, int32, int32)> moveFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow, int32 left, int32 top)
    {
      renderWindow->move(left, top);
    };

    getMutableProperties().m_left = left;
    getMutableProperties().m_top = top;

    g_coreThread().queueCommand(bind(moveFunc, getCore(), left, top));
  }

  void
  RenderWindow::hide() {
    function<void(SPtr<geCoreThread::RenderWindow>)> hideFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow)
    {
      renderWindow->setHidden(true);
    };

    getMutableProperties().m_isHidden = true;

    g_coreThread().queueCommand(bind(hideFunc, getCore()));
  }

  void
  RenderWindow::show() {
    function<void(SPtr<geCoreThread::RenderWindow>)> showFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow)
    {
      renderWindow->setHidden(false);
    };

    getMutableProperties().m_isHidden = false;

    g_coreThread().queueCommand(bind(showFunc, getCore()));
  }

  void
  RenderWindow::minimize() {
    function<void(SPtr<geCoreThread::RenderWindow>)> minimizeFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow)
    {
      renderWindow->minimize();
    };

    getMutableProperties().m_isMaximized = false;

    g_coreThread().queueCommand(bind(minimizeFunc, getCore()));
  }

  void
  RenderWindow::maximize() {
    function<void(SPtr<geCoreThread::RenderWindow>)> maximizeFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow)
    {
      renderWindow->maximize();
    };

    getMutableProperties().m_isMaximized = true;

    g_coreThread().queueCommand(bind(maximizeFunc, getCore()));
  }

  void
  RenderWindow::restore() {
    function<void(SPtr<geCoreThread::RenderWindow>)> restoreFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow)
    {
      renderWindow->restore();
    };

    getMutableProperties().m_isMaximized = false;

    g_coreThread().queueCommand(bind(restoreFunc, getCore()));
  }

  void
  RenderWindow::setFullscreen(uint32 width,
                              uint32 height,
                              float refreshRate,
                              uint32 monitorIdx) {
    function<void(SPtr<geCoreThread::RenderWindow>, uint32,
                  uint32,
                  float,
                  uint32)> fullscreenFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow,
       uint32 width,
       uint32 height,
       float refreshRate,
       uint32 monitorIdx)
    {
      renderWindow->setFullscreen(width, height, refreshRate, monitorIdx);
    };

    g_coreThread().queueCommand(bind(fullscreenFunc,
                                     getCore(),
                                     width,
                                     height,
                                     refreshRate,
                                     monitorIdx));
  }

  void
  RenderWindow::setFullscreen(const VideoMode& mode) {
    function<void(SPtr<geCoreThread::RenderWindow>, const VideoMode&)> fullscreenFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow, const VideoMode& mode)
    {
      renderWindow->setFullscreen(mode);
    };

    g_coreThread().queueCommand(bind(fullscreenFunc, getCore(), cref(mode)));
  }

  void
  RenderWindow::setWindowed(uint32 width, uint32 height) {
    function<void(SPtr<geCoreThread::RenderWindow>, uint32, uint32)> windowedFunc =
    [](SPtr<geCoreThread::RenderWindow> renderWindow, uint32 width, uint32 height)
    {
      renderWindow->setWindowed(width, height);
    };

    g_coreThread().queueCommand(bind(windowedFunc, getCore(), width, height));
  }

  SPtr<geCoreThread::RenderWindow>
  RenderWindow::getCore() const {
    return static_pointer_cast<geCoreThread::RenderWindow>(m_coreSpecific);
  }

  SPtr<RenderWindow>
  RenderWindow::create(RENDER_WINDOW_DESC& desc,
                       SPtr<RenderWindow> parentWindow) {
    return RenderWindowManager::instance().create(desc, parentWindow);
  }

  RenderWindowProperties&
  RenderWindow::getMutableProperties() {
    return const_cast<RenderWindowProperties&>(getProperties());
  }

  const RenderWindowProperties&
  RenderWindow::getProperties() const {
    return static_cast<const RenderWindowProperties&>(getPropertiesInternal());
  }

  void
  RenderWindow::_notifyWindowEvent(WINDOW_EVENT_TYPE::E type) {
    THROW_IF_CORE_THREAD;

    geCoreThread::RenderWindow* coreWindow = getCore().get();
    RenderWindowProperties& syncProps = coreWindow->getSyncedProperties();
    auto& props = const_cast<RenderWindowProperties&>(getProperties());

    switch (type)
    {
      case WINDOW_EVENT_TYPE::kResized:
      {
        _windowMovedOrResized();

        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_width = props.m_width;
          syncProps.m_height = props.m_height;
        }

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        RenderWindowManager::instance().notifyMovedOrResized(coreWindow);

        break;
      }
      case WINDOW_EVENT_TYPE::kMoved:
      {
        _windowMovedOrResized();

        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_top = props.m_top;
          syncProps.m_left = props.m_left;
        }

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        RenderWindowManager::instance().notifyMovedOrResized(coreWindow);

        break;
      }
      case WINDOW_EVENT_TYPE::kFocusReceived:
      {
        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_hasFocus = true;
        }

        props.m_hasFocus = true;

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        RenderWindowManager::instance().notifyFocusReceived(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kFocusLost:
      {
        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_hasFocus = false;
        }

        props.m_hasFocus = false;

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        RenderWindowManager::instance().notifyFocusLost(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kMinimized:
      {
        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_isMaximized = false;
        }

        props.m_isMaximized = false;

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kMaximized:
      {
        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_isMaximized = true;
        }

        props.m_isMaximized = true;

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kRestored:
      {
        {
          ScopedSpinLock lock(coreWindow->m_lock);
          syncProps.m_isMaximized = false;
        }

        props.m_isMaximized = false;

        geCoreThread::RenderWindowManager::instance().notifySyncDataDirty(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kMouseLeft:
      {
        RenderWindowManager::instance().notifyMouseLeft(coreWindow);
        break;
      }
      case WINDOW_EVENT_TYPE::kCloseRequested:
      {
        RenderWindowManager::instance().notifyCloseRequested(coreWindow);
        break;
      }
    }
  }

  namespace geCoreThread {
    RenderWindow::RenderWindow(const RENDER_WINDOW_DESC& desc, uint32 windowId)
      : m_desc(desc),
        m_windowId(windowId) {
      RenderWindowManager::instance().windowCreated(this);
    }

    RenderWindow::~RenderWindow() {
      RenderWindowManager::instance().windowDestroyed(this);
    }

    void
    RenderWindow::setHidden(bool hidden) {
      THROW_IF_NOT_CORE_THREAD;

      auto& props = const_cast<RenderWindowProperties&>(getProperties());

      props.m_isHidden = hidden;
      {
        ScopedSpinLock lock(m_lock);
        getSyncedProperties().m_isHidden = hidden;
      }

      geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
    }

    void
    RenderWindow::setActive(bool /*state*/) {
      THROW_IF_NOT_CORE_THREAD;
    }

    void
    RenderWindow::_notifyWindowEvent(WINDOW_EVENT_TYPE::E type) {
      THROW_IF_NOT_CORE_THREAD;

      RenderWindowProperties& syncProps = getSyncedProperties();
      auto& props = const_cast<RenderWindowProperties&>(getProperties());

      switch (type)
      {
        case WINDOW_EVENT_TYPE::kResized:
        {
          _windowMovedOrResized();

          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_width = props.m_width;
            syncProps.m_height = props.m_height;
          }

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          geEngineSDK::RenderWindowManager::instance().notifyMovedOrResized(this);

          break;
        }
        case WINDOW_EVENT_TYPE::kMoved:
        {
          _windowMovedOrResized();

          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_top = props.m_top;
            syncProps.m_left = props.m_left;
          }

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          geEngineSDK::RenderWindowManager::instance().notifyMovedOrResized(this);

          break;
        }
        case WINDOW_EVENT_TYPE::kFocusReceived:
        {
          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_hasFocus = true;
          }

          props.m_hasFocus = true;

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          geEngineSDK::RenderWindowManager::instance().notifyFocusReceived(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kFocusLost:
        {
          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_hasFocus = false;
          }

          props.m_hasFocus = false;

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          geEngineSDK::RenderWindowManager::instance().notifyFocusLost(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kMinimized:
        {
          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_isMaximized = false;
          }

          props.m_isMaximized = false;

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kMaximized:
        {
          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_isMaximized = true;
          }

          props.m_isMaximized = true;

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kRestored:
        {
          {
            ScopedSpinLock lock(m_lock);
            syncProps.m_isMaximized = false;
          }

          props.m_isMaximized = false;

          geEngineSDK::RenderWindowManager::instance().notifySyncDataDirty(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kMouseLeft:
        {
          geEngineSDK::RenderWindowManager::instance().notifyMouseLeft(this);
          break;
        }
        case WINDOW_EVENT_TYPE::kCloseRequested:
        {
          geEngineSDK::RenderWindowManager::instance().notifyCloseRequested(this);
          break;
        }
      }
    }

    const RenderWindowProperties&
    RenderWindow::getProperties() const {
      return static_cast<const RenderWindowProperties&>(getPropertiesInternal());
    }
  }
}
