/*****************************************************************************/
/**
 * @file    geD3D11HardwareBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/13
 * @brief   Common functionality for all DirectX 11 hardware buffers.
 *
 * Common functionality for all DirectX 11 hardware buffers.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11HardwareBuffer.h"
#include "geD3D11Mappings.h"
#include "geD3D11Device.h"
#include "geD3D11CommandBuffer.h"

#include <geException.h>
#include <geDebug.h>


namespace geEngineSDK {
  using std::ref;
  using std::static_pointer_cast;

  namespace geCoreThread {
    D3D11HardwareBuffer::D3D11HardwareBuffer(BUFFER_TYPE::E btype,
                                             GPU_BUFFER_USAGE::E usage,
                                             uint32 elementCount,
                                             uint32 elementSize,
                                             D3D11Device& device,
                                             bool useSystemMem,
                                             bool streamOut)
      : HardwareBuffer(elementCount * elementSize, usage, GPU_DEVICE_FLAGS::kDEFAULT),
        m_bufferType(btype),
        m_device(device),
        m_elementCount(elementCount),
        m_elementSize(elementSize),
        m_usage(usage) {
      bool isLoadStore = (usage & GPU_BUFFER_USAGE::kLOADSTORE) ==
                                  GPU_BUFFER_USAGE::kLOADSTORE;

      if (useSystemMem) {
        if (isLoadStore) {
          LOGWRN("LoadStore usage and useSystemMem cannot be used together.");
          isLoadStore = false;
        }

        if (streamOut) {
          LOGWRN("useSystemMem and streamOut cannot be used together.");
          streamOut = false;
        }
      }

      if (isLoadStore) {
        if (BUFFER_TYPE::kCONSTANT == btype) {
          LOGWRN("Constant buffers cannot be bound with LoadStore usage.");
          isLoadStore = false;
        }

        if (D3D11Mappings::isDynamic(usage)) {
          LOGWRN("Dynamic usage not supported with LoadStore usage.");
          usage = static_cast<GPU_BUFFER_USAGE::E>(usage & ~GPU_BUFFER_USAGE::kDYNAMIC);
        }
      }

      if (streamOut) {
        if (BUFFER_TYPE::kCONSTANT == btype) {
          LOGWRN("Constant buffers cannot be used with streamOut.");
          streamOut = false;
        }
      }

      m_desc.ByteWidth = getSize();
      m_desc.MiscFlags = 0;
      m_desc.StructureByteStride = 0;

      if (useSystemMem) {
        m_desc.Usage = D3D11_USAGE_STAGING;
        m_desc.BindFlags = 0;
        m_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
      }
      else {
        m_desc.Usage = D3D11Mappings::getUsage(usage);
        m_desc.CPUAccessFlags = D3D11Mappings::getAccessFlags(usage);

        switch (btype)
        {
          case BUFFER_TYPE::kSTANDARD:
            m_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            break;
          case BUFFER_TYPE::kSTRUCTURED:
            m_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            m_desc.StructureByteStride = elementSize;
            m_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            break;
          case BUFFER_TYPE::kRAW:
            m_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            m_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            break;
          case BUFFER_TYPE::kVERTEX:
            m_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            break;
          case BUFFER_TYPE::kINDEX:
            m_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            break;
          case BUFFER_TYPE::kCONSTANT:
            m_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            break;
          case BUFFER_TYPE::kINDIRECTARGUMENT:
            m_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            m_desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
            break;
        }

        if (isLoadStore) {
          m_desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        }

        if (streamOut) {
          m_desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
        }
      }

      HRESULT hr = device.getD3D11Device()->CreateBuffer(&m_desc, nullptr, &m_d3dBuffer);
      if (FAILED(hr) || m_device.hasError()) {
        String msg = device.getErrorDescription();
        GE_EXCEPT(RenderingAPIException,
                  "Cannot create D3D11 buffer: " + msg);
      }
    }

    D3D11HardwareBuffer::~D3D11HardwareBuffer() {
      SAFE_RELEASE(m_d3dBuffer);

      if (nullptr != m_pTempStagingBuffer) {
        ge_delete(m_pTempStagingBuffer);
      }
    }

    void*
    D3D11HardwareBuffer::map(uint32 offset,
                             uint32 length,
                             GPU_LOCK_OPTIONS::E options,
                             uint32 /*deviceIdx*/,
                             uint32 /*queueIdx*/) {
      if (length > m_size) {
        GE_EXCEPT(RenderingAPIException,
                  "Provided length " + toString(length) +
                  " larger than the buffer " + toString(m_size) + ".");
      }

      //Use direct (and faster) Map/Unmap if dynamic write, or a staging read/write
      if ((D3D11_USAGE_DYNAMIC == m_desc.Usage && GPU_LOCK_OPTIONS::kREAD_ONLY != options) ||
          D3D11_USAGE_STAGING == m_desc.Usage) {
        D3D11_MAP mapType;

        switch (options)
        {
          case GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD:
            if (m_usage & GPU_BUFFER_USAGE::kDYNAMIC) {
              mapType = D3D11_MAP_WRITE_DISCARD;
            }
            else {
              //Map cannot be called with MAP_WRITE_DISCARD access, because the
              //Resource was not created as D3D11_USAGE_DYNAMIC.
              //D3D11_USAGE_DYNAMIC Resources must use either MAP_WRITE_DISCARD
              //or MAP_WRITE_NO_OVERWRITE with Map.
              mapType = D3D11_MAP_WRITE;
            }
            break;
          case GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE:
            if (BUFFER_TYPE::kINDEX == m_bufferType || BUFFER_TYPE::kVERTEX == m_bufferType) {
              mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
            }
            else {
              const auto& featureOptions = m_device.getFeatureOptions();

              if (BUFFER_TYPE::kCONSTANT == m_bufferType &&
                  featureOptions.MapNoOverwriteOnDynamicConstantBuffer) {
                mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
              }
              else if (featureOptions.MapNoOverwriteOnDynamicBufferSRV) {
                mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
              }
              else {
                mapType = D3D11_MAP_WRITE;
              }
            }
            break;
          case GPU_LOCK_OPTIONS::kWRITE_ONLY:
            mapType = D3D11_MAP_WRITE;
            break;
          case GPU_LOCK_OPTIONS::kREAD_WRITE:
            if ((m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) != 0 &&
                (m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0) {
              mapType = D3D11_MAP_READ_WRITE;
            }
            else if (m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) {
              mapType = D3D11_MAP_WRITE;
            }
            else {
              mapType = D3D11_MAP_READ;
            }
            break;
          default:
          case GPU_LOCK_OPTIONS::kREAD_ONLY:
            mapType = D3D11_MAP_READ;
            break;
        }

        if (D3D11Mappings::isMappingRead(mapType) &&
            (m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) == 0) {
          LOGERR("Trying to read a buffer, but buffer wasn't created with a "
                 "read access flag.");
        }

        if (D3D11Mappings::isMappingWrite(mapType) &&
            (m_desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) == 0) {
          LOGERR("Trying to write to a buffer, but buffer wasn't created "
                 "with a write access flag.");
        }

        void * pRet = nullptr;
        D3D11_MAPPED_SUBRESOURCE mappedSubResource;
        mappedSubResource.pData = nullptr;
        m_device.clearErrors();

        HRESULT hr = m_device.getImmediateContext()->Map(m_d3dBuffer,
                                                         0,
                                                         mapType,
                                                         0,
                                                         &mappedSubResource);
        if (FAILED(hr) || m_device.hasError()) {
          String msg = m_device.getErrorDescription();
          GE_EXCEPT(RenderingAPIException,
                    "Error calling Map: " + msg);
        }

        pRet = static_cast<void*>(static_cast<char*>(mappedSubResource.pData) + offset);

        return pRet;
      }
      else {
        //Otherwise create a staging buffer to do all read/write operations on.
        //Usually try to avoid this.

        m_useTempStagingBuffer = true;
        if (!m_pTempStagingBuffer) {
          //Create another buffer instance but use system memory
          m_pTempStagingBuffer = ge_new<D3D11HardwareBuffer>(m_bufferType,
                                                             GPU_BUFFER_USAGE::kSTATIC,
                                                             1,
                                                             m_size,
                                                             ref(m_device),
                                                             true);
        }

        //Schedule a copy to the staging
        if (GPU_LOCK_OPTIONS::kREAD_ONLY == options ||
            GPU_LOCK_OPTIONS::kREAD_WRITE == options) {
          m_pTempStagingBuffer->copyData(*this, 0, 0, m_size, true);
        }

        //Register whether we'll need to upload on unlock
        m_stagingUploadNeeded = (GPU_LOCK_OPTIONS::kREAD_ONLY != options);

        return m_pTempStagingBuffer->lock(offset, length, options);
      }
    }

    void
    D3D11HardwareBuffer::unmap() {
      if (m_useTempStagingBuffer) {
        m_useTempStagingBuffer = false;

        m_pTempStagingBuffer->unlock();

        if (m_stagingUploadNeeded) {
          copyData(*m_pTempStagingBuffer, 0, 0, m_size, true);
        }

        if (nullptr != m_pTempStagingBuffer) {
          ge_delete(m_pTempStagingBuffer);
          m_pTempStagingBuffer = nullptr;
        }
      }
      else {
        m_device.getImmediateContext()->Unmap(m_d3dBuffer, 0);
      }
    }

    void
    D3D11HardwareBuffer::copyData(HardwareBuffer& srcBuffer,
                                  uint32 srcOffset,
                                  uint32 dstOffset,
                                  uint32 length,
                                  bool /*discardWholeBuffer*/,
                                  const SPtr<geCoreThread::CommandBuffer>& commandBuffer) {
      auto executeRef =
      [this](HardwareBuffer& srcBuffer, uint32 srcOffset, uint32 dstOffset, uint32 length)
      {
        //If we're copying same-size buffers in their entirety
        if (0 == srcOffset &&
            0 == dstOffset &&
            length == m_size &&
            m_size == srcBuffer.getSize()) {
          m_device.getImmediateContext()->CopyResource(m_d3dBuffer,
            static_cast<D3D11HardwareBuffer&>(srcBuffer).getD3DBuffer());
          if (m_device.hasError()) {
            String errorDescription = m_device.getErrorDescription();
            GE_EXCEPT(RenderingAPIException,
                      "Cannot copy D3D11 resource\n"
                      "Error Description:" + errorDescription);
          }
        }
        else {
          //Copy subregion
          D3D11_BOX srcBox;
          srcBox.left = srcOffset;
          srcBox.right = srcOffset + length;
          srcBox.top = 0;
          srcBox.bottom = 1;
          srcBox.front = 0;
          srcBox.back = 1;

          m_device.getImmediateContext()->
            CopySubresourceRegion(m_d3dBuffer,
                                  0,
                                  dstOffset,
                                  0,
                                  0,
                                  static_cast<D3D11HardwareBuffer&>(srcBuffer).getD3DBuffer(),
                                  0,
                                  &srcBox);

          if (m_device.hasError()) {
            String errorDescription = m_device.getErrorDescription();
            GE_EXCEPT(RenderingAPIException,
                      "Cannot copy D3D11 subresource region\n"
                      "Error Description:" + errorDescription);
          }
        }
      };

      if (nullptr == commandBuffer) {
        executeRef(srcBuffer, srcOffset, dstOffset, length);
      }
      else {
        auto execute = [&]() { executeRef(srcBuffer, srcOffset, dstOffset, length); };
        auto cb = static_pointer_cast<D3D11CommandBuffer>(commandBuffer);
        cb->queueCommand(execute);
      }
    }

    void
    D3D11HardwareBuffer::readData(uint32 offset,
                                  uint32 length,
                                  void* dest,
                                  uint32 /*deviceIdx*/,
                                  uint32 /*queueIdx*/) {
      //There is no functional interface in D3D, just do via manual lock,
      //copy & unlock
      void* pSrc = this->lock(offset, length, GPU_LOCK_OPTIONS::kREAD_ONLY);
      memcpy(dest, pSrc, length);
      this->unlock();
    }

    void
    D3D11HardwareBuffer::writeData(uint32 offset,
                                   uint32 length,
                                   const void* pSource,
                                   BUFFER_WRITE_TYPE::E writeFlags,
                                   uint32 /*queueIdx*/) {
      if (D3D11_USAGE_DYNAMIC == m_desc.Usage ||
          D3D11_USAGE_STAGING == m_desc.Usage) {
        GPU_LOCK_OPTIONS::E lockOption = GPU_LOCK_OPTIONS::kWRITE_ONLY;
        if (BUFFER_WRITE_TYPE::kDISCARD == writeFlags) {
          lockOption = GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD;
        }
        else if (BUFFER_WRITE_TYPE::kNO_OVERWRITE == writeFlags) {
          lockOption = GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE;
        }

        void* pDst = this->lock(offset, length, lockOption);
        memcpy(pDst, pSource, length);
        this->unlock();
      }
      else if (D3D11_USAGE_DEFAULT == m_desc.Usage) {
        if (BUFFER_TYPE::kCONSTANT == m_bufferType) {
          GE_ASSERT(0 == offset);

          //Constant buffer cannot be updated partially using UpdateSubresource
          m_device.getImmediateContext()->UpdateSubresource(m_d3dBuffer,
                                                            0,
                                                            nullptr,
                                                            pSource,
                                                            0,
                                                            0);
        }
        else {
          D3D11_BOX dstBox;
          dstBox.left = offset;
          dstBox.right = offset + length;
          dstBox.top = 0;
          dstBox.bottom = 1;
          dstBox.front = 0;
          dstBox.back = 1;

          m_device.getImmediateContext()->UpdateSubresource(m_d3dBuffer,
                                                            0,
                                                            &dstBox,
                                                            pSource,
                                                            0,
                                                            0);
        }
      }
      else {
        LOGERR("Trying to write into a buffer with unsupported usage: " +
               toString(m_desc.Usage));
      }
    }
  }
}
