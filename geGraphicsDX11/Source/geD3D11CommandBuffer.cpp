/*****************************************************************************/
/**
 * @file    geD3D11CommandBuffer.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geD3D11CommandBuffer.h"

#include <geException.h>

namespace geEngineSDK {
  using std::function;

  namespace geCoreThread {
    D3D11CommandBuffer::D3D11CommandBuffer(GPU_QUEUE_TYPE::E type,
                                           uint32 deviceIdx,
                                           uint32 queueIdx,
                                           bool secondary)
      : CommandBuffer(type, deviceIdx, queueIdx, secondary),
        m_activeDrawOp(DOT_TRIANGLE_LIST) {
      if (0 != deviceIdx) {
        GE_EXCEPT(InvalidParametersException,
                  "Only a single device supported on DX11.");
      }
    }

    void
    D3D11CommandBuffer::queueCommand(const function<void()> command) {
      m_commands.push_back(command);
    }

    void
    D3D11CommandBuffer::appendSecondary(const SPtr<D3D11CommandBuffer>& secondaryBuffer) {
#if GE_DEBUG_MODE
      if (!secondaryBuffer->m_isSecondary) {
        LOGERR("Cannot append a command buffer that is not secondary.");
        return;
      }

      if (m_isSecondary) {
        LOGERR("Cannot append a buffer to a secondary command buffer.");
        return;
      }
#endif
      for (auto& entry : secondaryBuffer->m_commands) {
        m_commands.push_back(entry);
      }
    }

    void
    D3D11CommandBuffer::executeCommands() {
#if GE_DEBUG_MODE
      if (m_isSecondary) {
        LOGERR("Cannot execute commands on a secondary buffer.");
        return;
      }
#endif
      for (auto& entry : m_commands) {
        entry();
      }
    }

    void
    D3D11CommandBuffer::clear() {
      m_commands.clear();
    }
  }
}
