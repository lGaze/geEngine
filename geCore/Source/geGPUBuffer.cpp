/*****************************************************************************/
/**
 * @file    geGPUBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Handles a generic GPU buffer.
 *
 * Handles a generic GPU buffer that you may use for storing any kind of data.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geGPUBuffer.h"
#include "geRenderAPI.h"
#include "geHardwareBufferManager.h"
#include "geRenderStats.h"

#include <geException.h>

namespace geEngineSDK {
  using std::move;
  using std::static_pointer_cast;

  uint32
  getBufferSize(const GPU_BUFFER_DESC& desc) {
    uint32 elementSize;

    if (GPU_BUFFER_TYPE::kSTANDARD == desc.type) {
      elementSize = GPUBuffer::getFormatSize(desc.format);
    }
    else {
      elementSize = desc.elementSize;
    }

    return elementSize * desc.elementCount;
  }

  GPUBufferProperties::GPUBufferProperties(const GPU_BUFFER_DESC& desc)
    : m_desc(desc) {
    if (GPU_BUFFER_TYPE::kSTANDARD == m_desc.type) {
      m_desc.elementSize = GPUBuffer::getFormatSize(m_desc.format);
    }
  }

  GPUBuffer::GPUBuffer(const GPU_BUFFER_DESC& desc)
    : m_properties(desc)
  {}

  SPtr<geCoreThread::GPUBuffer>
  GPUBuffer::getCore() const {
    return static_pointer_cast<geCoreThread::GPUBuffer>(m_coreSpecific);
  }

  SPtr<geCoreThread::CoreObject>
  GPUBuffer::createCore() const {
    return geCoreThread::HardwareBufferManager::instance().
             createGPUBufferInternal(m_properties.m_desc);
  }

  uint32
  GPUBuffer::getFormatSize(GPU_BUFFER_FORMAT::E format) {
    static bool lookupInitialized = false;
    static uint32 lookup[GPU_BUFFER_FORMAT::kCOUNT];

    if (!lookupInitialized) {
      lookup[GPU_BUFFER_FORMAT::k16X1F] = 2;
      lookup[GPU_BUFFER_FORMAT::k16X2F] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X4F] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X1F] = 4;
      lookup[GPU_BUFFER_FORMAT::k32X2F] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X3F] = 12;
      lookup[GPU_BUFFER_FORMAT::k32X4F] = 16;
      lookup[GPU_BUFFER_FORMAT::k8X1] = 1;
      lookup[GPU_BUFFER_FORMAT::k8X2] = 2;
      lookup[GPU_BUFFER_FORMAT::k8X4] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X1] = 2;
      lookup[GPU_BUFFER_FORMAT::k16X2] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X4] = 8;
      lookup[GPU_BUFFER_FORMAT::k8X1S] = 1;
      lookup[GPU_BUFFER_FORMAT::k8X2S] = 2;
      lookup[GPU_BUFFER_FORMAT::k8X4S] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X1S] = 2;
      lookup[GPU_BUFFER_FORMAT::k16X2S] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X4S] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X1S] = 4;
      lookup[GPU_BUFFER_FORMAT::k32X2S] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X3S] = 12;
      lookup[GPU_BUFFER_FORMAT::k32X4S] = 16;
      lookup[GPU_BUFFER_FORMAT::k8X1U] = 1;
      lookup[GPU_BUFFER_FORMAT::k8X2U] = 2;
      lookup[GPU_BUFFER_FORMAT::k8X4U] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X1U] = 2;
      lookup[GPU_BUFFER_FORMAT::k16X2U] = 4;
      lookup[GPU_BUFFER_FORMAT::k16X4U] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X1U] = 4;
      lookup[GPU_BUFFER_FORMAT::k32X2U] = 8;
      lookup[GPU_BUFFER_FORMAT::k32X3U] = 12;
      lookup[GPU_BUFFER_FORMAT::k32X4U] = 16;
      lookupInitialized = true;
    }

    if (GPU_BUFFER_FORMAT::kCOUNT <= format) {
      return 0;
    }

    return lookup[static_cast<uint32>(format)];
  }

  SPtr<GPUBuffer>
  GPUBuffer::create(const GPU_BUFFER_DESC& desc) {
    return HardwareBufferManager::instance().createGPUBuffer(desc);
  }

  namespace geCoreThread {
    GPUBuffer::GPUBuffer(const GPU_BUFFER_DESC& desc,
                         GPU_DEVICE_FLAGS::E deviceMask)
      : HardwareBuffer(getBufferSize(desc), desc.usage, deviceMask),
        m_properties(desc) {
      if (GPU_BUFFER_TYPE::kSTANDARD != desc.type) {
        GE_ASSERT(GPU_BUFFER_FORMAT::kUNKNOWN == desc.format &&
                  "Format must be set to GPU_BUFFER_FORMAT::kUNKNOWN when "
                  "using non-standard buffers");
      }
      else {
        GE_ASSERT(0 == desc.elementSize &&
                  "No element size can be provided for standard buffer. "
                  "Size is determined from format.");
      }
    }

    GPUBuffer::GPUBuffer(const GPU_BUFFER_DESC& desc,
                         SPtr<HardwareBuffer> underlyingBuffer)
      : HardwareBuffer(getBufferSize(desc), desc.usage, underlyingBuffer->getDeviceMask()),
        m_properties(desc),
        m_buffer(underlyingBuffer.get()),
        m_sharedBuffer(move(underlyingBuffer)),
        m_isExternalBuffer(true) {
#if GE_DEBUG_MODE
      const auto& props = getProperties();
#endif
      GE_ASSERT(m_sharedBuffer->getSize() ==
                (props.getElementCount() * props.getElementSize()));
      
      if (GPU_BUFFER_TYPE::kSTANDARD != desc.type) {
        GE_ASSERT(GPU_BUFFER_FORMAT::kUNKNOWN == desc.format &&
                  "Format must be set to GPU_BUFFER_FORMAT::kUNKNOWN when "
                  "using non-standard buffers");
      }
      else {
        GE_ASSERT(0 == desc.elementSize &&
                  "No element size can be provided for standard buffer. "
                  "Size is determined from format.");
      }
    }

    GPUBuffer::~GPUBuffer() {
      GE_INC_RENDER_STAT_CAT(ResDestroyed, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);

      if (m_buffer && !m_sharedBuffer) {
        m_bufferDeleter(m_buffer);
      }
    }

    void
    GPUBuffer::initialize() {
      GE_INC_RENDER_STAT_CAT(ResCreated, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      CoreObject::initialize();
    }

    void*
    GPUBuffer::map(uint32 offset,
                   uint32 length,
                   GPU_LOCK_OPTIONS::E options,
                   uint32 deviceIdx,
                   uint32 queueIdx) {
#if GE_PROFILING_ENABLED
      if (GPU_LOCK_OPTIONS::kREAD_ONLY == options ||
          GPU_LOCK_OPTIONS::kREAD_WRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      }

      if (GPU_LOCK_OPTIONS::kREAD_WRITE == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD == options||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      }
#endif

      return m_buffer->lock(offset, length, options, deviceIdx, queueIdx);
    }

    void
    GPUBuffer::unmap() {
      m_buffer->unlock();
    }

    void
    GPUBuffer::readData(uint32 offset,
                        uint32 length,
                        void* dest,
                        uint32 deviceIdx,
                        uint32 queueIdx) {
      GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);

      m_buffer->readData(offset, length, dest, deviceIdx, queueIdx);
    }

    void
    GPUBuffer::writeData(uint32 offset,
                         uint32 length,
                         const void* source,
                         BUFFER_WRITE_TYPE::E writeFlags,
                         uint32 queueIdx) {
      GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);

      m_buffer->writeData(offset, length, source, writeFlags, queueIdx);
    }

    void
    GPUBuffer::copyData(HardwareBuffer& srcBuffer,
                        uint32 srcOffset,
                        uint32 dstOffset,
                        uint32 length,
                        bool discardWholeBuffer,
                        const SPtr<CommandBuffer>& commandBuffer) {
      auto& srcGpuBuffer = static_cast<GPUBuffer&>(srcBuffer);
      m_buffer->copyData(*srcGpuBuffer.m_buffer,
                         srcOffset,
                         dstOffset,
                         length,
                         discardWholeBuffer,
                         commandBuffer);
    }

    SPtr<GPUBuffer>
    GPUBuffer::getView(GPU_BUFFER_TYPE::E type,
                       GPU_BUFFER_FORMAT::E format,
                       uint32 elementSize) {
      const uint32 elemSize = type == GPU_BUFFER_TYPE::kSTANDARD ?
        geEngineSDK::GPUBuffer::getFormatSize(format) : elementSize;

      if ((m_buffer->getSize() % elemSize) != 0) {
        LOGERR("Size of the buffer isn't divisible by individual element size "
               "provided for the buffer view.");
        return nullptr;
      }

      GPU_BUFFER_DESC desc;
      desc.type = type;
      desc.format = format;
      desc.usage = m_usage;
      desc.elementSize = elementSize;
      desc.elementCount = m_buffer->getSize() / elemSize;

      if (!m_sharedBuffer) {
        m_sharedBuffer = ge_shared_ptr(m_buffer, m_bufferDeleter);
        m_isExternalBuffer = false;
      }

      SPtr<GPUBuffer> newView = create(desc, m_sharedBuffer);
      return newView;
    }

    SPtr<GPUBuffer>
    GPUBuffer::create(const GPU_BUFFER_DESC& desc,
                      GPU_DEVICE_FLAGS::E deviceMask) {
      return HardwareBufferManager::instance().createGPUBuffer(desc, deviceMask);
    }

    SPtr<GPUBuffer>
    GPUBuffer::create(const GPU_BUFFER_DESC& desc,
                      SPtr<HardwareBuffer> underlyingBuffer) {
      return HardwareBufferManager::instance().createGPUBuffer(desc, move(underlyingBuffer));
    }
  }
}
