/*****************************************************************************/
/**
 * @file    geGPUBuffer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Handles a generic GPU buffer.
 *
 * Handles a generic GPU buffer that you may use for storing any kind of data.
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
#include "geCoreObject.h"
#include "geHardwareBuffer.h"

namespace geEngineSDK {
  /**
   * @brief Descriptor structure used for initialization of a GPUBuffer.
   */
  struct GPU_BUFFER_DESC
  {
    /**
     * @brief Number of elements in the buffer.
     */
    uint32 elementCount = 0;

    /**
     * @brief Size of each individual element in the buffer, in bytes. Only
     *        needed if using non-standard buffer. If using standard buffers
     *        element size is calculated from format and this must be zero.
     */
    uint32 elementSize = 0;

    /**
     * @brief Type of the buffer. Determines how is buffer seen by the GPU
     *        program and in what ways can it be used.
     */
    GPU_BUFFER_TYPE::E type = GPU_BUFFER_TYPE::kSTANDARD;

    /**
     * @brief Format if the data in the buffer. Only relevant for standard
     *        buffers, must be BF_UNKNOWN otherwise.
     */
    GPU_BUFFER_FORMAT::E format = GPU_BUFFER_FORMAT::k32X4F;

    /**
     * @brief Usage that tells the hardware how will be buffer be used.
     */
    GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kSTATIC;
  };

  /**
   * @brief Information about a GPUBuffer. Allows core and non-core versions of
   *        GPUBuffer to share the same structure for properties.
   */
  class GE_CORE_EXPORT GPUBufferProperties
  {
   public:
    GPUBufferProperties(const GPU_BUFFER_DESC& desc);

    /**
     * @brief Returns the type of the GPU buffer. Type determines which kind of
     *        views (if any) can be created for the buffer, and how is data
     *        read or modified in it.
     */
    GPU_BUFFER_TYPE::E
    getType() const {
      return m_desc.type;
    }

    /**
     * @brief Returns format used by the buffer. Only relevant for standard
     *        buffers.
     */
    GPU_BUFFER_FORMAT::E
    getFormat() const {
      return m_desc.format;
    }

    /**
     * @brief Returns buffer usage which determines how are planning on
     *        updating the buffer contents.
     */
    GPU_BUFFER_USAGE::E
    getUsage() const {
      return m_desc.usage;
    }

    /**
     * @brief Returns number of elements in the buffer.
     */
    uint32
    getElementCount() const {
      return m_desc.elementCount;
    }

    /**
     * @brief Returns size of a single element in the buffer in bytes.
     */
    uint32
    getElementSize() const {
      return m_desc.elementSize;
    }

   protected:
    friend class GPUBuffer;

    GPU_BUFFER_DESC m_desc;
  };

  /**
   * @brief Handles a generic GPU buffer that you may use for storing any kind
   *        of data you wish to be accessible to the GPU.
   * These buffers may be bounds to GPU program binding slots and accessed from
   * a GPU program, or may be used by fixed pipeline in some way.
   *
   * Buffer types:
   *  - Raw buffers containing a block of bytes that are up to the GPU program
   *    to interpret.
   *	- Structured buffer containing an array of structures compliant to a
   *    certain layout. Similar to raw buffer but easier to interpret the data.
   *	- Random read/write buffers that allow you to write to random parts of
   *    the buffer from within the GPU program, and then read it later.
   *    These can only be bound to pixel and compute stages.
   *	- Append/Consume buffers also allow you to write to them, but in a
   *    stack-like fashion, usually where one set of programs produces data
   *    while other set consumes it from the same buffer.
   *    Append/Consume buffers are structured by default.
   *
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT GPUBuffer : public CoreObject
  {
   public:
    virtual ~GPUBuffer() = default;

    /**
     * @brief Returns properties describing the buffer.
     */
    const GPUBufferProperties&
    getProperties() const {
      return m_properties;
    }

    /**
     * @brief Retrieves a core implementation of a GPU buffer usable only from
     *        the core thread.
     */
    SPtr<geCoreThread::GPUBuffer>
    getCore() const;

    /**
     * @brief Returns the size of a single element in the buffer, of the
     *        provided format, in bytes.
     */
    static uint32
    getFormatSize(GPU_BUFFER_FORMAT::E format);

    /**
     * @copydoc HardwareBufferManager::createGPUBuffer
     */
    static SPtr<GPUBuffer>
    create(const GPU_BUFFER_DESC& desc);

   protected:
    friend class HardwareBufferManager;

    GPUBuffer(const GPU_BUFFER_DESC& desc);

    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;

    GPUBufferProperties m_properties;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread version of a bs::GPUBuffer.
     * @note  Core thread only.
     */
    class GE_CORE_EXPORT GPUBuffer : public CoreObject, public HardwareBuffer
    {
     public:
      virtual ~GPUBuffer();

      /**
       * @brief Returns properties describing the buffer.
       */
      const GPUBufferProperties&
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
       * @brief Returns a view of this buffer with specified format/type.
       * @param[in] type          Type of buffer to view the contents as. Only
       *            supported values are GBT_STANDARD and GBT_STRUCTURED.
       * @param[in] format        Format of the data in the buffer. Size of the
       *            underlying buffer must be divisible by the	size of an
       *            individual element of this format. Must be BF_UNKNOWN if
       *            buffer type is GBT_STRUCTURED.
       * @param[in] elementSize   Size of the individual element in the buffer.
       *            Size of the underlying buffer must be divisible by this
       *            size. Must be 0 if buffer type is GBT_STANDARD (element
       *            size gets deduced from format).
       * @return    New view of the buffer, using the provided format and type.
       */
      SPtr<GPUBuffer>
      getView(GPU_BUFFER_TYPE::E type,
              GPU_BUFFER_FORMAT::E format,
              uint32 elementSize = 0);

      /**
       * @copydoc HardwareBufferManager::createGPUBuffer
       */
      static SPtr<GPUBuffer>
      create(const GPU_BUFFER_DESC& desc,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @brief Creates a view of an existing hardware buffer. No internal
       *        buffer will be allocated and the provided buffer will be used
       *        for all internal operations instead. Information provided in
       *        @p desc (such as element size and count) must match the
       *        provided @p underlyingBuffer.
       */
      static SPtr<GPUBuffer>
      create(const GPU_BUFFER_DESC& desc, SPtr<HardwareBuffer> underlyingBuffer);

     protected:
      friend class HardwareBufferManager;

      GPUBuffer(const GPU_BUFFER_DESC& desc, GPU_DEVICE_FLAGS::E deviceMask);
      
      GPUBuffer(const GPU_BUFFER_DESC& desc, SPtr<HardwareBuffer> underlyingBuffer);

      /**
       * @copydoc HardwareBuffer::map
       */
      void*
      map(uint32 offset,
          uint32 length,
          GPU_LOCK_OPTIONS::E options,
          uint32 deviceIdx = 0,
          uint32 queueIdx = 0) override;

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

      GPUBufferProperties m_properties;

      HardwareBuffer* m_buffer = nullptr;
      SPtr<HardwareBuffer> m_sharedBuffer;
      bool m_isExternalBuffer = false;

      using Deleter = void(*)(HardwareBuffer*);
      Deleter m_bufferDeleter = nullptr;
    };
  }
}
