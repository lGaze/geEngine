/*****************************************************************************/
/**
 * @file    geGPUParamBlockBuffer.h
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
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include "geCoreObject.h"

namespace geEngineSDK {
  class HardwareBuffer;

  /**
   * @brief Represents a GPU parameter block buffer. Parameter block buffers
   *        are bound to GPU programs which then fetch parameters from those
   *        buffers.
   *        Writing or reading from this buffer will translate directly to API
   *        calls that update the GPU.
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT GPUParamBlockBuffer : public CoreObject
  {
   public:
    GPUParamBlockBuffer(uint32 size, GPU_BUFFER_USAGE::E usage);
    virtual ~GPUParamBlockBuffer();

    /**
     * @brief Write some data to the specified offset in the buffer.
     * @note  All values are in bytes. Actual hardware buffer update is
     *        delayed until rendering.
     */
    void
    write(uint32 offset, const void* data, uint32 size);

    /**
     * @brief Read some data from the specified offset in the buffer.
     * @note  All values are in bytes. This reads from the cached CPU buffer
     *        and not from the GPU.
     */
    void
    read(uint32 offset, void* data, uint32 size);

    /**
     * @brief Clear specified section of the buffer to zero.
     * @note  All values are in bytes. Actual hardware buffer update is
     *        delayed until rendering.
     */
    void
    zeroOut(uint32 offset, uint32 size);

    /**
     * @brief Returns internal cached data of the buffer.
     */
    const uint8*
    getCachedData() const {
      return m_cachedData;
    }

    /**
     * @brief Returns the size of the buffer in bytes.
     */
    uint32
    getSize() const {
      return m_size;
    }

    /**
     * @brief Retrieves a core implementation of a GPU param block buffer
     *        usable only from the core thread.
     */
    SPtr<geCoreThread::GPUParamBlockBuffer>
    getCore() const;

    /**
     * @copydoc HardwareBufferManager::createGPUParamBlockBuffer
     */
    static SPtr<GPUParamBlockBuffer>
    create(uint32 size, GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kDYNAMIC);

   protected:
    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;

    /**
     * @copydoc CoreObject::syncToCore
     */
    CoreSyncData
    syncToCore(FrameAlloc* allocator) override;

    GPU_BUFFER_USAGE::E m_usage;
    uint32 m_size;
    uint8* m_cachedData;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread version of a geEngineSDK::GPUParamBlockBuffer.
     * @note  Core thread only.
     */
    class GE_CORE_EXPORT GPUParamBlockBuffer : public CoreObject
    {
     public:
      GPUParamBlockBuffer(uint32 size,
                          GPU_BUFFER_USAGE::E usage,
                          GPU_DEVICE_FLAGS::E deviceMask);
      virtual ~GPUParamBlockBuffer();

      /**
       * @brief Writes all of the specified data to the buffer.
       *        Data size must be the same size as the buffer.
       * @param[in] data      Data to write. Must match the size of the buffer.
       * @param[in] queueIdx  Device queue to perform the write operation on.
       *                      See @ref queuesDoc.
       */
      void
      writeToGPU(const uint8* data, uint32 queueIdx = 0);

      /**
       * @brief Flushes any cached data into the actual GPU buffer.
       * @param[in] queueIdx  Device queue to perform the write operation on.
       *                      See @ref queuesDoc.
       */
      void
      flushToGPU(uint32 queueIdx = 0);

      /**
       * @brief Write some data to the specified offset in the buffer.
       * @note  All values are in bytes. Actual hardware buffer update is
       *        delayed until rendering or until flushToGPU() is called.
       */
      void
      write(uint32 offset, const void* data, uint32 size);

      /**
       * @brief Read some data from the specified offset in the buffer.
       * @note  All values are in bytes. This reads from the cached CPU buffer
       *        and not directly from the GPU.
       */
      void
      read(uint32 offset, void* data, uint32 size);

      /**
       * @brief Clear specified section of the buffer to zero.
       * @note  All values are in bytes. Actual hardware buffer update is
       *        delayed until rendering or until flushToGPU() is called.
       */
      void
      zeroOut(uint32 offset, uint32 size);

      /**
       * @brief Returns the size of the buffer in bytes.
       */
      uint32
      getSize() const {
        return m_size;
      }

      /**
       * @copydoc HardwareBufferManager::createGPUParamBlockBuffer
       */
      static SPtr<GPUParamBlockBuffer>
      create(uint32 size,
             GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kDYNAMIC,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      friend class HardwareBufferManager;

      /**
       * @copydoc CoreObject::syncToCore
       */
      void
      syncToCore(const CoreSyncData& data)  override;

      /**
       * @copydoc CoreObject::initialize
       */
      void
      initialize() override;

      HardwareBuffer* m_buffer;

      GPU_BUFFER_USAGE::E m_usage;
      uint32 m_size;

      uint8* m_cachedData;
      bool m_gpuBufferDirty;
    };
  }
}
