/*****************************************************************************/
/**
 * @file    geD3D11GPUBufferView.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/14
 * @brief   Represents a specific view of a GPUBuffer.
 *
 * Represents a specific view of a GPUBuffer. Different views all of the same
 * buffer be used in different situations (for example for reading from a
 * shader, or for a unordered read/write operation).
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

namespace geEngineSDK {
  namespace geCoreThread {
    /**
     * @brief Descriptor structure used for initializing a GPUBufferView.
     * @see GPUBuffer
     */
    struct GE_CORE_EXPORT GPU_BUFFER_VIEW_DESC
    {
      uint32 firstElement;
      uint32 elementWidth;
      uint32 numElements;
      bool useCounter;
      GPU_VIEW_USAGE::E usage;
      GPU_BUFFER_FORMAT::E format;
    };

    /**
     * @brief Represents a specific view of a GPUBuffer. Different views all of
     *        the same buffer be used in different situations (for example for
     *        reading from a shader, or for a unordered read/write operation).
     */
    class GPUBufferView
    {
     public:
      class HashFunction
      {
       public:
        size_t
        operator()(const GPU_BUFFER_VIEW_DESC& key) const;
      };

      class EqualFunction
      {
       public:
        bool
        operator()(const GPU_BUFFER_VIEW_DESC& a,
                   const GPU_BUFFER_VIEW_DESC& b) const;
      };

      GPUBufferView();
      virtual ~GPUBufferView();

      /**
       * @brief Initializes a new buffer view for the specified buffer.
       *        Descriptor structure defines which portion of the buffer, and
       *        how will its contents be represented by the view.
       */
      void
      initialize(D3D11GPUBuffer* buffer, GPU_BUFFER_VIEW_DESC& desc);

      /**
       * @brief Returns a descriptor structure used for creating the view.
       */
      const GPU_BUFFER_VIEW_DESC&
      getDesc() const {
        return m_desc;
      }

      /**
       * @brief Returns the buffer this view was created for.
       */
      D3D11GPUBuffer*
      getBuffer() const {
        return m_buffer;
      }

      /**
       * @brief Returns index of first element in the buffer that this view
       *        provides access to.
       */
      uint32
      getFirstElement() const {
        return m_desc.firstElement;
      }

      /**
       * @brief Returns width of an element in the buffer, in bytes.
       */
      uint32
      getElementWidth() const {
        return m_desc.elementWidth;
      }

      /**
       * @brief Returns the total number of elements this buffer provides
       *        access to.
       */
      uint32
      getNumElements() const {
        return m_desc.numElements;
      }

      /**
       * @brief Returns true if this view allows a GPU program to use counters
       *        on the bound buffer.
       */
      bool
      getUseCounter() const {
        return m_desc.useCounter;
      }

      /**
       * @brief Returns view usage which determines where in the pipeline can
       *        the view be bound.
       */
      GPU_VIEW_USAGE::E
      getUsage() const {
        return m_desc.usage;
      }

      /**
       * @brief Returns the DX11 shader resource view object for the buffer.
       */
      ID3D11ShaderResourceView*
      getSRV() const {
        return m_srv;
      }

      /**
       * @brief Returns the DX11 unordered access view object for the buffer.
       */
      ID3D11UnorderedAccessView*
      getUAV() const {
        return m_uav;
      }

     private:
      /**
       * @brief Creates a DX11 shader resource view that allows a buffer to be
       *        bound to a shader for reading (the most common option).
       * @param[in] buffer        Buffer to create the view for.
       * @param[in] firstElement  Index of the first element the view manages.
       *                          Interpretation of this value depends on exact
       *                          buffer type. It may be byte offset for raw
       *                          buffers, or number of elements for structured
       *                          buffers.
       * @param[in] elementWidth  Width of a single element in the buffer.
       *                          Size of the structure in structured buffers
       *                          and ignored for raw buffers as they always
       *                          operate on single byte basis.
       * @param[in] numElements   Number of elements the view manages, starting
       *                          after the "firstElement". This means number
       *                          of bytes for raw buffers or number of
       *                          structures for structured buffers.
       * @return Constructed DX11 shader resource view object.
       */
      ID3D11ShaderResourceView*
      createSRV(D3D11GPUBuffer* buffer,
                uint32 firstElement,
                uint32 elementWidth,
                uint32 numElements);

      /**
       * @brief Creates a DX11 unordered access view that allows a buffer to be
       *        bound to a shader for random reading or writing.
       * @param[in] buffer        Buffer to create the view for.
       * @param[in] firstElement  Index of the first element the view manages.
       *                          Interpretation of this value depends on exact
       *                          buffer type. It may be byte offset for raw
       *                          buffers, or number of elements for structured
       *                          buffers.
       * @param[in] numElements   Number of elements the view manages, starting
       *                          after the @p firstElement. This means number
       *                          of bytes for raw buffers or number of
       *                          structures for structured buffers.
       * @return Constructed DX11 unordered access view object.
       */
      ID3D11UnorderedAccessView*
      createUAV(D3D11GPUBuffer* buffer,
                uint32 firstElement,
                uint32 numElements,
                bool useCounter);

      ID3D11ShaderResourceView* m_srv;
      ID3D11UnorderedAccessView* m_uav;

      GPU_BUFFER_VIEW_DESC m_desc;
      D3D11GPUBuffer* m_buffer;
    };
  }
}
