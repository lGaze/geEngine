/*****************************************************************************/
/**
 * @file    geGPUParamBlockBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/11
 * @brief   Represents a GPU parameter block buffer.
 *
 * Represents a GPU parameter block buffer. Parameter block buffers are bound
 * to GPU programs which then fetch parameters from those buffers.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geGPUParamBlockBuffer.h"
#include "geHardwareBufferManager.h"

namespace geEngineSDK {
  using std::static_pointer_cast;

  GPUParamBlockBuffer::GPUParamBlockBuffer(uint32 size, GPU_BUFFER_USAGE::E usage)
    : m_usage(usage),
      m_size(size),
      m_cachedData(nullptr) {
    if (m_size > 0) {
      m_cachedData = reinterpret_cast<uint8*>(ge_alloc(m_size));
      memset(m_cachedData, 0, m_size);
    }
  }

  GPUParamBlockBuffer::~GPUParamBlockBuffer() {
    if (nullptr != m_cachedData) {
      ge_free(m_cachedData);
    }
  }

  void
  GPUParamBlockBuffer::write(uint32 offset,
                             const void* data,
                             uint32 size) {
#if GE_DEBUG_MODE
    if ((offset + size) > m_size) {
      GE_EXCEPT(InvalidParametersException,
                "Wanted range is out of buffer bounds. "
                "Available range: 0 .. " + toString(m_size) + ". "
                "Wanted range: " + toString(offset) + " .. " +
                toString(offset + size) + ".");
    }
#endif
    memcpy(m_cachedData + offset, data, size);
    markCoreDirty();
  }

  void
  GPUParamBlockBuffer::read(uint32 offset, void* data, uint32 size) {
#if GE_DEBUG_MODE
    if ((offset + size) > m_size) {
      GE_EXCEPT(InvalidParametersException,
                "Wanted range is out of buffer bounds. "
                "Available range: 0 .. " + toString(m_size) + ". "
                "Wanted range: " + toString(offset) + " .. " +
                toString(offset + size) + ".");
    }
#endif
    memcpy(data, m_cachedData + offset, size);
  }

  void
  GPUParamBlockBuffer::zeroOut(uint32 offset, uint32 size) {
#if GE_DEBUG_MODE
    if ((offset + size) > m_size) {
      GE_EXCEPT(InvalidParametersException,
              "Wanted range is out of buffer bounds. "
              "Available range: 0 .. " + toString(m_size) + ". "
              "Wanted range: " + toString(offset) + " .. " +
              toString(offset + size) + ".");
    }
#endif
    memset(m_cachedData + offset, 0, size);
    markCoreDirty();
  }

  SPtr<geCoreThread::GPUParamBlockBuffer>
  GPUParamBlockBuffer::getCore() const {
    return static_pointer_cast<geCoreThread::GPUParamBlockBuffer>(m_coreSpecific);
  }

  SPtr<geCoreThread::CoreObject>
  GPUParamBlockBuffer::createCore() const {
    return geCoreThread::HardwareBufferManager::instance().
      createGPUParamBlockBufferInternal(m_size, m_usage);
  }

  CoreSyncData
  GPUParamBlockBuffer::syncToCore(FrameAlloc* allocator) {
    uint8* buffer = allocator->alloc(static_cast<SIZE_T>(m_size));
    read(0, buffer, m_size);

    return CoreSyncData(buffer, m_size);
  }

  SPtr<GPUParamBlockBuffer>
  GPUParamBlockBuffer::create(uint32 size, GPU_BUFFER_USAGE::E usage) {
    return HardwareBufferManager::instance().createGPUParamBlockBuffer(size, usage);
  }

  namespace geCoreThread {
    GPUParamBlockBuffer::GPUParamBlockBuffer(uint32 size,
                                             GPU_BUFFER_USAGE::E usage,
                                             GPU_DEVICE_FLAGS::E /*deviceMask*/)
      : m_usage(usage),
        m_size(size),
        m_cachedData(nullptr),
        m_gpuBufferDirty(false) {
      if (m_size > 0) {
        m_cachedData = reinterpret_cast<uint8*>(ge_alloc(m_size));
        memset(m_cachedData, 0, m_size);
      }
    }

    GPUParamBlockBuffer::~GPUParamBlockBuffer() {
      if (nullptr != m_cachedData) {
        ge_free(m_cachedData);
      }
    }

    void
    GPUParamBlockBuffer::write(uint32 offset, const void* data, uint32 size) {
#if GE_DEBUG_MODE
      if ((offset + size) > m_size) {
        GE_EXCEPT(InvalidParametersException,
                  "Wanted range is out of buffer bounds. "
                  "Available range: 0 .. " + toString(m_size) + ". "
                  "Wanted range: " + toString(offset) + " .. " +
                  toString(offset + size) + ".");
      }
#endif
      memcpy(m_cachedData + offset, data, size);
      m_gpuBufferDirty = true;
    }

    void
    GPUParamBlockBuffer::read(uint32 offset, void* data, uint32 size) {
#if GE_DEBUG_MODE
      if ((offset + size) > m_size) {
        GE_EXCEPT(InvalidParametersException,
                  "Wanted range is out of buffer bounds. "
                  "Available range: 0 .. " + toString(m_size) + ". "
                  "Wanted range: " + toString(offset) + " .. " +
                  toString(offset + size) + ".");
      }
#endif
      memcpy(data, m_cachedData + offset, size);
    }

    void
    GPUParamBlockBuffer::zeroOut(uint32 offset, uint32 size) {
#if GE_DEBUG_MODE
      if ((offset + size) > m_size) {
        GE_EXCEPT(InvalidParametersException,
                  "Wanted range is out of buffer bounds. "
                  "Available range: 0 .. " + toString(m_size) + ". "
                  "Wanted range: " + toString(offset) + " .. " +
                  toString(offset + size) + ".");
      }
#endif
      memset(m_cachedData + offset, 0, size);
      m_gpuBufferDirty = true;
    }

    void
    GPUParamBlockBuffer::flushToGPU(uint32 queueIdx) {
      if (m_gpuBufferDirty) {
        writeToGPU(m_cachedData, queueIdx);
        m_gpuBufferDirty = false;
      }
    }

    void
    GPUParamBlockBuffer::syncToCore(const CoreSyncData& data) {
      GE_ASSERT(data.getBufferSize() == m_size);
      write(0, data.getBuffer(), data.getBufferSize());
    }

    SPtr<GPUParamBlockBuffer>
    GPUParamBlockBuffer::create(uint32 size,
                                GPU_BUFFER_USAGE::E usage,
                                GPU_DEVICE_FLAGS::E deviceMask) {
      return HardwareBufferManager::instance().createGPUParamBlockBuffer(size,
                                                                         usage,
                                                                         deviceMask);
    }
  }
}
