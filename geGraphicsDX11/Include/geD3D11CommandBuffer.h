/*****************************************************************************/
/**
 * @file    geD3D11CommandBuffer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/29
 * @brief   Command buffer implementation for DirectX 11.
 *
 * Command buffer implementation for DirectX 11, which doesn't support
 * multi-threaded command generation. Instead all commands are stored in an
 * internal buffer, and then sent to the actual render API when the buffer is
 * executed.
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
#include "geD3D11RenderAPI.h"

#include <geCommandBuffer.h>

namespace geEngineSDK {
  using std::function;

  namespace geCoreThread {
    class D3D11CommandBuffer : public CommandBuffer
    {
     public:
      /**
       * @brief Registers a new command in the command buffer.
       */
      void
      queueCommand(const function<void()> command);

      /**
       * @brief Appends all commands from the secondary buffer into this
       *        command buffer.
       */
      void
      appendSecondary(const SPtr<D3D11CommandBuffer>& secondaryBuffer);

      /**
       * @brief Executes all commands in the command buffer.
       *        Not supported on secondary buffer.
       */
      void
      executeCommands();

      /**
       * @brief Removes all commands from the command buffer.
       */
      void
      clear();

     private:
      friend class D3D11CommandBufferManager;
      friend class D3D11RenderAPI;

      D3D11CommandBuffer(GPU_QUEUE_TYPE::E type,
                         uint32 deviceIdx,
                         uint32 queueIdx,
                         bool secondary);

      Vector<function<void()>> m_commands;
      DrawOperationType m_activeDrawOp;
    };
  }
}
