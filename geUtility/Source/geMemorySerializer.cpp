/*****************************************************************************/
/**
 * @file    geMemorySerializer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Encodes/decodes an IReflectable object from/to memory.
 *
 * Encodes/decodes an IReflectable object from/to memory.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geMemorySerializer.h"

#include "geException.h"
#include "geIReflectable.h"
#include "geBinarySerializer.h"
#include "geDataStream.h"

namespace geEngineSDK {
  using namespace std::placeholders;
  using std::function;
  using std::bind;

  uint8*
  MemorySerializer::encode(IReflectable* object,
                           uint32& bytesWritten,
                           function<void*(SIZE_T)> allocator,
                           bool shallow,
                           SerializationContext* context) {
    BinarySerializer bs;

    BufferPiece piece;
    piece.buffer = reinterpret_cast<uint8*>(ge_alloc(WRITE_BUFFER_SIZE));
    piece.size = 0;

    m_bufferPieces.push_back(piece);

    bs.encode(object,
              piece.buffer,
              WRITE_BUFFER_SIZE,
              &bytesWritten,
              bind(&MemorySerializer::flushBuffer, this, _1, _2, _3),
              shallow,
              context);

    uint8* resultBuffer;
    if (nullptr != allocator) {
      resultBuffer = reinterpret_cast<uint8*>(allocator(static_cast<SIZE_T>(bytesWritten)));
    }
    else {
      resultBuffer = reinterpret_cast<uint8*>(ge_alloc(static_cast<SIZE_T>(bytesWritten)));
    }

    SIZE_T offset = 0;
    for (auto& bufferPiece : m_bufferPieces) {
      if (bufferPiece.size > 0) {
        memcpy(resultBuffer + offset, bufferPiece.buffer, bufferPiece.size);
        offset += bufferPiece.size;
      }
    }

    for (auto iter = m_bufferPieces.rbegin(); iter != m_bufferPieces.rend(); ++iter) {
      ge_free(iter->buffer);
    }

    m_bufferPieces.clear();

    return resultBuffer;
  }

  SPtr<IReflectable>
  MemorySerializer::decode(uint8* buffer,
                           uint32 bufferSize,
                           SerializationContext* context) {
    SPtr<MemoryDataStream>
      stream = ge_shared_ptr_new<MemoryDataStream>(buffer, bufferSize, false);

    BinarySerializer bs;
    SPtr<IReflectable> object = bs.decode(stream, bufferSize, context);

    return object;
  }

  uint8*
  MemorySerializer::flushBuffer(uint8* /*bufferStart*/,
                                uint32 bytesWritten,
                                uint32& /*newBufferSize*/) {
    m_bufferPieces.back().size = bytesWritten;

    BufferPiece piece;
    piece.buffer = reinterpret_cast<uint8*>(ge_alloc(WRITE_BUFFER_SIZE));
    piece.size = 0;

    m_bufferPieces.push_back(piece);

    return piece.buffer;
  }
}
