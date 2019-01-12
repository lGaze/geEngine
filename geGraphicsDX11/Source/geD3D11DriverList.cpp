/*****************************************************************************/
/**
 * @file    geD3D11DriverList.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/11
 * @brief   Contains a list of all available drivers.
 *
 * Contains a list of all available drivers.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11DriverList.h"
#include "geD3D11Driver.h"

#include <geException.h>

namespace geEngineSDK {
  namespace geCoreThread {
    D3D11DriverList::D3D11DriverList(IDXGIFactory1* dxgiFactory) {
      enumerate(dxgiFactory);
    }

    D3D11DriverList::~D3D11DriverList() {
      for (auto driver : m_driverList) {
        ge_delete(driver);
      }
      m_driverList.clear();
    }

    void
    D3D11DriverList::enumerate(IDXGIFactory1* dxgiFactory) {
      uint32 adapterIdx = 0;
      IDXGIAdapter* dxgiAdapter = nullptr;
      HRESULT hr;

      while ((hr = dxgiFactory->EnumAdapters(adapterIdx, &dxgiAdapter))
             != DXGI_ERROR_NOT_FOUND) {
        if (FAILED(hr)) {
          SAFE_RELEASE(dxgiAdapter);
          GE_EXCEPT(InternalErrorException, "Enumerating adapters failed.");
        }

        m_driverList.push_back(ge_new<D3D11Driver>(adapterIdx, dxgiAdapter));

        SAFE_RELEASE(dxgiAdapter);
        ++adapterIdx;
      }
    }

    uint32
    D3D11DriverList::count() const {
      return static_cast<uint32>(m_driverList.size());
    }

    D3D11Driver*
    D3D11DriverList::item(uint32 idx) const {
      return m_driverList.at(idx);
    }

    D3D11Driver*
    D3D11DriverList::item(const String &name) const {
      for (auto driver : m_driverList) {
          if (driver->getDriverDescription() == name) {
            return driver;
          }
      }

      GE_EXCEPT(InvalidParametersException,
                "Cannot find video mode with the specified name.");

      //return nullptr; //Unreachable code
    }
  }
}
