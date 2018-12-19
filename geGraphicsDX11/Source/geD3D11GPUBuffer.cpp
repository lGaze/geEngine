/*****************************************************************************/
/**
 * @file    geD3D11GPUBuffer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/14
 * @brief   DirectX 11 implementation of a generic GPU buffer.
 *
 * DirectX 11 implementation of a generic GPU buffer.
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
#include "geD3D11GPUBuffer.h"
#include "geD3D11GPUBufferView.h"
#include "geD3D11RenderAPI.h"
#include "geD3D11HardwareBuffer.h"
#include "geD3D11Device.h"
#include "geD3D11Mappings.h"

#include <gePoolAlloc.h>
#include <geRenderStats.h>
#include <geException.h>

namespace geEngineSDK {
  using std::move;
  using std::static_pointer_cast;

  namespace geCoreThread {
    static void
    deleteBuffer(HardwareBuffer* buffer) {
      ge_pool_delete(static_cast<D3D11HardwareBuffer*>(buffer));
    }

    D3D11GPUBuffer::D3D11GPUBuffer(const GPU_BUFFER_DESC& desc,
                                   GPU_DEVICE_FLAGS::E deviceMask)
      : GPUBuffer(desc, deviceMask) {
      GE_ASSERT((GPU_DEVICE_FLAGS::kDEFAULT == deviceMask ||
                 GPU_DEVICE_FLAGS::kPRIMARY == deviceMask) &&
                "Multiple GPUs not supported natively on DirectX 11.");
    }

    D3D11GPUBuffer::D3D11GPUBuffer(const GPU_BUFFER_DESC& desc,
                                   SPtr<HardwareBuffer> underlyingBuffer)
      : GPUBuffer(desc, move(underlyingBuffer))
    {}

    D3D11GPUBuffer::~D3D11GPUBuffer() {
      clearBufferViews();
    }

    void
    D3D11GPUBuffer::initialize() {
      const GPUBufferProperties& props = getProperties();
      m_bufferDeleter = &deleteBuffer;

      switch (props.getType())
      {
        case GPU_BUFFER_TYPE::kSTANDARD:
          bufferType = BUFFER_TYPE::kSTANDARD;
          break;
        case GPU_BUFFER_TYPE::kSTRUCTURED:
          bufferType = BUFFER_TYPE::kSTRUCTURED;
          break;
        case GPU_BUFFER_TYPE::kINDIRECTARGUMENT:
          bufferType = BUFFER_TYPE::kINDIRECTARGUMENT;
          break;
        default:
          GE_EXCEPT(InvalidParametersException,
                    "Unsupported buffer type " + toString(props.getType()));
      }

      m_buffer = ge_new<D3D11HardwareBuffer>(bufferType, props.getUsage(),
                                             props.getElementCount(),
                                             props.getElementSize(),
                                             rs->getPrimaryDevice(),
                                             false,
                                             false,
                                             props.getRandomGPUWrite(),
                                             props.getUseCounter());

      auto thisPtr = static_pointer_cast<D3D11GPUBuffer>(getThisPtr());
      uint32 usage = GPU_VIEW_USAGE::kDEFAULT;
      if (props.getRandomGPUWrite()) {
        usage |= GPU_VIEW_USAGE::kRANDOMWRITE;
      }

      //Keep a single view of the entire buffer, we don't support views of
      //sub-sets (yet)
      m_bufferView = requestView(thisPtr,
                                 0,
                                 props.getElementCount(),
                                 static_cast<GPU_VIEW_USAGE::E>(usage));

      GE_INC_RENDER_STAT_CAT(ResCreated,
                             RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);

      GPUBuffer::initialize();
    }

    void*
    D3D11GPUBuffer::lock(uint32 offset,
                         uint32 length,
                         GPU_LOCK_OPTIONS::E options,
                         uint32 /*deviceIdx*/,
                         uint32 /*queueIdx*/) {
#if GE_PROFILING_ENABLED
      if (GPU_LOCK_OPTIONS::kREAD_ONLY == options ||
          GPU_LOCK_OPTIONS::kREAD_WRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      }

      if (GPU_LOCK_OPTIONS::kREAD_WRITE == options||
          GPU_LOCK_OPTIONS::kWRITE_ONLY == options||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_DISCARD == options ||
          GPU_LOCK_OPTIONS::kWRITE_ONLY_NO_OVERWRITE == options) {
        GE_INC_RENDER_STAT_CAT(ResWrite,
                               RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      }
#endif
      return m_buffer->lock(offset, length, options);
    }

    void
    D3D11GPUBuffer::unlock() {
      m_buffer->unlock();
    }

    void
    D3D11GPUBuffer::readData(uint32 offset,
                             uint32 length,
                             void* dest,
                             uint32 /*deviceIdx*/,
                             uint32 /*queueIdx*/) {
      GE_INC_RENDER_STAT_CAT(ResRead, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      m_buffer->readData(offset, length, dest);
    }

    void
    D3D11GPUBuffer::writeData(uint32 offset,
                              uint32 length,
                              const void* source,
                              BUFFER_WRITE_TYPE::E writeFlags,
                              uint32 /*queueIdx*/) {
      GE_INC_RENDER_STAT_CAT(ResWrite, RENDER_STAT_RESOURCE_TYPE::kGPUBuffer);
      m_buffer->writeData(offset, length, source, writeFlags);
    }

    void
    D3D11GPUBuffer::copyData(HardwareBuffer& srcBuffer,
                             uint32 srcOffset,
                             uint32 dstOffset,
                             uint32 length,
                             bool discardWholeBuffer,
                             const SPtr<CommandBuffer>& commandBuffer) {
      auto d3d11SrcBuffer = static_cast<D3D11GPUBuffer*>(&srcBuffer);
      m_buffer->copyData(*d3d11SrcBuffer->m_buffer,
                         srcOffset,
                         dstOffset,
                         length,
                         discardWholeBuffer,
                         commandBuffer);
    }

    ID3D11Buffer*
    D3D11GPUBuffer::getDX11Buffer() const {
      return m_buffer->getD3DBuffer();
    }

    GPUBufferView*
    D3D11GPUBuffer::requestView(const SPtr<D3D11GPUBuffer>& buffer,
                                uint32 firstElement,
                                uint32 numElements,
                                GPU_VIEW_USAGE::E usage) {
      const auto& props = buffer->getProperties();

      GPU_BUFFER_VIEW_DESC key;
      key.firstElement = firstElement;
      key.elementWidth = props.getElementSize();
      key.numElements = numElements;
      key.usage = usage;
      key.format = props.getFormat();
      key.useCounter = props.getUseCounter();

      auto iterFind = buffer->m_bufferViews.find(key);
      if (buffer->m_bufferViews.end() == iterFind) {
        auto newView = ge_new<GPUBufferView>();
        newView->initialize(buffer, key);
        buffer->m_bufferViews[key] = ge_new<GPUBufferReference>(newView);
        iterFind = buffer->m_bufferViews.find(key);
      }

      ++iterFind->second->refCount;

      return iterFind->second->view;
    }

    void
    D3D11GPUBuffer::releaseView(GPUBufferView* view) {
      SPtr<D3D11GPUBuffer> buffer = view->getBuffer();

      auto iterFind = buffer->m_bufferViews.find(view->getDesc());
      if (buffer->m_bufferViews.end() == iterFind) {
        GE_EXCEPT(InternalErrorException,
                  "Trying to release a buffer view that doesn't exist!");
      }

      --iterFind->second->refCount;

      if (0 == iterFind->second->refCount) {
        GPUBufferReference* toRemove = iterFind->second;
        buffer->m_bufferViews.erase(iterFind);

        if (nullptr != toRemove->view) {
          ge_delete(toRemove->view);
        }

        ge_delete(toRemove);
      }
    }

    void
    D3D11GPUBuffer::clearBufferViews() {
      for (auto & bufferView : m_bufferViews) {
        if (bufferView.second->view != nullptr) {
          ge_delete(bufferView.second->view);
        }

        ge_delete(bufferView.second);
      }

      m_bufferViews.clear();
    }

    ID3D11ShaderResourceView*
    D3D11GPUBuffer::getSRV() const {
      return m_bufferView->getSRV();
    }

    ID3D11UnorderedAccessView*
    D3D11GPUBuffer::getUAV() const {
      return m_bufferView->getUAV();
    }
  }
}
