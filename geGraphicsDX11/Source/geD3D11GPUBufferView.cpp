/*****************************************************************************/
/**
 * @file    geD3D11GPUBufferView.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/14
 * @brief   Represents a specific view of a GPUBuffer.
 *
 * Represents a specific view of a GPUBuffer. Different views all of the same
 * buffer be used in different situations (for example for reading from a
 * shader, or for a unordered read/write operation).
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11GPUBufferView.h"
#include "geD3D11GPUBuffer.h"
#include "geD3D11RenderAPI.h"
#include "geD3D11Device.h"
#include "geD3D11Mappings.h"

#include <geRenderStats.h>
#include <geException.h>

namespace geEngineSDK {
  namespace geCoreThread {
    size_t
    GPUBufferView::HashFunction::operator()(const GPU_BUFFER_VIEW_DESC& key) const {
      size_t seed = 0;
      hash_combine(seed, key.elementWidth);
      hash_combine(seed, key.firstElement);
      hash_combine(seed, key.numElements);
      hash_combine(seed, key.useCounter);
      hash_combine(seed, key.usage);
      hash_combine(seed, key.format);

      return seed;
    }

    bool
    GPUBufferView::EqualFunction::operator()(const GPU_BUFFER_VIEW_DESC& a,
                                             const GPU_BUFFER_VIEW_DESC& b) const {
      return a.elementWidth == b.elementWidth &&
             a.firstElement == b.firstElement &&
             a.numElements == b.numElements &&
             a.useCounter == b.useCounter &&
             a.usage == b.usage &&
             a.format == b.format;
    }

    GPUBufferView::GPUBufferView()
      : m_srv(nullptr),
        m_uav(nullptr)
    {}

    GPUBufferView::~GPUBufferView() {
      SAFE_RELEASE(m_srv);
      SAFE_RELEASE(m_uav);

      GE_INC_RENDER_STAT_CAT(ResDestroyed, D3D11_RENDER_STAT_RESOURCE_TYPE::kResourceView);
    }

    void
    GPUBufferView::initialize(D3D11GPUBuffer* buffer,
                              GPU_BUFFER_VIEW_DESC& desc) {
      m_buffer = buffer;
      m_desc = desc;

      if ((desc.usage & GPU_VIEW_USAGE::kDEFAULT) != 0) {
        m_srv = createSRV(buffer,
                          desc.firstElement,
                          desc.elementWidth,
                          desc.numElements);
      }

      if ((desc.usage & GPU_VIEW_USAGE::kRANDOMWRITE) != 0) {
        m_uav = createUAV(buffer,
                          desc.firstElement,
                          desc.numElements,
                          desc.useCounter);
      }

      if ((desc.usage & GPU_VIEW_USAGE::kRENDERTARGET) != 0 ||
          (desc.usage & GPU_VIEW_USAGE::kDEPTHSTENCIL) != 0) {
        GE_EXCEPT(NotImplementedException,
                  "Invalid usage flags for a GPU buffer view.");
      }

      GE_INC_RENDER_STAT_CAT(ResCreated, D3D11_RENDER_STAT_RESOURCE_TYPE::kResourceView);
    }

    ID3D11ShaderResourceView*
    GPUBufferView::createSRV(D3D11GPUBuffer* buffer,
                             uint32 firstElement,
                             uint32 elementWidth,
                             uint32 numElements) {
      const GPUBufferProperties& props = buffer->getProperties();

      D3D11_SHADER_RESOURCE_VIEW_DESC desc;
      memset(&desc, 0, sizeof(desc));

      if (props.getType() == GPU_BUFFER_TYPE::kSTANDARD) {
        desc.Format = D3D11Mappings::getBF(props.getFormat());
        desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = firstElement;
        desc.Buffer.NumElements = numElements;
      }
      else if (props.getType() == GPU_BUFFER_TYPE::kSTRUCTURED) {
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = firstElement;
        desc.Buffer.NumElements = numElements;
      }
      else if (props.getType() == GPU_BUFFER_TYPE::kINDIRECTARGUMENT)
      {
        desc.Format = DXGI_FORMAT_R32_UINT;
        desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        desc.Buffer.ElementOffset = firstElement * elementWidth;
        desc.Buffer.ElementWidth = elementWidth;
      }

      ID3D11ShaderResourceView* srv = nullptr;

      auto d3d11rs = static_cast<D3D11RenderAPI*>(D3D11RenderAPI::instancePtr());
      D3D11Device& device = d3d11rs->getPrimaryDevice();

      HRESULT hr = device.getD3D11Device()->CreateShaderResourceView(buffer->getDX11Buffer(),
                                                                     &desc,
                                                                     &srv);

      if (FAILED(hr) || device.hasError()) {
        String msg = device.getErrorDescription();
        GE_EXCEPT(RenderingAPIException,
                  "Cannot create ShaderResourceView: " + msg);
      }

      return srv;
    }

    ID3D11UnorderedAccessView*
    GPUBufferView::createUAV(D3D11GPUBuffer* buffer,
                             uint32 firstElement,
                             uint32 numElements,
                             bool useCounter) {
      const GPUBufferProperties& props = buffer->getProperties();

      D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
      memset(&desc, 0, sizeof(desc));

      desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

      if (props.getType() == GPU_BUFFER_TYPE::kSTANDARD) {
        desc.Format = D3D11Mappings::getBF(props.getFormat());
        desc.Buffer.FirstElement = firstElement;
        desc.Buffer.NumElements = numElements;

        if (useCounter) {
          desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
        }
        else {
          desc.Buffer.Flags = 0;
        }
      }
      else if (props.getType() == GPU_BUFFER_TYPE::kSTRUCTURED) {
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Buffer.FirstElement = firstElement;
        desc.Buffer.NumElements = numElements;

        if (useCounter) {
          desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
        }
        else {
          desc.Buffer.Flags = 0;
        }
      }
      else if (props.getType() == GPU_BUFFER_TYPE::kINDIRECTARGUMENT) {
        desc.Format = DXGI_FORMAT_R32_UINT;
        desc.Buffer.Flags = 0;
        desc.Buffer.FirstElement = firstElement;
        desc.Buffer.NumElements = numElements;
      }

      ID3D11UnorderedAccessView* uav = nullptr;

      auto d3d11rs = static_cast<D3D11RenderAPI*>(D3D11RenderAPI::instancePtr());
      D3D11Device& device = d3d11rs->getPrimaryDevice();

      HRESULT hr = device.getD3D11Device()->CreateUnorderedAccessView(buffer->getDX11Buffer(),
                                                                      &desc,
                                                                      &uav);

      if (FAILED(hr) || device.hasError()) {
        String msg = device.getErrorDescription();
        GE_EXCEPT(RenderingAPIException,
                  "Cannot create UnorderedAccessView: " + msg);
      }

      return uav;
    }
  }
}
