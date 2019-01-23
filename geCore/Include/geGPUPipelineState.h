/*****************************************************************************/
/**
 * @file    geGPUPipelineState.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/10
 * @brief   State of the GPU pipeline that determines how are primitives drawn.
 *
 * Describes the state of the GPU pipeline that determines how are primitives
 * rendered. It consists of programmable states (vertex, fragment, geometry,
 * etc. GPU programs), as well as a set of fixed states (blend, rasterizer,
 * depth-stencil). Once created the state is immutable, and can be bound to
 * RenderAPI for rendering.
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
#include "geCoreObject.h"

namespace geEngineSDK {
  /**
   * @brief Descriptor structure used for initializing a GPU pipeline state.
   */
  struct PIPELINE_STATE_DESC
  {
    SPtr<BlendState> blendState;
    SPtr<RasterizerState> rasterizerState;
    SPtr<DepthStencilState> depthStencilState;

    SPtr<GPUProgram> vertexProgram;
    SPtr<GPUProgram> fragmentProgram;
    SPtr<GPUProgram> geometryProgram;
    SPtr<GPUProgram> hullProgram;
    SPtr<GPUProgram> domainProgram;
  };

  namespace geCoreThread {
    /**
     * @brief Descriptor structure used for initializing a GPU pipeline state.
     */
    struct PIPELINE_STATE_DESC
    {
      SPtr<BlendState> blendState;
      SPtr<RasterizerState> rasterizerState;
      SPtr<DepthStencilState> depthStencilState;

      SPtr<GPUProgram> vertexProgram;
      SPtr<GPUProgram> fragmentProgram;
      SPtr<GPUProgram> geometryProgram;
      SPtr<GPUProgram> hullProgram;
      SPtr<GPUProgram> domainProgram;
    };
  }

  /**
   * @brief Contains all data used by a GPU pipeline state, templated so it may
   *        contain both core and sim thread data.
   */
  template<bool Core>
  struct TGPUPipelineStateTypes
  {};

  template<>
  struct TGPUPipelineStateTypes<false>
  {
    using GPUPipelineParamInfoType = GPUPipelineParamInfo;
    using StateDescType = PIPELINE_STATE_DESC;
  };

  template<>
  struct TGPUPipelineStateTypes<true>
  {
    using GPUPipelineParamInfoType = geCoreThread::GPUPipelineParamInfo;
    using StateDescType = geCoreThread::PIPELINE_STATE_DESC;
  };

  /**
   * @brief Templated version of GraphicsPipelineState so it can be used for
   *        both core and non-core versions of the pipeline state.
   */
  template<bool Core>
  class GE_CORE_EXPORT TGraphicsPipelineState
  {
   public:
     using BlendStateType = SPtr<CoreVariantType<BlendState, Core>>;
     using RasterizerStateType = SPtr<CoreVariantType<RasterizerState, Core>>;
     using DepthStencilStateType = SPtr<CoreVariantType<DepthStencilState, Core>>;
     using GPUProgramType = SPtr<CoreVariantType<GPUProgram, Core>>;
     using StateDescType = typename TGPUPipelineStateTypes<Core>::StateDescType;
     using GPUPipelineParamInfoType = typename TGPUPipelineStateTypes<Core>
                                                 ::GPUPipelineParamInfoType;

    virtual ~TGraphicsPipelineState() = default;

    bool
    hasVertexProgram() const {
      return nullptr != m_data.vertexProgram;
    }

    bool
    hasFragmentProgram() const {
      return nullptr != m_data.fragmentProgram;
    }

    bool
    hasGeometryProgram() const {
      return nullptr != m_data.geometryProgram;
    }

    bool
    hasHullProgram() const {
      return nullptr != m_data.hullProgram;
    }

    bool
    hasDomainProgram() const {
      return nullptr != m_data.domainProgram;
    }

    BlendStateType
    getBlendState() const {
      return m_data.blendState;
    }

    RasterizerStateType
    getRasterizerState() const {
      return m_data.rasterizerState;
    }

    DepthStencilStateType
    getDepthStencilState() const {
      return m_data.depthStencilState;
    }

    const GPUProgramType&
    getVertexProgram() const {
      return m_data.vertexProgram;
    }

    const GPUProgramType&
    getFragmentProgram() const {
      return m_data.fragmentProgram;
    }

    const GPUProgramType&
    getGeometryProgram() const {
      return m_data.geometryProgram;
    }

    const GPUProgramType&
    getHullProgram() const {
      return m_data.hullProgram;
    }

    const GPUProgramType&
    getDomainProgram() const {
      return m_data.domainProgram;
    }

    /**
     * @brief Returns an object containing meta-data for parameters of all GPU
     *        programs used in this pipeline state.
     */
    const SPtr<GPUPipelineParamInfoType>&
    getParamInfo() const {
      return m_paramInfo;
    }

   protected:
    TGraphicsPipelineState();
    TGraphicsPipelineState(const StateDescType& desc);

    StateDescType m_data;
    SPtr<GPUPipelineParamInfoType> m_paramInfo;
  };

  /**
   * @brief Templated version of ComputePipelineState so it can be used for
   *        both core and non-core versions of the pipeline state.
   */
  template<bool Core>
  class GE_CORE_EXPORT TComputePipelineState
  {
   public:
    using GPUProgramType = SPtr<CoreVariantType<GPUProgram, Core>>;

    using GPUPipelineParamInfoType = typename TGPUPipelineStateTypes<Core>
                                                ::GPUPipelineParamInfoType;

    virtual ~TComputePipelineState() = default;

    const GPUProgramType&
    getProgram() const {
      return m_program;
    }

    /**
     * @brief Returns an object containing meta-data for parameters of the GPU
     *        program used in this pipeline state.
     */
    const SPtr<GPUPipelineParamInfoType>&
    getParamInfo() const {
      return m_paramInfo;
    }

   protected:
    TComputePipelineState();
    TComputePipelineState(const GPUProgramType& program);

    GPUProgramType m_program;
    SPtr<GPUPipelineParamInfoType> m_paramInfo;
  };

  /**
   * @brief Describes the state of the GPU pipeline that determines how are
   *        primitives rendered. It consists of programmable states (vertex,
   *        fragment, geometry, etc. GPU programs), as well as a set of fixed
   *        states (blend, rasterizer, depth-stencil). Once created the state
   *        is immutable, and can be bound to RenderAPI for rendering.
   */
  class GE_CORE_EXPORT GraphicsPipelineState
    : public CoreObject, public TGraphicsPipelineState<false>
  {
   public:
    virtual ~GraphicsPipelineState() = default;

    /**
     * @brief Retrieves a core implementation of the pipeline object usable
     *        only from the core thread.
     * @note  Core thread only.
     */
    SPtr<geCoreThread::GraphicsPipelineState>
    getCore() const;

    /**
     * @brief @copydoc RenderStateManager::createGraphicsPipelineState
     */
    static SPtr<GraphicsPipelineState>
    create(const PIPELINE_STATE_DESC& desc);

   protected:
    friend class RenderStateManager;

    GraphicsPipelineState(const PIPELINE_STATE_DESC& desc);

    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;
  };

  /**
   * @brief Describes the state of the GPU pipeline that determines how are
   *        compute programs executed. It consists of of a single programmable
   *        state (GPU program). Once created the state is immutable, and can
   *        be bound to RenderAPI for use.
   */
  class GE_CORE_EXPORT ComputePipelineState
    : public CoreObject, public TComputePipelineState<false>
  {
   public:
    virtual ~ComputePipelineState() = default;

    /**
     * @brief Retrieves a core implementation of the pipeline object usable
     *        only from the core thread.
     * @note  Core thread only.
     */
    SPtr<geCoreThread::ComputePipelineState>
    getCore() const;

    /**
     * @copydoc RenderStateManager::createComputePipelineState
     */
    static SPtr<ComputePipelineState>
    create(const SPtr<GPUProgram>& program);

   protected:
    friend class RenderStateManager;

    ComputePipelineState(const SPtr<GPUProgram>& program);

    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;
  };

  namespace geCoreThread {
    /**
     * @brief Core thread version of a bs::GraphicsPipelineState.
     */
    class GE_CORE_EXPORT GraphicsPipelineState
      : public CoreObject, public TGraphicsPipelineState<true>
    {
     public:
      GraphicsPipelineState(const PIPELINE_STATE_DESC& desc,
                            GPU_DEVICE_FLAGS::E deviceMask);
      virtual ~GraphicsPipelineState() = default;

      /**
       * @copydoc CoreObject::initialize()
       */
      void
      initialize() override;

      /**
       * @copydoc RenderStateManager::createGraphicsPipelineState
       */
      static SPtr<GraphicsPipelineState>
      create(const PIPELINE_STATE_DESC& desc,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      GPU_DEVICE_FLAGS::E m_deviceMask;
    };

    /**
     * @brief Core thread version of a bs::ComputePipelineState.
     */
    class GE_CORE_EXPORT ComputePipelineState
      : public CoreObject, public TComputePipelineState<true>
    {
     public:
      ComputePipelineState(const SPtr<GPUProgram>& program,
                           GPU_DEVICE_FLAGS::E deviceMask);
      virtual ~ComputePipelineState() = default;

      /**
       * @copydoc CoreObject::initialize()
       */
      void
      initialize() override;

      /**
       * @copydoc RenderStateManager::createComputePipelineState
       */
      static SPtr<ComputePipelineState>
      create(const SPtr<GPUProgram>& program,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      GPU_DEVICE_FLAGS::E m_deviceMask;
    };
  }
}
