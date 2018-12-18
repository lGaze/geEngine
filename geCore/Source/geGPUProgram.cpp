/*****************************************************************************/
/**
 * @file    geGPUProgram.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/12
 * @brief   Contains a GPU program.
 *
 * Contains a GPU program such as vertex or fragment program which gets
 * compiled from the provided source code.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geGPUProgram.h"
#include "geGPUProgramRTTI.h"
#include "geRenderAPICapabilities.h"
#include "geRenderAPI.h"
#include "geGPUParams.h"
#include "geGPUParamDesc.h"
#include "geGPUProgramManager.h"

namespace geEngineSDK {
  using std::static_pointer_cast;

  GPUProgramBytecode::~GPUProgramBytecode() {
    if (instructions.data) {
      ge_free(instructions.data);
    }
  }

  RTTITypeBase*
  GPUProgramBytecode::getRTTIStatic() {
    return GPUProgramBytecodeRTTI::instance();
  }

  RTTITypeBase*
  GPUProgramBytecode::getRTTI() const {
    return GPUProgramBytecode::getRTTIStatic();
  }

  GPUProgram::GPUProgram(const GPU_PROGRAM_DESC& desc)
    : m_needsAdjacencyInfo(desc.requiresAdjacency),
      m_language(desc.language),
      m_type(desc.type),
      m_entryPoint(desc.entryPoint),
      m_source(desc.source)
  {}

  bool
  GPUProgram::isCompiled() const {
    return getCore()->isCompiled();
  }

  String
  GPUProgram::getCompileErrorMessage() const {
    return getCore()->getCompileErrorMessage();
  }

  SPtr<GPUParamDesc>
  GPUProgram::getParamDesc() const {
    return getCore()->getParamDesc();
  }

  SPtr<geCoreThread::GPUProgram>
  GPUProgram::getCore() const {
    return static_pointer_cast<geCoreThread::GPUProgram>(m_coreSpecific);
  }

  SPtr<geCoreThread::CoreObject>
  GPUProgram::createCore() const {
    GPU_PROGRAM_DESC desc;
    desc.source = m_source;
    desc.entryPoint = m_entryPoint;
    desc.language = m_language;
    desc.type = m_type;
    desc.requiresAdjacency = m_needsAdjacencyInfo;
    desc.bytecode = m_bytecode;

    return geCoreThread::GPUProgramManager::instance().createInternal(desc);
  }

  SPtr<GPUProgram>
  GPUProgram::create(const GPU_PROGRAM_DESC& desc) {
    return GPUProgramManager::instance().create(desc);
  }

  /***************************************************************************/
  /**
  * Includes
  */
  /***************************************************************************/
  RTTITypeBase*
  GPUProgram::getRTTIStatic() {
    return GPUProgramRTTI::instance();
  }

  RTTITypeBase*
  GPUProgram::getRTTI() const {
    return GPUProgram::getRTTIStatic();
  }

  namespace geCoreThread {
    GPUProgram::GPUProgram(const GPU_PROGRAM_DESC& desc,
                           GPU_DEVICE_FLAGS::E /*deviceMask*/)
      : m_needsAdjacencyInfo(desc.requiresAdjacency),
        m_type(desc.type),
        m_entryPoint(desc.entryPoint),
        m_source(desc.source),
        m_bytecode(desc.bytecode) {
      m_parametersDesc = ge_shared_ptr_new<GPUParamDesc>();
    }

    bool
    GPUProgram::isSupported() const {
      return true;
    }

    SPtr<GPUProgram>
    GPUProgram::create(const GPU_PROGRAM_DESC& desc,
                       GPU_DEVICE_FLAGS::E deviceMask) {
      return GPUProgramManager::instance().create(desc, deviceMask);
    }

    SPtr<GPUProgramBytecode>
    GPUProgram::compileBytecode(const GPU_PROGRAM_DESC& desc) {
      return GPUProgramManager::instance().compileBytecode(desc);
    }
  }
}