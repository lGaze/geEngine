/*****************************************************************************/
/**
 * @file    geHardwareBufferManager.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Handles creation of various hardware buffers.
 *
 * Handles creation of various hardware buffers.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geHardwareBufferManager.h"

#include "geVertexData.h"
#include "geGPUBuffer.h"
#include "geVertexDeclaration.h"
#include "geGPUParamBlockBuffer.h"
#include "geVertexDataDesc.h"
#include "geGPUParams.h"

namespace geEngineSDK {
  using std::move;

  SPtr<VertexDeclaration>
  HardwareBufferManager::createVertexDeclaration(const SPtr<VertexDataDesc>& desc) {
    VertexDeclaration* decl = GE_PVT_NEW(VertexDeclaration, desc->createElements());
    SPtr<VertexDeclaration> declPtr = ge_core_ptr<VertexDeclaration>(decl);

    declPtr->_setThisPtr(declPtr);
    declPtr->initialize();

    return declPtr;
  }

  SPtr<VertexBuffer>
  HardwareBufferManager::createVertexBuffer(const VERTEX_BUFFER_DESC& desc) {
    SPtr<VertexBuffer> vbuf = ge_core_ptr<VertexBuffer>(GE_PVT_NEW(VertexBuffer, desc));

    vbuf->_setThisPtr(vbuf);
    vbuf->initialize();

    return vbuf;
  }

  SPtr<IndexBuffer>
  HardwareBufferManager::createIndexBuffer(const INDEX_BUFFER_DESC& desc) {
    SPtr<IndexBuffer> ibuf = ge_core_ptr<IndexBuffer>(GE_PVT_NEW(IndexBuffer, desc));

    ibuf->_setThisPtr(ibuf);
    ibuf->initialize();

    return ibuf;
  }

  SPtr<GPUParamBlockBuffer>
  HardwareBufferManager::createGPUParamBlockBuffer(uint32 size, GPU_BUFFER_USAGE::E usage) {
    auto paramBlockPtr = ge_core_ptr<GPUParamBlockBuffer>(
      new (ge_alloc<GPUParamBlockBuffer>()) GPUParamBlockBuffer(size, usage));

    paramBlockPtr->_setThisPtr(paramBlockPtr);
    paramBlockPtr->initialize();

    return paramBlockPtr;
  }

  SPtr<GPUBuffer>
  HardwareBufferManager::createGPUBuffer(const GPU_BUFFER_DESC& desc) {
    SPtr<GPUBuffer> gbuf = ge_core_ptr<GPUBuffer>(GE_PVT_NEW(GPUBuffer, desc));
    gbuf->_setThisPtr(gbuf);
    gbuf->initialize();

    return gbuf;
  }

  SPtr<GPUParams>
  HardwareBufferManager::createGPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo) {
    auto params = GE_PVT_NEW(GPUParams, paramInfo);
    SPtr<GPUParams> paramsPtr = ge_core_ptr<GPUParams>(params);

    paramsPtr->_setThisPtr(paramsPtr);
    paramsPtr->initialize();

    return paramsPtr;
  }

  namespace geCoreThread {
    HardwareBufferManager::
      VertexDeclarationKey::VertexDeclarationKey(const Vector<VertexElement>& elements)
        : elements(elements)
    {}

    size_t
    HardwareBufferManager::
      VertexDeclarationKey::HashFunction::operator()(const VertexDeclarationKey& v) const {
      size_t hash = 0;
      for (auto& entry : v.elements) {
        hash_combine(hash, VertexElement::getHash(entry));
      }

      return hash;
    }

    bool
    HardwareBufferManager::
      VertexDeclarationKey::EqualFunction::operator()(const VertexDeclarationKey& lhs,
                                                      const VertexDeclarationKey& rhs) const {
      if (lhs.elements.size() != rhs.elements.size()) {
        return false;
      }

      size_t numElements = lhs.elements.size();
      auto iterLeft = lhs.elements.begin();
      auto iterRight = rhs.elements.begin();
      for (size_t i = 0; i < numElements; ++i) {
        if (*iterLeft != *iterRight) {
          return false;
        }

        ++iterLeft;
        ++iterRight;
      }

      return true;
    }

    SPtr<IndexBuffer>
    HardwareBufferManager::createIndexBuffer(const INDEX_BUFFER_DESC& desc,
                                             GPU_DEVICE_FLAGS::E deviceMask) {
      SPtr<IndexBuffer> ibuf = createIndexBufferInternal(desc, deviceMask);
      ibuf->initialize();

      return ibuf;
    }

    SPtr<VertexBuffer>
    HardwareBufferManager::createVertexBuffer(const VERTEX_BUFFER_DESC& desc,
                                              GPU_DEVICE_FLAGS::E deviceMask) {
      SPtr<VertexBuffer> vbuf = createVertexBufferInternal(desc, deviceMask);
      vbuf->initialize();

      return vbuf;
    }

    SPtr<VertexDeclaration>
    HardwareBufferManager::createVertexDeclaration(const SPtr<VertexDataDesc>& desc,
                                                   GPU_DEVICE_FLAGS::E deviceMask) {
      Vector<VertexElement> elements = desc->createElements();
      return createVertexDeclaration(elements, deviceMask);
    }

    SPtr<GPUParams>
    HardwareBufferManager::createGPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo,
                                           GPU_DEVICE_FLAGS::E deviceMask) {
      SPtr<GPUParams> params = createGPUParamsInternal(paramInfo, deviceMask);
      params->initialize();

      return params;
    }

    SPtr<VertexDeclaration>
    HardwareBufferManager::createVertexDeclaration(const Vector<VertexElement>& elements,
                                                   GPU_DEVICE_FLAGS::E deviceMask) {
      VertexDeclarationKey key(elements);

      auto iterFind = m_cachedDeclarations.find(key);
      if (iterFind != m_cachedDeclarations.end()) {
        return iterFind->second;
      }

      SPtr<VertexDeclaration> declPtr = createVertexDeclarationInternal(elements, deviceMask);
      declPtr->initialize();

      m_cachedDeclarations[key] = declPtr;
      return declPtr;
    }

    SPtr<GPUParamBlockBuffer>
    HardwareBufferManager::createGPUParamBlockBuffer(uint32 size,
                                                     GPU_BUFFER_USAGE::E usage,
                                                     GPU_DEVICE_FLAGS::E deviceMask) {
      auto paramBlockPtr = createGPUParamBlockBufferInternal(size, usage, deviceMask);
      paramBlockPtr->initialize();

      return paramBlockPtr;
    }

    SPtr<GPUBuffer>
    HardwareBufferManager::createGPUBuffer(const GPU_BUFFER_DESC& desc,
                                           GPU_DEVICE_FLAGS::E deviceMask) {
      SPtr<GPUBuffer> gbuf = createGPUBufferInternal(desc, deviceMask);
      gbuf->initialize();

      return gbuf;
    }

    SPtr<GPUBuffer>
    HardwareBufferManager::createGPUBuffer(const GPU_BUFFER_DESC& desc,
                                           SPtr<HardwareBuffer> underlyingBuffer) {
      SPtr<GPUBuffer> gbuf = createGPUBufferInternal(desc, move(underlyingBuffer));
      gbuf->initialize();

      return gbuf;
    }

    SPtr<VertexDeclaration>
    HardwareBufferManager::createVertexDeclarationInternal(
                                                        const Vector<VertexElement>& elements,
                                                        GPU_DEVICE_FLAGS::E deviceMask) {
      auto decl = GE_PVT_NEW(VertexDeclaration, elements, deviceMask);
      SPtr<VertexDeclaration> ret = ge_shared_ptr<VertexDeclaration>(decl);
      ret->_setThisPtr(ret);

      return ret;
    }

    SPtr<GPUParams>
    HardwareBufferManager::createGPUParamsInternal(
                                                  const SPtr<GPUPipelineParamInfo>& paramInfo,
                                                  GPU_DEVICE_FLAGS::E deviceMask) {
      auto params = GE_PVT_NEW(GPUParams, paramInfo, deviceMask);
      SPtr<GPUParams> paramsPtr = ge_shared_ptr<GPUParams>(params);
      paramsPtr->_setThisPtr(paramsPtr);

      return paramsPtr;
    }
  }
}
