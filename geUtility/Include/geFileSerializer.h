/*****************************************************************************/
/**
 * @file    geFileSerializer.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Encodes objects to the specified file using the RTTI system.
 *
 * Encodes the provided object to the specified file using the RTTI system.
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
#include "gePrerequisitesUtil.h"

namespace geEngineSDK {
  struct SerializationContext;

  using std::ofstream;

  class GE_UTILITY_EXPORT FileEncoder
  {
   public:
    FileEncoder(const Path& fileLocation);
    ~FileEncoder();

    /**
     * @brief Parses the provided object, serializes all of its data as
     *        specified by its RTTIType and saves the serialized data to the
     *        provided file location.
     * @param[in] object  Object to encode.
     * @param[in] params  Optional parameters to be passed to the serialization
     *            callbacks on the objects being serialized.
     */
    void
    encode(IReflectable* object, SerializationContext* context = nullptr);

   private:
    /**
     * @brief Called by the binary serializer whenever the buffer gets full.
     */
    uint8*
    flushBuffer(uint8* bufferStart, uint32 bytesWritten, uint32& newBufferSize);

    ofstream m_outputStream;
    uint8* m_writeBuffer = nullptr;

    static const uint32 WRITE_BUFFER_SIZE = 2048;
  };

  /**
   * @brief Decodes objects from the specified file using the RTTI system.\
   */
  class GE_UTILITY_EXPORT FileDecoder
  {
   public:
    FileDecoder(const Path& fileLocation);

    /**
     * @brief Deserializes an IReflectable object by reading the binary data at
     *        the provided file location.
     * @param[in] params  Optional parameters to be passed to the serialization
     *            callbacks on the objects being serialized.
     */
    SPtr<IReflectable>
    decode(SerializationContext* context = nullptr);

    /**
     * @brief Skips over an object in the file. Calling decode() will decode
     *        the next object.
     */
    void
    skip();

   private:
    SPtr<DataStream> m_inputStream;
  };
}
