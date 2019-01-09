/*****************************************************************************/
/**
 * @file    geD3D11Device.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/11
 * @brief   	Wrapper around DirectX 11 device object.
 *
 * 	Wrapper around DirectX 11 device object.
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
    /**
     * @brief Available DX11 error levels.
     */
    namespace GE_D3D11_ERROR_LEVEL {
      enum E {
        kNO_EXCEPTION,
        kCORRUPTION,
        kERROR,
        kWARNING,
        kINFO
      };
    }

    class D3D11Device final
    {
     public:
      /**
       * @brief Constructs the object with a previously created DX11 device.
       */
      D3D11Device(ID3D11Device* device);
      ~D3D11Device();

      /**
       * @brief Shuts down the device any releases any internal resources.
       */
      void
      shutdown();

      /**
       * @brief Returns DX11 immediate context object.
       */
      ID3D11DeviceContext*
      getImmediateContext() const {
        return m_immediateContext;
      }

      /**
       * @brief Returns DX11 class linkage object.
       */
      ID3D11ClassLinkage*
      getClassLinkage() const {
        return m_classLinkage;
      }

      /**
       * @brief Returns internal DX11 device.
       */
      ID3D11Device*
      getD3D11Device() const {
        return m_d3d11Device;
      }

      /**
       * @brief Returns information about optional features supported by the
       *        device.
       */
      const D3D11_FEATURE_DATA_D3D11_OPTIONS&
      getFeatureOptions() const {
        return m_d3d11FeatureOptions;
      }

      /**
       * @brief Resets error state & error messages.
       */
      void
      clearErrors();

      /**
       * @brief Query if error occurred at any point since last clearErrors()
       *        call. Use getErrorDescription to get a string describing the
       *        error.
       */
      bool
      hasError() const;

      /**
       * @brief Returns a string describing an error if one occurred.
       */
      String
      getErrorDescription(bool bClearErrors = true);

      /**
       * @brief Sets the level for which we want to receive errors for.
       *        Errors will be reported for the provided level and any higher
       *        priority level.
       */
      void
      setExceptionsErrorLevel(const GE_D3D11_ERROR_LEVEL::E exceptionsErrorLevel);

     private:
      D3D11Device() = default;

      ID3D11Device* m_d3d11Device = nullptr;
      ID3D11DeviceContext* m_immediateContext = nullptr;
      ID3D11InfoQueue* m_infoQueue = nullptr;
      ID3D11ClassLinkage* m_classLinkage = nullptr;
      D3D11_FEATURE_DATA_D3D11_OPTIONS m_d3d11FeatureOptions;
    };
  }
}
