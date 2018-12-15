/*****************************************************************************/
/**
 * @file    geFrameAlloc.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2016/03/06
 * @brief   Frame allocator
 *
 * Performs very fast allocations but can only free all of its memory at once.
 * Perfect for allocations that last just a single frame.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesUtil.h"
#include "geFrameAlloc.h"
#include "geException.h"

namespace geEngineSDK {
  uint8*
  FrameAlloc::MemBlock::alloc(SIZE_T amount) {
    uint8* freePtr = &m_data[m_freePtr];
    m_freePtr += amount;
    return freePtr;
  }

  void
  FrameAlloc::MemBlock::clear() {
    m_freePtr = 0;
  }

#if GE_DEBUG_MODE
  FrameAlloc::FrameAlloc(SIZE_T blockSize)
    : m_blockSize(blockSize),
      m_freeBlock(nullptr),
      m_nextBlockIdx(0),
      m_totalAllocBytes(0),
      m_lastFrame(nullptr)
  {}
#else
  FrameAlloc::FrameAlloc(SIZE_T blockSize)
    : m_blockSize(blockSize),
      m_freeBlock(nullptr),
      m_nextBlockIdx(0),
      m_totalAllocBytes(0),
      m_lastFrame(nullptr)
  {}
#endif

  FrameAlloc::~FrameAlloc() {
    for (auto& block : m_blocks) {
      deallocBlock(block);
    }
  }

  uint8*
  FrameAlloc::alloc(SIZE_T amount) {
#if GE_DEBUG_MODE
    amount += sizeof(SIZE_T);
#endif
    SIZE_T freeMem = 0;
    if (nullptr != m_freeBlock) {
      freeMem = m_freeBlock->m_size - m_freeBlock->m_freePtr;
    }

    if (amount > freeMem) {
      allocBlock(amount);
    }

    uint8* data = m_freeBlock->alloc(amount);

#if GE_DEBUG_MODE
    m_totalAllocBytes += amount;

    SIZE_T* storedSize = reinterpret_cast<SIZE_T*>(data);
    *storedSize = amount;

    return data + sizeof(SIZE_T);
#else
    return data;
#endif
  }

  uint8*
  FrameAlloc::allocAligned(SIZE_T amount, SIZE_T alignment) {
#if GE_DEBUG_MODE
    amount += sizeof(SIZE_T);
#endif
    SIZE_T freeMem = 0;
    SIZE_T freePtr = 0;
    if (nullptr != m_freeBlock) {
      freeMem = m_freeBlock->m_size - m_freeBlock->m_freePtr;
#if GE_DEBUG_MODE
      freePtr = m_freeBlock->m_freePtr + sizeof(SIZE_T);
#else
      freePtr = m_freeBlock->m_freePtr;
#endif
    }

    SIZE_T alignOffset = (alignment - (freePtr & (alignment - 1))) & (alignment - 1);
    if ((amount + alignOffset) > freeMem) {
      //New blocks are allocated on a 16 byte boundary, ensure enough space is
      //allocated taking into account the requested alignment

#if GE_DEBUG_MODE
      alignOffset = (alignment - (sizeof(SIZE_T) & (alignment - 1))) & (alignment - 1);
#else
      if (16 < alignment) {
        alignOffset = alignment - 16;
      }
      else {
        alignOffset = 0;
      }
#endif
      allocBlock(amount + alignOffset);
    }

    amount += alignOffset;
    uint8* data = m_freeBlock->alloc(amount);

#if GE_DEBUG_MODE
    m_totalAllocBytes += amount;

    auto storedSize = reinterpret_cast<SIZE_T*>(data + alignOffset);
    *storedSize = amount;

    return data + sizeof(SIZE_T) + alignOffset;
#else
    return data + alignOffset;
#endif
  }

  void
  FrameAlloc::free(uint8* data) {
    //Dealloc is only used for debug and can be removed if needed.
    //All the actual deallocation happens in clear()
#if GE_DEBUG_MODE
    if (data) {
      data -= sizeof(SIZE_T);
      auto storedSize = reinterpret_cast<SIZE_T*>(data);
      m_totalAllocBytes -= *storedSize;
    }
#else
    GE_UNREFERENCED_PARAMETER(data);
#endif
  }

  void
  FrameAlloc::markFrame() {
    auto framePtr = reinterpret_cast<void**>(alloc(sizeof(void*)));
    *framePtr = m_lastFrame;
    m_lastFrame = framePtr;
  }

  void
  FrameAlloc::clear() {
    if (nullptr != m_lastFrame) {
      GE_ASSERT(m_blocks.size() > 0 && 0 < m_nextBlockIdx);

      free(reinterpret_cast<uint8*>(m_lastFrame));

      auto framePtr = reinterpret_cast<uint8*>(m_lastFrame);
      m_lastFrame = *reinterpret_cast<void**>(m_lastFrame);

#if GE_DEBUG_MODE
      framePtr -= sizeof(SIZE_T);
#endif

      uint32 startBlockIdx = m_nextBlockIdx - 1;
      uint32 numFreedBlocks = 0;
      for (int32 i = startBlockIdx; i >= 0; --i) {
        MemBlock* curBlock = m_blocks[i];
        uint8* blockEnd = curBlock->m_data + curBlock->m_size;
        if (framePtr >= curBlock->m_data && framePtr < blockEnd) {
          uint8* dataEnd = curBlock->m_data + curBlock->m_freePtr;
          auto sizeInBlock = static_cast<SIZE_T>(dataEnd - framePtr);
          GE_ASSERT(sizeInBlock <= curBlock->m_freePtr);

          curBlock->m_freePtr -= sizeInBlock;
          if (0 == curBlock->m_freePtr) {
            ++numFreedBlocks;

            //Reset block counter if we're gonna reallocate this one
            if (1 < numFreedBlocks) {
              m_nextBlockIdx = static_cast<uint32>(i);
            }
          }

          break;
        }
        else {
          curBlock->m_freePtr = 0;
          m_nextBlockIdx = static_cast<uint32>(i);
          ++numFreedBlocks;
        }
      }

      if (1 < numFreedBlocks) {
        SIZE_T totalBytes = 0;
        for (uint32 i = 0; i < numFreedBlocks; ++i) {
          MemBlock* curBlock = m_blocks[m_nextBlockIdx];
          totalBytes += curBlock->m_size;

          deallocBlock(curBlock);
          m_blocks.erase(m_blocks.begin() + m_nextBlockIdx);
        }

        uint32 oldNextBlockIdx = m_nextBlockIdx;
        allocBlock(totalBytes);

        /** Point to the first non-full block, or if none available then point
         the the block we just allocated */
        if (0 < oldNextBlockIdx) {
          m_freeBlock = m_blocks[oldNextBlockIdx - 1];
        }
      }
      else {
        m_freeBlock = m_blocks[m_nextBlockIdx - 1];
      }
    }
    else {
#if GE_DEBUG_MODE
      if (m_totalAllocBytes.load() > 0) {
        GE_EXCEPT(InvalidStateException,
                  "Not all frame allocated bytes were properly released.");
      }
#endif
      if (m_blocks.size() > 1) {
        //Merge all blocks into one
        SIZE_T totalBytes = 0;
        for (auto& block : m_blocks) {
          totalBytes += block->m_size;
          deallocBlock(block);
        }

        m_blocks.clear();
        m_nextBlockIdx = 0;

        allocBlock(totalBytes);
      }
      else if (!m_blocks.empty()) {
        m_blocks[0]->m_freePtr = 0;
      }
    }
  }

  FrameAlloc::MemBlock*
  FrameAlloc::allocBlock(SIZE_T wantedSize) {
    SIZE_T blockSize = m_blockSize;
    if (wantedSize > blockSize) {
      blockSize = wantedSize;
    }

    MemBlock* newBlock = nullptr;
    while (m_nextBlockIdx < m_blocks.size()) {
      MemBlock* curBlock = m_blocks[m_nextBlockIdx];
      if (blockSize <= curBlock->m_size) {
        newBlock = curBlock;
        ++m_nextBlockIdx;
        break;
      }
      else {
        //Found an empty block that doesn't fit our data, delete it
        deallocBlock(curBlock);
        m_blocks.erase(m_blocks.begin() + m_nextBlockIdx);
      }
    }

    if (nullptr == newBlock) {
      SIZE_T alignOffset = 16 - (sizeof(MemBlock) & (16 - 1));

      auto data = reinterpret_cast<uint8*>(ge_alloc_aligned16(blockSize +
                                                              sizeof(MemBlock) +
                                                              alignOffset));
      newBlock = new (data) MemBlock(blockSize);
      data += sizeof(MemBlock) + alignOffset;
      newBlock->m_data = data;

      m_blocks.push_back(newBlock);
      ++m_nextBlockIdx;
    }

    //If previous block had some empty space it is lost until next "clear"
    m_freeBlock = newBlock;

    return newBlock;
  }

  void
  FrameAlloc::deallocBlock(MemBlock* block) {
    block->~MemBlock();
    ge_free_aligned16(block);
  }

  void
  FrameAlloc::setOwnerThread(ThreadId /*thread*/) {}

  GE_THREADLOCAL FrameAlloc* _globalFrameAlloc = nullptr;

  FrameAlloc&
  g_frameAlloc() {
    if (nullptr == _globalFrameAlloc) {
      //Note: This will leak memory but since it should exist throughout the
      //entirety of runtime it should only leak on shutdown when the OS will
      //free it anyway.
      _globalFrameAlloc = new FrameAlloc();
    }

    return *_globalFrameAlloc;
  }

  uint8*
  ge_frame_alloc(SIZE_T numBytes) {
    return g_frameAlloc().alloc(numBytes);
  }

  uint8*
  ge_frame_alloc_aligned(SIZE_T count, SIZE_T align) {
    return g_frameAlloc().allocAligned(count, align);
  }

  void
  ge_frame_free(void* data) {
    g_frameAlloc().free(reinterpret_cast<uint8*>(data));
  }

  void
  ge_frame_free_aligned(void* data) {
    g_frameAlloc().free(reinterpret_cast<uint8*>(data));
  }

  void
  ge_frame_mark() {
    g_frameAlloc().markFrame();
  }

  void
  ge_frame_clear() {
    g_frameAlloc().clear();
  }
}
