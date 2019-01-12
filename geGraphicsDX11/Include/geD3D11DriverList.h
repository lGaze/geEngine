/*****************************************************************************/
/**
 * @file    geD3D11DriverList.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/11
 * @brief   Contains a list of all available drivers.
 *
 * Contains a list of all available drivers.
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
#include "gePrerequisitesD3D11.h"

namespace geEngineSDK {
  namespace geCoreThread {
    class D3D11DriverList final
    {
     public:
      /**
       * @brief Constructs a new driver list from an existing DXGI factory
       *        object.
       */
      D3D11DriverList(IDXGIFactory1* dxgiFactory);
      ~D3D11DriverList();

      /**
       * @brief Returns the number of available drivers.
       */
      uint32
      count() const;

      /**
       * @brief Returns a driver at the specified index.
       */
      D3D11Driver*
      item(uint32 idx) const;

      /**
       * @brief Returns a driver with the specified name,
                or null if it cannot be found.
       */
      D3D11Driver*
      item(const String &name) const;

     private:
      /**
       * @brief Enumerates the DXGI factory object and constructs a list of
       *        available drivers.
       */
      void
      enumerate(IDXGIFactory1* dxgiFactory);

      Vector<D3D11Driver*> m_driverList;
    };
  }
}
