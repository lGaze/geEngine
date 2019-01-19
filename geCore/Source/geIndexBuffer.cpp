/*****************************************************************************/
/**
 * @file    geIndexBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Hardware buffer that hold indices of vertices in a vertex buffer.
 *
 * Hardware buffer that hold indices of vertices in a vertex buffer.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geIndexBuffer.h"
#include "geHardwareBufferManager.h"
#include "geRenderAPI.h"
#include "geGPUBuffer.h"
#include "geRenderStats.h"

namespace geEngineSDK {
  using std::static_pointer_cast;

  uint32
  calcIndexSize(IndexType type) {
    switch (type)
    {
      case IT_16BIT:
        return sizeof(uint16);
      default:
      case IT_32BIT:
        return sizeof(uint32);
    }
  }

  void
  checkValidDesc(const INDEX_BUFFER_DESC& desc) {
    if (0 == desc.numIndices) {
      GE_EXCEPT(InvalidParametersException,
                "Index buffer index count is not allowed to be zero.");
    }
  }

  IndexBufferProperties::IndexBufferProperties(IndexType idxType,
                                               uint32 numIndices)
    : m_indexType(idxType),
      m_numIndices(numIndices),
      m_indexSize(calcIndexSize(idxType))
  {}

  IndexBuffer::IndexBuffer(const INDEX_BUFFER_DESC& desc)
    : m_properties(desc.indexType, desc.numIndices),
      m_usage(desc.usage) {
#if GE_DEBUG_MODE
    checkValidDesc(desc);
#endif
  }

  SPtr<geCoreThread::IndexBuffer>
  IndexBuffer::getCore() const {
    return static_pointer_cast<geCoreThread::IndexBuffer>(m_coreSpecific);
  }

  SPtr<geCoreThread::CoreObject>
  IndexBuffer::createCore() const {
    INDEX_BUFFER_DESC desc;
    desc.indexType = m_properties.m_indexType;
    desc.numIndices = m_properties.m_numIndices;
    desc.usage = m_usage;

    return geCoreThread::HardwareBufferManager::instance().createIndexBufferInternal(desc);
  }

  SPtr<IndexBuffer>
  IndexBuffer::create(const INDEX_BUFFER_DESC& desc) {
    return HardwareBufferManager::instance().createIndexBuffer(desc);
  }

  namespace geCoreThread {
    IndexBuffer::IndexBuffer(const INDEX_BUFFER_DESC& desc,
                             GPU_DEVICE_FLAGS::E deviceMask)
      : HardwareBuffer(calcIndexSize(desc.indexType) * desc.numIndices,
                       desc.usage,
                       deviceMask),
        m_properties(desc.indexType, desc.numIndices) {
#if GE_DEBUG_MODE
      checkValidDesc(desc);
#endif
    }

    IndexBuffer::~IndexBuffer() {
      if (m_buffer && !m_sharedBuffer) {
        m_bufferDeleter(m_buffer);
      }

      GE_INC_RENDER_STAT_CAT(ResDestroyed, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
    }

    void
    IndexBuffer::initialize() {
      GE_INC_RENDER_STAT_CAT(ResCreated, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
      CoreObject::initialize();
    }

    void*
    IndexBuffer::map(uint32 offset,
                     uint32 length,
                     GPU_LOCK_OPTIONS::E options,
                     uint32 deviceIdx,
                     uint32 queueIdx) {
#if GE_PROFILING_ENABLED
      if (GPU_LOCK_OPTIONS::kREAD_ONLY == options ||
          GPU_LOCK_OPTIONS::kREAD_WRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
      }

      if (GPU_LOCK_OPTIONS::kREAD_WRITE == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
      }
#endif
      return m_buffer->lock(offset, length, options, deviceIdx, queueIdx);
    }

    void
    IndexBuffer::unmap() {
      m_buffer->unlock();
    }

    void
    IndexBuffer::readData(uint32 offset,
                          uint32 length,
                          void* dest,
                          uint32 deviceIdx,
                          uint32 queueIdx) {
      m_buffer->readData(offset, length, dest, deviceIdx, queueIdx);
      GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
    }

    void
    IndexBuffer::writeData(uint32 offset,
                           uint32 length,
                           const void* source,
                           BUFFER_WRITE_TYPE::E writeFlags,
                           uint32 queueIdx) {
      m_buffer->writeData(offset, length, source, writeFlags, queueIdx);
      GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kIndexBuffer);
    }

    void
    IndexBuffer::copyData(HardwareBuffer& srcBuffer,
                          uint32 srcOffset,
                          uint32 dstOffset,
                          uint32 length,
                          bool discardWholeBuffer,
                          const SPtr<CommandBuffer>& commandBuffer) {
      auto& srcIndexBuffer = static_cast<IndexBuffer&>(srcBuffer);
      m_buffer->copyData(*srcIndexBuffer.m_buffer,
                         srcOffset,
                         dstOffset,
                         length,
                         discardWholeBuffer,
                         commandBuffer);
    }

    SPtr<GPUBuffer>
    IndexBuffer::getLoadStore(GPU_BUFFER_TYPE::E type,
                              GPU_BUFFER_FORMAT::E format,
                              uint32 elementSize) {
      if ((m_usage & GPU_BUFFER_USAGE::kLOADSTORE) != GPU_BUFFER_USAGE::kLOADSTORE) {
        return nullptr;
      }

      for (const auto& entry : m_loadStoreViews) {
        const GPUBufferProperties& props = entry->getProperties();
        if (props.getType() == type) {
          if (GPU_BUFFER_TYPE::kSTANDARD == type && props.getFormat() == format) {
            return entry;
          }

          if (type == GPU_BUFFER_TYPE::kSTRUCTURED && props.getElementSize() == elementSize)
            return entry;
        }
      }

      uint32 elemSize = GPU_BUFFER_TYPE::kSTANDARD == type ?
                          geEngineSDK::GPUBuffer::getFormatSize(format) : elementSize;
      if ((m_buffer->getSize() % elemSize) != 0) {
        LOGERR("Size of the buffer isn't divisible by individual element "
               "size provided for the buffer view.");
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
      }

      SPtr<GPUBuffer> newView = GPUBuffer::create(desc, m_sharedBuffer);
      m_loadStoreViews.push_back(newView);

      return newView;
    }

    SPtr<IndexBuffer>
    IndexBuffer::create(const INDEX_BUFFER_DESC& desc,
                        GPU_DEVICE_FLAGS::E deviceMask) {
      return HardwareBufferManager::instance().createIndexBuffer(desc, deviceMask);
    }
  }
}
