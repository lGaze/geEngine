/*****************************************************************************/
/**
 * @file    geFileSerializer.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Encodes objects to the specified file using the RTTI system.
 *
 * Encodes the provided object to the specified file using the RTTI system.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geFileSerializer.h"
#include "geException.h"
#include "geIReflectable.h"
#include "geBinarySerializer.h"
#include "geFileSystem.h"
#include "geDataStream.h"
#include "geDebug.h"

namespace geEngineSDK {
  using namespace std::placeholders;
  using std::ios;
  using std::ios_base;
  using std::bind;

  FileEncoder::FileEncoder(const Path& fileLocation) {
    m_writeBuffer = reinterpret_cast<uint8*>(
                      ge_alloc(static_cast<SIZE_T>(WRITE_BUFFER_SIZE)));

    Path parentDir = fileLocation.getDirectory();
    if (!FileSystem::exists(parentDir)) {
      FileSystem::createDir(parentDir);
    }

    m_outputStream.open(fileLocation.toPlatformString().c_str(),
                        ios::out | ios::binary);
    if (m_outputStream.fail()) {
      LOGWRN("Failed to save file: \"" + fileLocation.toString() +
             "\". Error: " + strerror(errno) + ".");
    }
  }

  FileEncoder::~FileEncoder() {
    ge_free(m_writeBuffer);
    m_outputStream.close();
    m_outputStream.clear();
  }

  void
  FileEncoder::encode(IReflectable* object, SerializationContext* context) {
    if (nullptr == object) {
      return;
    }

    auto curPos = static_cast<uint64>(m_outputStream.tellp());
    m_outputStream.seekp(sizeof(uint32), ios_base::cur);

    BinarySerializer bs;
    uint32 totalBytesWritten = 0;
    bs.encode(object,
              m_writeBuffer,
              WRITE_BUFFER_SIZE,
              &totalBytesWritten,
              bind(&FileEncoder::flushBuffer, this, _1, _2, _3),
              false,
              context);

    m_outputStream.seekp(curPos);
    m_outputStream.write(reinterpret_cast<char*>(&totalBytesWritten),
                         sizeof(totalBytesWritten));
    m_outputStream.seekp(totalBytesWritten, ios_base::cur);
  }

  uint8*
  FileEncoder::flushBuffer(uint8* bufferStart,
                           uint32 bytesWritten,
                           uint32& /*newBufferSize*/) {
    m_outputStream.write(reinterpret_cast<const char*>(bufferStart), bytesWritten);
    return bufferStart;
  }

  FileDecoder::FileDecoder(const Path& fileLocation) {
    m_inputStream = FileSystem::openFile(fileLocation, true);

    if (nullptr == m_inputStream) {
      return;
    }

    if (m_inputStream->size() > NumLimit::MAX_UINT32) {
      GE_EXCEPT(InternalErrorException,
                "File size is larger that uint32 can hold. Ask a programmer "
                "to use a bigger data type.");
    }
  }

  SPtr<IReflectable>
  FileDecoder::decode(SerializationContext* context) {
    if (m_inputStream->isEOF()) {
      return nullptr;
    }

    uint32 objectSize = 0;
    m_inputStream->read(&objectSize, sizeof(objectSize));

    BinarySerializer bs;
    SPtr<IReflectable> object = bs.decode(m_inputStream, objectSize, context);

    return object;
  }

  void
  FileDecoder::skip() {
    if (m_inputStream->isEOF()) {
      return;
    }

    uint32 objectSize = 0;
    m_inputStream->read(&objectSize, sizeof(objectSize));
    m_inputStream->skip(objectSize);
  }
}
