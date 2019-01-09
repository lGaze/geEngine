/*****************************************************************************/
/**
 * @file    geIndexBuffer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Hardware buffer that hold indices of vertices in a vertex buffer.
 *
 * Hardware buffer that hold indices of vertices in a vertex buffer.
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
   * @brief Descriptor structure used for initialization of an IndexBuffer.
   */
  struct INDEX_BUFFER_DESC
  {
    /**
     * @brief Index type, determines the size of a single index.
     */
    IndexType indexType;

    /**
     * @brief Number of indices can buffer can hold.
     */
    uint32 numIndices;

    /**
     * @brief Usage that tells the hardware how will be buffer be used.
     */
    GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kSTATIC;
  };

  /**
   * @brief Contains information about an index buffer.
   */
  class GE_CORE_EXPORT IndexBufferProperties
  {
   public:
    IndexBufferProperties(IndexType idxType, uint32 numIndexes);

    /**
     * @brief Returns the type of indices stored.
     */
    IndexType
    getType() const {
      return m_indexType;
    }

    /**
     * @brief Returns the number of indices this buffer can hold.
     */
    uint32
    getNumIndices() const {
      return m_numIndices;
    }

    /**
     * @brief Returns the size of a single index in bytes.
     */
    uint32
    getIndexSize() const {
      return m_indexSize;
    }

   protected:
    friend class IndexBuffer;
    friend class geCoreThread::IndexBuffer;

    IndexType m_indexType;
    uint32 m_numIndices;
    uint32 m_indexSize;
  };

  /**
   * @brief Hardware buffer that hold indices that reference vertices in a
   *        vertex buffer.
   */
  class GE_CORE_EXPORT IndexBuffer : public CoreObject
  {
   public:
    virtual ~IndexBuffer() = default;

    /**
     * @brief Returns information about the index buffer.
     */
    const IndexBufferProperties&
    getProperties() const {
      return m_properties;
    }

    /**
     * @brief Retrieves a core implementation of an index buffer usable only
     *        from the core thread.
     * @note  Core thread only.
     */
    SPtr<geCoreThread::IndexBuffer>
    getCore() const;

    /**
     * @copydoc HardwareBufferManager::createIndexBuffer
     */
    static SPtr<IndexBuffer>
    create(const INDEX_BUFFER_DESC& desc);

   protected:
    friend class HardwareBufferManager;

    IndexBuffer(const INDEX_BUFFER_DESC& desc);

    /**
     *@copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;

    IndexBufferProperties m_properties;
    GPU_BUFFER_USAGE::E m_usage;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread specific implementation of an bs::IndexBuffer.
     */
    class GE_CORE_EXPORT IndexBuffer
      : public CoreObject, public HardwareBuffer
    {
     public:
      IndexBuffer(const INDEX_BUFFER_DESC& desc,
        GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);
      virtual ~IndexBuffer();

      /**
       * @brief Returns information about the index buffer.
       */
      const IndexBufferProperties&
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
       *        GPU_BUFFER_USAGE::kLOADSTORE usage flag.
       * @param[in] type        Type of buffer to view the contents as.
       *            Only supported values are GPU_BUFFER_TYPE::kSTANDARD and
       *            GPU_BUFFER_TYPE::kSTRUCTURED.
       * @param[in] format      Format of the data in the buffer. Size of the
       *            underlying buffer must be divisible by the size of an
       *            individual element of this format.
       *            Must be BUFFER_FORMAT::kUNKNOWN if buffer type is
       *            GPU_BUFFER_TYPE::kSTRUCTURED.
       * @param[in] elementSize Size of the individual element in the buffer.
       *            Size of the underlying buffer must be divisible by this
       *            size. Must be 0 if buffer type is
       *            GPU_BUFFER_TYPE::kSTANDARD (element size gets deduced from
       *            format).
       * @return    Buffer usable for load store operations or null if the
       *            operation fails. Failure can happen if the buffer hasn't
       *            been created with GPU_BUFFER_USAGE::kLOADSTORE usage or if
       *            the elementSize doesn't divide the current buffer size.
       */
      SPtr<GPUBuffer>
      getLoadStore(GPU_BUFFER_TYPE::E type,
                   GPU_BUFFER_FORMAT::E format,
                   uint32 elementSize = 0);

      /**
       * @copydoc HardwareBufferManager::createIndexBuffer
       */
      static SPtr<IndexBuffer>
      create(const INDEX_BUFFER_DESC& desc,
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

      IndexBufferProperties m_properties;

      HardwareBuffer* m_buffer = nullptr;
      SPtr<HardwareBuffer> m_sharedBuffer;
      Vector<SPtr<GPUBuffer>> m_loadStoreViews;

      using Deleter = void(*)(HardwareBuffer*);
      Deleter m_bufferDeleter = nullptr;
    };
  }
}
