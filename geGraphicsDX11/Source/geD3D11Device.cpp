/*****************************************************************************/
/**
 * @file    geD3D11Device.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/11
 * @brief   	Wrapper around DirectX 11 device object.
 *
 * 	Wrapper around DirectX 11 device object.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11Device.h"

#include <geException.h>

namespace geEngineSDK {
  namespace geCoreThread {
    D3D11Device::D3D11Device(ID3D11Device* device)
      : m_d3d11Device(device) {
      GE_ASSERT(nullptr != device);

      HRESULT hr = E_FAIL;
      if (device) {
        device->GetImmediateContext(&m_immediateContext);
#if GE_DEBUG_MODE
        //This interface is not available unless we created the device with
        //debug layer
        hr = m_d3d11Device->QueryInterface(__uuidof(ID3D11InfoQueue),
                                                   reinterpret_cast<LPVOID*>(&m_infoQueue));

        if (FAILED(hr)) {
          GE_EXCEPT(RenderingAPIException, "Unable to query D3D11InfoQueue");
        }

        setExceptionsErrorLevel(GE_D3D11_ERROR_LEVEL::kERROR);
#endif
        //If feature level is 11, create class linkage
        SAFE_RELEASE(m_classLinkage);
        if (m_d3d11Device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0) {
          hr = m_d3d11Device->CreateClassLinkage(&m_classLinkage);
          if (FAILED(hr)) {
            GE_EXCEPT(RenderingAPIException,
                      "Unable to create class linkage.");
          }
        }

        //Get feature options
        m_d3d11Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS,
                                           &m_d3d11FeatureOptions,
                                           sizeof(m_d3d11FeatureOptions));
      }
    }

    D3D11Device::~D3D11Device() {
      shutdown();
    }

    void
    D3D11Device::shutdown() {
      if (m_immediateContext) {
        m_immediateContext->Flush();
        m_immediateContext->ClearState();
      }

      SAFE_RELEASE(m_infoQueue);
      SAFE_RELEASE(m_d3d11Device);
      SAFE_RELEASE(m_immediateContext);
      SAFE_RELEASE(m_classLinkage);
    }

    String
    D3D11Device::getErrorDescription(bool bClearErrors) {
      if (nullptr == m_d3d11Device) {
        return "Null device.";
      }

      String res;
      if (nullptr != m_infoQueue) {
        uint64 numStoredMessages =
          m_infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

        for (uint64 i = 0; i < numStoredMessages; ++i) {
          //Get the size of the message
          SIZE_T messageLength = 0;
          m_infoQueue->GetMessage(i, nullptr, &messageLength);

          //Allocate space and get the message
          auto* pMessage = reinterpret_cast<D3D11_MESSAGE*>(malloc(messageLength));
          m_infoQueue->GetMessage(i, pMessage, &messageLength);
          res = res + pMessage->pDescription + "\n";
          free(pMessage);
        }
      }

      if (bClearErrors) {
        clearErrors();
      }

      return res;
    }

    bool
    D3D11Device::hasError() const {
      if (nullptr != m_infoQueue) {
        const uint64 numStoredMessages =
          m_infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

        return numStoredMessages > 0;
      }

      return false;
    }

    void
    D3D11Device::clearErrors() {
      if (nullptr != m_d3d11Device && nullptr != m_infoQueue) {
        m_infoQueue->ClearStoredMessages();
      }
    }

    void
    D3D11Device::setExceptionsErrorLevel(const GE_D3D11_ERROR_LEVEL::E exceptionsErrorLevel) {
      if (nullptr == m_infoQueue) {
        return;
      }

      m_infoQueue->ClearRetrievalFilter();
      m_infoQueue->ClearStorageFilter();

      D3D11_INFO_QUEUE_FILTER filter;
      memset(&filter, 0, sizeof(filter));
      Vector<D3D11_MESSAGE_SEVERITY> severityList;

      switch (exceptionsErrorLevel)
      {
        case GE_D3D11_ERROR_LEVEL::kINFO:
          severityList.push_back(D3D11_MESSAGE_SEVERITY_INFO);
        case GE_D3D11_ERROR_LEVEL::kWARNING:
          severityList.push_back(D3D11_MESSAGE_SEVERITY_WARNING);
        case GE_D3D11_ERROR_LEVEL::kERROR:
          severityList.push_back(D3D11_MESSAGE_SEVERITY_ERROR);
        case GE_D3D11_ERROR_LEVEL::kCORRUPTION:
          severityList.push_back(D3D11_MESSAGE_SEVERITY_CORRUPTION);
        case GE_D3D11_ERROR_LEVEL::kNO_EXCEPTION:
        default:
          break;
      }

      if (!severityList.empty()) {
        filter.AllowList.NumSeverities = static_cast<UINT>(severityList.size());
        filter.AllowList.pSeverityList = &severityList[0];
      }

      m_infoQueue->AddStorageFilterEntries(&filter);
      m_infoQueue->AddRetrievalFilterEntries(&filter);
    }
  }
}
