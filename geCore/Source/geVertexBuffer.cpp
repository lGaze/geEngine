/*****************************************************************************/
/**
 * @file    geVertexBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Specialization of a hardware buffer used for holding vertex data.
 *
 * Specialization of a hardware buffer used for holding vertex data.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geVertexBuffer.h"
#include "geHardwareBufferManager.h"
#include "geRenderStats.h"
#include "geGPUBuffer.h"

namespace geEngineSDK {
  using std::static_pointer_cast;

  void
  checkValidDesc(const VERTEX_BUFFER_DESC& desc) {
    if (0 == desc.vertexSize) {
      GE_EXCEPT(InvalidParametersException,
                "Vertex buffer vertex size is not allowed to be zero.");
    }

    if (0 == desc.numVerts) {
      GE_EXCEPT(InvalidParametersException,
                "Vertex buffer vertex count is not allowed to be zero.");
    }
  }

  VertexBufferProperties::VertexBufferProperties(uint32 numVertices,
                                                 uint32 vertexSize)
    : m_numVertices(numVertices),
      m_vertexSize(vertexSize)
  {}

  VertexBuffer::VertexBuffer(const VERTEX_BUFFER_DESC& desc)
    : m_properties(desc.numVerts, desc.vertexSize),
      m_usage(desc.usage),
      m_streamOut(desc.streamOut) {
#if GE_DEBUG_MODE
    checkValidDesc(desc);
#endif
  }

  SPtr<geCoreThread::CoreObject>
  VertexBuffer::createCore() const {
    VERTEX_BUFFER_DESC desc;
    desc.vertexSize = m_properties.m_vertexSize;
    desc.numVerts = m_properties.m_numVertices;
    desc.usage = m_usage;
    desc.streamOut = m_streamOut;

    return geCoreThread::HardwareBufferManager::instance().
             createVertexBufferInternal(desc);
  }

  SPtr<geCoreThread::VertexBuffer>
  VertexBuffer::getCore() const {
    return static_pointer_cast<geCoreThread::VertexBuffer>(m_coreSpecific);
  }

  SPtr<VertexBuffer>
  VertexBuffer::create(const VERTEX_BUFFER_DESC& desc) {
    return HardwareBufferManager::instance().createVertexBuffer(desc);
  }

  namespace geCoreThread {
    VertexBuffer::VertexBuffer(const VERTEX_BUFFER_DESC& desc,
                               GPU_DEVICE_FLAGS::E deviceMask)
      : HardwareBuffer(desc.vertexSize * desc.numVerts, desc.usage, deviceMask),
        m_properties(desc.numVerts, desc.vertexSize) {
#if GE_DEBUG_MODE
      checkValidDesc(desc);
#endif
    }

    VertexBuffer::~VertexBuffer() {
      if (m_buffer && !m_sharedBuffer) {
        m_bufferDeleter(m_buffer);
      }

      GE_INC_RENDER_STAT_CAT(ResDestroyed, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
    }

    void
    VertexBuffer::initialize() {
      GE_INC_RENDER_STAT_CAT(ResCreated, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
      CoreObject::initialize();
    }

    void*
    VertexBuffer::map(uint32 offset,
                      uint32 length,
                      GPU_LOCK_OPTIONS::E options,
                      uint32 deviceIdx,
                      uint32 queueIdx) {
#if GE_PROFILING_ENABLED
      if (GPU_LOCK_OPTIONS::kREAD_ONLY == options ||
          GPU_LOCK_OPTIONS::kREAD_WRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
      }

      if (GPU_LOCK_OPTIONS::kREAD_WRITE == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
      }
#endif
      return m_buffer->lock(offset, length, options, deviceIdx, queueIdx);
    }

    void
    VertexBuffer::unmap() {
      m_buffer->unlock();
    }

    void
    VertexBuffer::readData(uint32 offset,
                           uint32 length,
                           void* dest,
                           uint32 deviceIdx,
                           uint32 queueIdx) {
      m_buffer->readData(offset, length, dest, deviceIdx, queueIdx);
      GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
    }

    void
    VertexBuffer::writeData(uint32 offset,
                            uint32 length,
                            const void* source,
                            BUFFER_WRITE_TYPE::E writeFlags,
                            uint32 queueIdx) {
      m_buffer->writeData(offset, length, source, writeFlags, queueIdx);
      GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kVertexBuffer);
    }

    void
    VertexBuffer::copyData(HardwareBuffer& srcBuffer,
                           uint32 srcOffset,
                           uint32 dstOffset,
                           uint32 length,
                           bool discardWholeBuffer,
                           const SPtr<CommandBuffer>& commandBuffer) {
      auto& srcVertexBuffer = static_cast<VertexBuffer&>(srcBuffer);
      m_buffer->copyData(*srcVertexBuffer.m_buffer,
                         srcOffset,
                         dstOffset,
                         length,
                         discardWholeBuffer,
                         commandBuffer);
    }

    SPtr<GPUBuffer>
    VertexBuffer::getLoadStore(GPU_BUFFER_TYPE::E type,
                               GPU_BUFFER_FORMAT::E format,
                               uint32 elementSize) {
      if ((m_usage & GPU_BUFFER_USAGE::kLOADSTORE) != GPU_BUFFER_USAGE::kLOADSTORE) {
        return nullptr;
      }

      for (const auto& entry : m_loadStoreViews) {
        const GPUBufferProperties& props = entry->getProperties();
        if (props.getType() == type) {
          if (GPU_BUFFER_TYPE::kSTANDARD == type &&
              props.getFormat() == format) {
            return entry;
          }

          if (GPU_BUFFER_TYPE::kSTRUCTURED == type &&
              props.getElementSize() == elementSize) {
            return entry;
          }
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

    SPtr<VertexBuffer>
    VertexBuffer::create(const VERTEX_BUFFER_DESC& desc,
                         GPU_DEVICE_FLAGS::E deviceMask) {
      return HardwareBufferManager::instance().createVertexBuffer(desc,
                                                                  deviceMask);
    }
  }
}
