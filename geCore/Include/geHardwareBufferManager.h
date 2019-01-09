/*****************************************************************************/
/**
 * @file    geHardwareBufferManager.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/08
 * @brief   Handles creation of various hardware buffers.
 *
 * Handles creation of various hardware buffers.
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
#include "geVertexBuffer.h"
#include "geIndexBuffer.h"
#include "geVertexDeclaration.h"

#include <geModule.h>

namespace geEngineSDK {
  struct GPU_BUFFER_DESC;
  struct GPU_PARAMS_DESC;

  /**
   * @brief Handles creation of various hardware buffers.
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT HardwareBufferManager
    : public Module<HardwareBufferManager>
  {
   public:
    HardwareBufferManager() = default;
    virtual ~HardwareBufferManager() = default;

    /**
     * @brief Creates a new vertex buffer used for holding number of vertices
     *        and other per-vertex data. Buffer can be bound to the pipeline
     *        and its data can be passed to the active vertex GPU program.
     * @param[in] desc  Description of the buffer to create.
     */
    SPtr<VertexBuffer>
    createVertexBuffer(const VERTEX_BUFFER_DESC& desc);

    /**
     * @brief Creates a new index buffer that holds indices referencing
     *        vertices in a vertex buffer. Indices are interpreted by the
     *        pipeline and vertices are drawn in the order specified in the
     *        index buffer.
     * @param[in] desc  Description of the buffer to create.
     */
    SPtr<IndexBuffer>
    createIndexBuffer(const INDEX_BUFFER_DESC& desc);

    /**
     * @brief Creates a GPU parameter block that you can use for setting
     *        parameters for GPU programs. Parameter blocks may be used for
     *        sharing parameter data between multiple GPU programs, requiring
     *        you to update only one buffer for all of them, potentially
     *        improving performance.
     * @param[in] size  Size of the parameter buffer in bytes.
     * @param[in] usage Usage that tells the hardware how will be buffer be
     *                  used.
     */
    SPtr<GPUParamBlockBuffer>
    createGPUParamBlockBuffer(uint32 size,
                              GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kDYNAMIC);

    /**
     * @brief Creates a generic buffer that can be passed as a parameter to a
     *        GPU program. This type of buffer can hold various type of data
     *        and can be used for various purposes. See GPUBufferType for
     *        explanation of different buffer types.
     * @param[in] desc  Description of the buffer to create.
     */
    SPtr<GPUBuffer>
    createGPUBuffer(const GPU_BUFFER_DESC& desc);

    /**
     * @brief Creates a new vertex declaration from a list of vertex elements.
     * @param[in] desc  Description of the object to create.
     */
    SPtr<VertexDeclaration>
    createVertexDeclaration(const SPtr<VertexDataDesc>& desc);

    /**
     * @copydoc GPUParams::create(const SPtr<GPUPipelineParamInfo>&)
     */
    SPtr<GPUParams>
    createGPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo);
  };

  namespace geCoreThread {
    /**
     * @brief Handles creation of various hardware buffers.
     * @note  Core thread only.
     */
    class GE_CORE_EXPORT HardwareBufferManager
      : public Module<HardwareBufferManager>
    {
     public:
      virtual ~HardwareBufferManager() = default;

      /**
       * @copydoc geEngineSDK::HardwareBufferManager::createVertexBuffer
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<VertexBuffer>
      createVertexBuffer(const VERTEX_BUFFER_DESC& desc,
                         GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::HardwareBufferManager::createIndexBuffer
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<IndexBuffer>
      createIndexBuffer(const INDEX_BUFFER_DESC& desc,
                        GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::HardwareBufferManager::createVertexDeclaration
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<VertexDeclaration>
      createVertexDeclaration(const SPtr<VertexDataDesc>& desc,
                              GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @brief Creates a new vertex declaration from a list of vertex
       *        elements.
       * @param[in] elements    List of elements to initialize the declaration
       *                        with.
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<VertexDeclaration>
      createVertexDeclaration(const Vector<VertexElement>& elements,
                              GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::HardwareBufferManager::createGPUParamBlockBuffer
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<GPUParamBlockBuffer>
      createGPUParamBlockBuffer(uint32 size,
                                GPU_BUFFER_USAGE::E usage = GPU_BUFFER_USAGE::kDYNAMIC,
                                GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::HardwareBufferManager::createGPUBuffer
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the object be created on.
       */
      SPtr<GPUBuffer>
      createGPUBuffer(const GPU_BUFFER_DESC& desc,
                      GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc GPUBuffer::create(const GPU_BUFFER_DESC&, SPtr<HardwareBuffer>)
       */
      SPtr<GPUBuffer>
      createGPUBuffer(const GPU_BUFFER_DESC& desc, SPtr<HardwareBuffer> underlyingBuffer);

      /**
       * @copydoc GPUParams::create(const SPtr<GPUPipelineParamInfo>&,
       *                            GPUDeviceFlags)
       */
      SPtr<GPUParams>
      createGPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo,
                      GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      friend class geEngineSDK::IndexBuffer;
      friend class geEngineSDK::VertexBuffer;
      friend class geEngineSDK::VertexDeclaration;
      friend class geEngineSDK::GPUParamBlockBuffer;
      friend class geEngineSDK::GPUBuffer;
      friend class IndexBuffer;
      friend class VertexBuffer;
      friend class GPUBuffer;

      /**
       * @brief Key for use in the vertex declaration map.
       */
      struct VertexDeclarationKey
      {
        VertexDeclarationKey(const Vector<VertexElement>& elements);

        class HashFunction
        {
         public:
          size_t
          operator()(const VertexDeclarationKey& key) const;
        };

        class EqualFunction
        {
         public:
          bool
          operator()(const VertexDeclarationKey& lhs,
                     const VertexDeclarationKey& rhs) const;
        };

        Vector<VertexElement> elements;
      };

      /**
       * @copydoc createVertexBuffer
       */
      virtual SPtr<VertexBuffer>
      createVertexBufferInternal(const VERTEX_BUFFER_DESC& desc,
                                 GPU_DEVICE_FLAGS::E deviceMask =
                                   GPU_DEVICE_FLAGS::kDEFAULT) = 0;

      /**
       * @copydoc createIndexBuffer
       */
      virtual SPtr<IndexBuffer>
      createIndexBufferInternal(const INDEX_BUFFER_DESC& desc,
                                GPU_DEVICE_FLAGS::E deviceMask =
                                  GPU_DEVICE_FLAGS::kDEFAULT) = 0;

      /**
       * @copydoc createGPUParamBlockBuffer
       */
      virtual SPtr<GPUParamBlockBuffer>
      createGPUParamBlockBufferInternal(uint32 size,
                                        GPU_BUFFER_USAGE::E usage =
                                          GPU_BUFFER_USAGE::kDYNAMIC,
                                        GPU_DEVICE_FLAGS::E deviceMask =
                                          GPU_DEVICE_FLAGS::kDEFAULT) = 0;

      /**
       * @copydoc createGPUBuffer
       */
      virtual SPtr<GPUBuffer>
      createGPUBufferInternal(const GPU_BUFFER_DESC& desc,
                              GPU_DEVICE_FLAGS::E deviceMask =
                                GPU_DEVICE_FLAGS::kDEFAULT) = 0;

      /**
       * @copydoc createGpuBuffer(const GPU_BUFFER_DESC&, SPtr<HardwareBuffer>)
       */
      virtual SPtr<GPUBuffer>
      createGPUBufferInternal(const GPU_BUFFER_DESC& desc,
                              SPtr<HardwareBuffer> underlyingBuffer) = 0;

      /**
       * @copydoc createVertexDeclaration(const Vector<VertexElement>&,
       *                                  GPUDeviceFlags)
       */
      virtual SPtr<VertexDeclaration>
      createVertexDeclarationInternal(const Vector<VertexElement>& elements,
                                      GPU_DEVICE_FLAGS::E deviceMask =
                                        GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc createGPUParams
       */
      virtual SPtr<GPUParams>
      createGPUParamsInternal(const SPtr<GPUPipelineParamInfo>& paramInfo,
                              GPU_DEVICE_FLAGS::E deviceMask =
                                GPU_DEVICE_FLAGS::kDEFAULT);

      using DeclarationMap = UnorderedMap<VertexDeclarationKey,
                                          SPtr<VertexDeclaration>,
                                          VertexDeclarationKey::HashFunction,
                                          VertexDeclarationKey::EqualFunction>;

      DeclarationMap m_cachedDeclarations;
    };
  }
}
