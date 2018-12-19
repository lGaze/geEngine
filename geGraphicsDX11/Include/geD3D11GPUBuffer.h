/*****************************************************************************/
/**
 * @file    geD3D11GPUBuffer.h
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
#include "gePrerequisitesD3D11.h"
#include "geD3D11GPUBufferView.h"

#include <geGPUBuffer.h>

namespace geEngineSDK {
  namespace geCoreThread {
    class D3D11GPUBuffer : public GPUBuffer
    {
    public:
      ~D3D11GPUBuffer();

      /**
       * @brief Creates a buffer view that may be used for binding a buffer to
       *        a slot in the pipeline. Views allow you to specify how is data
       *        in the buffer organized to make it easier for the pipeline to
       *        interpret.
       * @param[in] buffer        Buffer to create the view for.
       * @param[in] firstElement  Position of the first element visible by the
       *                          view.
       * @param[in] numElements   Number of elements to bind to the view.
       * @param[in] usage         Determines type of the view we are creating,
       *                          and which slots in the pipeline will the view
       *                          be bindable to.
       * @note  If a view with this exact parameters already exists, it will be
       *        returned and new one will not be created.
       * @note  Only Default and RandomWrite views are supported for this type
       *        of buffer.
       */
      //TODO: Low Priority: Perhaps reflect usage flag limitation by having an
      //enum with only the supported two options?
      static GPUBufferView*
      requestView(D3D11GPUBuffer* buffer,
                  uint32 firstElement,
                  uint32 numElements,
                  GPU_VIEW_USAGE::E usage);

      /**
       * @brief Releases a view created with requestView.
       * @note  View will only truly get released once all references to it
       *        are released.
       */
      static void
      releaseView(GPUBufferView* view);

      /**
       * @brief Returns the internal DX11 GPU buffer object.
       */
      ID3D11Buffer*
      getDX11Buffer() const;

      /**
       * @brief Returns the DX11 shader resource view object for the buffer.
       */
      ID3D11ShaderResourceView*
      getSRV() const;

      /**
       * @brief Returns the DX11 unordered access view object for the buffer.
       */
      ID3D11UnorderedAccessView*
      getUAV() const;

     protected:
      friend class D3D11HardwareBufferManager;

      D3D11GPUBuffer(const GPU_BUFFER_DESC& desc,
                     GPU_DEVICE_FLAGS::E deviceMask);

      D3D11GPUBuffer(const GPU_BUFFER_DESC& desc,
                     SPtr<HardwareBuffer> underlyingBuffer);

      /**
       * @brief Destroys all buffer views regardless if their reference count
       *        is zero or not.
       */
      void
      clearBufferViews();

      /**
       * @copydoc GPUBuffer::initialize
       */
      void
      initialize() override;

     private:
      /**
       * @brief Helper class to help with reference counting for GPU buffer views.
       */
      struct GPUBufferReference
      {
        GPUBufferReference(GPUBufferView* _view)
          : view(_view),
            refCount(0)
        {}

        GPUBufferView* view;
        uint32 refCount;
      };

      GPUBufferView* m_bufferView = nullptr;
      UnorderedMap<GPU_BUFFER_VIEW_DESC,
                   GPUBufferReference*,
                   GPUBufferView::HashFunction,
                   GPUBufferView::EqualFunction> m_bufferViews;
    };
  }
}
