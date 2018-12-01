/*****************************************************************************/
/**
 * @file    geGPUProgramRTTI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/02
 * @brief   RTTI Objects for geGPUProgram.
 *
 * RTTI Objects for geGPUProgram.
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
#include "geGPUProgram.h"
#include "geGPUParamDesc.h"
#include "geGPUProgramManager.h"

#include <geRTTIType.h>

namespace geEngineSDK {
  class GE_CORE_EXPORT GPUProgramBytecodeRTTI
    : public RTTIType<GPUProgramBytecode, IReflectable, GPUProgramBytecodeRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(instructions, 0)
      GE_RTTI_MEMBER_REFLPTR(paramDesc, 1)
      GE_RTTI_MEMBER_PLAIN(vertexInput, 2)
      GE_RTTI_MEMBER_PLAIN(messages, 3)
      GE_RTTI_MEMBER_PLAIN(compilerId, 4)
      GE_RTTI_MEMBER_PLAIN(compilerVersion, 5)
    GE_END_RTTI_MEMBERS

   public:
    const String&
    getRTTIName() override {
      static String name = "GPUProgramBytecode";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_GPUProgramBytecode;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<GPUProgramBytecode>();
    }
  };

  class GE_CORE_EXPORT GPUParamDescRTTI
    : public RTTIType<GPUParamDesc, IReflectable, GPUParamDescRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(paramBlocks, 0)
      GE_RTTI_MEMBER_PLAIN(params, 1)
      GE_RTTI_MEMBER_PLAIN(samplers, 2)
      GE_RTTI_MEMBER_PLAIN(textures, 3)
      GE_RTTI_MEMBER_PLAIN(loadStoreTextures, 4)
      GE_RTTI_MEMBER_PLAIN(buffers, 5)
    GE_END_RTTI_MEMBERS

   public:
    const String&
    getRTTIName() override {
      static String name = "GPUParamDesc";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_GPUParamDesc;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      return ge_shared_ptr_new<GPUParamDesc>();
    }
  };

  class GE_CORE_EXPORT GPUProgramRTTI
    : public RTTIType<GPUProgram, IReflectable, GPUProgramRTTI>
  {
   private:
    GE_BEGIN_RTTI_MEMBERS
      GE_RTTI_MEMBER_PLAIN(m_type, 2)
      GE_RTTI_MEMBER_PLAIN(m_needsAdjacencyInfo, 3)
      GE_RTTI_MEMBER_PLAIN(m_entryPoint, 4)
      GE_RTTI_MEMBER_PLAIN(m_source, 6)
      GE_RTTI_MEMBER_PLAIN(m_language, 7)
    GE_END_RTTI_MEMBERS

   public:
    void
    onSerializationStarted(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      //Need to ensure the core thread object is initialized
      auto gpuProgram = static_cast<GPUProgram*>(obj);
      gpuProgram->blockUntilCoreInitialized();
    }

    void
    onDeserializationEnded(IReflectable* obj,
                           SerializationContext* /*context*/) override {
      auto gpuProgram = static_cast<GPUProgram*>(obj);
      gpuProgram->initialize();
    }

    const String&
    getRTTIName() override {
      static String name = "GPUProgram";
      return name;
    }

    uint32
    getRTTIId() override {
      return TYPEID_CORE::kID_GPUProgram;
    }

    SPtr<IReflectable>
    newRTTIObject() override {
      //Params don't matter, they'll get overwritten
      return GPUProgramManager::instance().createEmpty("", GPU_PROGRAM_TYPE::kVERTEX_PROGRAM);
    }
  };
}
