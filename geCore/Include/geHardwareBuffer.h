/*****************************************************************************/
/**
 * @file    geHardwareBuffer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/19
 * @brief   Abstract class defining common features of hardware buffers.
 *
 * Abstract class defining common features of hardware buffers.
 * Hardware buffers usually represent areas of memory the GPU or the driver can
 * access directly.
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

namespace geEngineSDK {
  using std::min;

  /**
   * @brief Abstract class defining common features of hardware buffers.
   *        Hardware buffers usually represent areas of memory the GPU or the
   *        driver can access directly.
   * @note  Core thread only.
   * @note  Be aware that reading from non-system memory hardware buffers is
   *        usually slow and should be avoided.
   */
  class GE_CORE_EXPORT HardwareBuffer
  {
   public:
    virtual ~HardwareBuffer() = default;

    /**
     * @brief Locks a portion of the buffer and returns pointer to the locked
     *        area. You must call unlock() when done.
     * @param[in] offset    Offset in bytes from which to lock the buffer.
     * @param[in] length    Length of the area you want to lock, in bytes.
     * @param[in] options   Signifies what you want to do with the returned
     *            pointer. Caller must ensure not to do anything he hasn't
     *            requested (for example don't try to read from the buffer
     *            unless you requested it here).
     * @param[in] deviceIdx Index of the device whose memory to map. If the
     *            buffer doesn't exist on this device, the method returns null.
     * @param[in] queueIdx  Device queue to perform any read/write operations
     *            on. See @ref queuesDoc.
     */
    virtual void*
    lock(uint32 offset,
         uint32 length,
         GPU_LOCK_OPTIONS::E options,
         uint32 deviceIdx = 0,
         uint32 queueIdx = 0) {
      GE_ASSERT(!isLocked() && "Can't lock buffer, it is already locked!");
      void* ret = map(offset, length, options, deviceIdx, queueIdx);
      m_isLocked = true;

      return ret;
    }

    /**
     * @brief Locks the entire buffer and returns pointer to the locked area.
     *        You must call unlock() when done.
     * @param[in] options   Signifies what you want to do with the returned
     *            pointer. Caller must ensure not to do anything he hasn't
     *            requested (for example don't try to read from the buffer
     *            unless you requested it here).
     * @param[in] deviceIdx Index of the device whose memory to map. If the
     *            buffer doesn't exist on this device, the method returns null.
     * @param[in] queueIdx  Device queue to perform any read / write operations
     *            on. See @ref queuesDoc.
     */
    void*
    lock(GPU_LOCK_OPTIONS::E options,
         uint32 deviceIdx = 0,
         uint32 queueIdx = 0) {
      return this->lock(0, m_size, options, deviceIdx, queueIdx);
    }

    /**
     * @brief Releases the lock on this buffer.
     */
    virtual void
    unlock() {
      GE_ASSERT(isLocked() && "Can't unlock this buffer, it is not locked!");
      unmap();
      m_isLocked = false;
    }

    /**
     * @brief Reads data from a portion of the buffer and copies it to the
     *        destination buffer. Caller must ensure destination buffer is
     *        large enough.
     * @param[in] offset    Offset in bytes from which to copy the data.
     * @param[in] length    Length of the area you want to copy, in bytes.
     * @param[in] dest      Destination buffer large enough to store the read
     *            data. Data is written from the start of the buffer
     *            (@p offset is only applied to the source).
     * @param[in] deviceIdx Index of the device whose memory to read. If the
     *            buffer doesn't exist on this device, no data will be read.
     * @param[in] queueIdx  Device queue to perform the read operation on.
     *            See @ref queuesDoc.
     */
    virtual void
    readData(uint32 offset,
             uint32 length,
             void* dest,
             uint32 deviceIdx = 0,
             uint32 queueIdx = 0) = 0;

    /**
     * @brief Writes data into a portion of the buffer from the source memory.
     * @param[in] offset      Offset in bytes from which to copy the data.
     * @param[in] length      Length of the area you want to copy, in bytes.
     * @param[in] source      Source buffer containing the data to write.
     *            Data is read from the start of the buffer
     *            (@p offset is only applied to the destination).
     * @param[in] writeFlags  Optional write flags that may affect performance.
     * @param[in] queueIdx    Device queue to perform the write operation on.
     *            See @ref queuesDoc.
     */
    virtual void
    writeData(uint32 offset,
              uint32 length,
              const void* source,
              BUFFER_WRITE_TYPE::E writeFlags = BUFFER_WRITE_TYPE::kNORMAL,
              uint32 queueIdx = 0) = 0;

    /**
     * @brief Copies data from a specific portion of the source buffer into a
     *        specific portion of this buffer.
     * @param[in] srcBuffer     Buffer to copy from.
     * @param[in] srcOffset     Offset into the source buffer to start copying
     *                          from, in bytes.
     * @param[in] dstOffset     Offset into this buffer to start copying to,
     *                          in bytes.
     * @param[in] length        Size of the data to copy, in bytes.
     * @param[in] discardWholeBuffer  Specify true if the data in the current
     *            buffer can be entirely discarded.
     *            This may improve performance.
     * @param[in] commandBuffer Command buffer to queue the copy operation on.
     *            If null, main command buffer is used.
     */
    virtual void
    copyData(HardwareBuffer& srcBuffer,
             uint32 srcOffset,
             uint32 dstOffset,
             uint32 length,
             bool discardWholeBuffer = false,
             const SPtr<geCoreThread::CommandBuffer>& commandBuffer = nullptr) = 0;

    /**
     * @brief Copy data from the provided buffer into this buffer.
     *        If buffers are not the same size, smaller size will be used.
     * @param[in] srcBuffer     Hardware buffer to copy from.
     * @param[in] commandBuffer Command buffer to queue the copy operation on.
     *                          If null, main command buffer is used.
     */
    virtual void
    copyData(HardwareBuffer& srcBuffer,
             const SPtr<geCoreThread::CommandBuffer>& commandBuffer = nullptr) {
      uint32 sz = min(getSize(), srcBuffer.getSize());
      copyData(srcBuffer, 0, 0, sz, true, commandBuffer);
    }

    /**
     * @brief Returns the size of this buffer in bytes.
     */
    uint32
    getSize() const {
      return m_size;
    }

    /**
     * @brief Returns whether or not this buffer is currently locked.
     */
    bool
    isLocked() const {
      return m_isLocked;
    }

    /**
     * @brief Returns a mask signifying on which devices has been this buffer
     *        created on.
     */
    GPU_DEVICE_FLAGS::E
    getDeviceMask() const {
      return m_deviceMask;
    }

   protected:
    friend class HardwareBufferManager;

    /**
     * @brief Constructs a new buffer.
     * @param[in] size  Size of the buffer, in bytes.
     */
    HardwareBuffer(uint32 size, GPU_BUFFER_USAGE::E usage, GPU_DEVICE_FLAGS::E deviceMask)
      : m_size(size),
        m_usage(usage),
        m_deviceMask(deviceMask)
    {}

    /**
     * @copydoc lock
     */
    virtual void*
    map(uint32 offset,
        uint32 length,
        GPU_LOCK_OPTIONS::E options,
        uint32 deviceIdx,
        uint32 queueIdx) {
      GE_UNREFERENCED_PARAMETER(offset);
      GE_UNREFERENCED_PARAMETER(length);
      GE_UNREFERENCED_PARAMETER(options);
      GE_UNREFERENCED_PARAMETER(deviceIdx);
      GE_UNREFERENCED_PARAMETER(queueIdx);
      return nullptr;
    }

    /**
     * @copydoc unlock
     */
    virtual void
    unmap() {}

   protected:
    uint32 m_size;
    GPU_BUFFER_USAGE::E m_usage;
    GPU_DEVICE_FLAGS::E m_deviceMask;
    bool m_isLocked = false;
  };
}
