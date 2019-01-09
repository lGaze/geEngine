/*****************************************************************************/
/**
 * @file    geVertexBuffer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Specialization of a hardware buffer used for holding vertex data.
 *
 * Specialization of a hardware buffer used for holding vertex data.
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
#include "gePrerequisitesCore.h"
#include "geHardwareBuffer.h"
#include "geCoreObject.h"

namespace geEngineSDK {
  /**
   * @brief Descriptor structure used for initialization of a VertexBuffer.
   */
  struct VERTEX_BUFFER_DESC
  {
    /**
     * Size of a single vertex in the buffer, in bytes.
     */
    uint32 vertexSize;

    /**
     * Number of vertices the buffer can hold.
     */
    uint32 numVerts;

    /**
     * Usage that tells the hardware how will the buffer be used.
     */
    GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kSTATIC;

    /**
     * If true the buffer will be usable for streaming out data from the GPU.
     */
    bool streamOut = false;
  };

  /**
   * @brief Contains information about a vertex buffer.
   */
  class GE_CORE_EXPORT VertexBufferProperties
  {
   public:
    VertexBufferProperties(uint32 numVertices, uint32 vertexSize);

    /**
     * @brief Gets the size in bytes of a single vertex in this buffer.
     */
    uint32
    getVertexSize() const {
      return m_vertexSize;
    }

    /**
     * @brief Get the number of vertices in this buffer.
     */
    uint32
    getNumVertices() const {
      return m_numVertices;
    }

   protected:
    friend class VertexBuffer;
    friend class geCoreThread::VertexBuffer;

    uint32 m_numVertices;
    uint32 m_vertexSize;
  };

  /**
   * @brief Specialization of a hardware buffer used for holding vertex data.
   */
  class GE_CORE_EXPORT VertexBuffer : public CoreObject
  {
   public:
    virtual ~VertexBuffer() = default;

    /**
     * @brief Retrieves a core implementation of a vertex buffer usable only
     *        from the core thread.
     * @note  Core thread only.
     */
    SPtr<geCoreThread::VertexBuffer>
    getCore() const;

    /**
     * @copydoc HardwareBufferManager::createVertexBuffer
     */
    static SPtr<VertexBuffer>
    create(const VERTEX_BUFFER_DESC& desc);

    static constexpr int32 MAX_SEMANTIC_IDX = 8;

   protected:
    friend class HardwareBufferManager;

    VertexBuffer(const VERTEX_BUFFER_DESC& desc);

    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;

    VertexBufferProperties m_properties;
    GPU_BUFFER_USAGE::E m_usage;
    bool m_streamOut;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread specific implementation of a
     *        geEngineSDK::VertexBuffer.
     */
    class GE_CORE_EXPORT VertexBuffer
      : public CoreObject, public HardwareBuffer {
     public:
      VertexBuffer(const VERTEX_BUFFER_DESC& desc,
                   GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      virtual ~VertexBuffer();

      /**
       * @brief Returns information about the vertex buffer.
       */
      const VertexBufferProperties&
      getProperties() const {
        return m_properties;
      }

      /**
       * @copydoc HardwareBuffer::readData
       */
      void
      readData(uint32 offset,
               uint32 length,
               void* dest,
               uint32 deviceIdx = 0,
               uint32 queueIdx = 0) override;

      /**
       * @copydoc HardwareBuffer::writeData
       */
      void
      writeData(uint32 offset,
                uint32 length,
                const void* source,
                BUFFER_WRITE_TYPE::E writeFlags = BUFFER_WRITE_TYPE::kNORMAL,
                uint32 queueIdx = 0) override;

      /**
       * @copydoc HardwareBuffer::copyData
       */
      void
      copyData(HardwareBuffer& srcBuffer,
               uint32 srcOffset,
               uint32 dstOffset,
               uint32 length,
               bool discardWholeBuffer = false,
               const SPtr<CommandBuffer>& commandBuffer = nullptr) override;

      /**
       * @brief Returns a view of this buffer that can be used for load-store
       *        operations. Buffer must have been created with the
       *        GPU_BUFFER_UASGE::kLOADSTORE usage flag.
       * @param[in] type        Type of buffer to view the contents as.
       *            Only supported values are GPU_BUFFER_TYPE::kSTANDARD and
       *            GPU_BUFFER_TYPE::kSTRUCTURED.
       * @param[in] format      Format of the data in the buffer.
       *            Size of the underlying buffer must be divisible by the
       *            size of an individual element of this format. Must be
       *            BUFFER_FORMAT::kUNKNOWN if buffer type is
       *            GPU_BUFFER_TYPE::kSTRUCTURED.
       * @param[in] elementSize Size of the individual element in the buffer.
       *            Size of the underlying buffer must be divisible by this
       *            size. Must be 0 if buffer type is GPU_BUFFER_TYPE::kSTANDARD.
       * @return  Buffer usable for load store operations or null if the
       *          operation fails. Failure can happen if the buffer hasn't been
       *          created with GPU_BUFFER_USAGE::kLOADSTORE usage or if the
       *          element size doesn't divide the current buffer size.
       */
      SPtr<GPUBuffer>
      getLoadStore(GPU_BUFFER_TYPE::E type,
                   GPU_BUFFER_FORMAT::E format,
                   uint32 elementSize = 0);

      /**
       * @copydoc HardwareBufferManager::createVertexBuffer
       */
      static SPtr<VertexBuffer>
      create(const VERTEX_BUFFER_DESC& desc,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      friend class HardwareBufferManager;

      /**
       * @copydoc HardwareBuffer::map
       */
      void*
      map(uint32 offset,
          uint32 length,
          GPU_LOCK_OPTIONS::E options,
          uint32 deviceIdx,
          uint32 queueIdx) override;

      /**
       * @copydoc HardwareBuffer::unmap
       */
      void
      unmap() override;

      /**
       * @copydoc CoreObject::initialize
       */
      void
      initialize() override;

      VertexBufferProperties m_properties;

      HardwareBuffer* m_buffer = nullptr;
      SPtr<HardwareBuffer> m_sharedBuffer;
      Vector<SPtr<GPUBuffer>> m_loadStoreViews;

      using Deleter = void(*)(HardwareBuffer*);
      Deleter m_bufferDeleter = nullptr;
    };
  }
}
