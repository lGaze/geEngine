/*****************************************************************************/
/**
 * @file    geD3D11GPUParamBlockBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/16
 * @brief   DirectX 11 implementation of a parameter block buffer.
 *
 * DirectX 11 implementation of a parameter block buffer
 * (constant buffer in DX11 lingo).
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11GPUParamBlockBuffer.h"
#include "geD3D11HardwareBuffer.h"
#include "geD3D11RenderAPI.h"
#include "geD3D11Device.h"

#include <geRenderStats.h>

namespace geEngineSDK {
  namespace geCoreThread {
    D3D11GPUParamBlockBuffer::D3D11GPUParamBlockBuffer(uint32 size,
                                                       GPU_BUFFER_USAGE::E usage,
                                                       GPU_DEVICE_FLAGS::E deviceMask)
      : GPUParamBlockBuffer(size, usage, deviceMask) {
      GE_ASSERT((GPU_DEVICE_FLAGS::kDEFAULT == deviceMask||
                 GPU_DEVICE_FLAGS::kPRIMARY == deviceMask) &&
                "Multiple GPUs not supported natively on DirectX 11.");
    }

    D3D11GPUParamBlockBuffer::~D3D11GPUParamBlockBuffer() {
      if (nullptr != m_buffer) {
        ge_pool_delete(static_cast<D3D11HardwareBuffer*>(m_buffer));
      }
    }

    void
    D3D11GPUParamBlockBuffer::initialize() {
      auto d3d11rs = static_cast<D3D11RenderAPI*>(RenderAPI::instancePtr());
      D3D11Device& device = d3d11rs->getPrimaryDevice();

      m_buffer = ge_pool_new<D3D11HardwareBuffer>(BUFFER_TYPE::kCONSTANT,
                                                  m_usage,
                                                  1,
                                                  m_size,
                                                  device);


      GPUParamBlockBuffer::initialize();
    }


    ID3D11Buffer*
    D3D11GPUParamBlockBuffer::getD3D11Buffer() const {
      return static_cast<D3D11HardwareBuffer*>(m_buffer)->getD3DBuffer();
    }
  }
}
