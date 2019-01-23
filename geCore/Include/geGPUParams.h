/*****************************************************************************/
/**
 * @file    geGPUParams.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/11
 * @brief   Contains descriptions for all parameters in a set of programs.
 *
 * Contains descriptions for all parameters in a set of programs (one for each
 * stage) and allows you to write and read those parameters. All parameter
 * values are stored internally on the CPU, and are only submitted to the GPU
 * once the parameters are bound to the pipeline.
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
#include "geGPUParam.h"
#include "geCoreObject.h"
#include "geIResourceListener.h"

namespace geEngineSDK {
  /**
   * @brief Helper structure whose specializations convert an engine data type
   *        into a GPU program data parameter type.
   */
  template<class T>
  struct TGPUDataParamInfo
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kSTRUCT };
  };

  template<>
  struct TGPUDataParamInfo<float>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kFLOAT1 };
  };

  template<>
  struct TGPUDataParamInfo<Vector2>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kFLOAT2 };
  };
  
  template<>
  struct TGPUDataParamInfo<Vector3>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kFLOAT3 };
  };
  
  template<>
  struct TGPUDataParamInfo<Vector4>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kFLOAT4 };
  };
  
  template<>
  struct TGPUDataParamInfo<int32>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kINT1 };
  };
  
  template<>
  struct TGPUDataParamInfo<Vector2I>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kINT2 };
  };
  
  template<>
  struct TGPUDataParamInfo<Vector3I>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kINT3 };
  };
  
  template<>
  struct TGPUDataParamInfo<Vector4I>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kINT4 };
  };
  
  template<>
  struct TGPUDataParamInfo<Matrix4>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kMATRIX_4X4 };
  };
  
  template<>
  struct TGPUDataParamInfo<LinearColor>
  {
    enum { kTypeId = GPU_PARAM_DATA_TYPE::kCOLOR };
  };

  class GPUPipelineParamInfoBase;

  /**
   * @brief Contains functionality common for both sim and core thread version
   *        of GPUParams.
   */
  class GE_CORE_EXPORT GPUParamsBase
  {
   public:
    virtual ~GPUParamsBase() = default;

    /**
     * @note  Disallow copy/assign because it would require some care when
     *        copying (copy internal data shared_ptr and all the internal
     *        buffers too).
     *        Trivial to implement but not needed at this time.
     *        Undelete and implement if necessary.
     */
    GPUParamsBase(const GPUParamsBase& other) = delete;
    GPUParamsBase&
    operator=(const GPUParamsBase& rhs) = delete;

    /**
     * @brief Returns a description of all stored parameters.
     */
    SPtr<GPUParamDesc>
    getParamDesc(GPU_PROGRAM_TYPE::E type) const;

    /**
     * @brief Gets the object that contains the processed information about
     *        all parameters.
     */
    SPtr<GPUPipelineParamInfoBase>
    getParamInfo() const {
      return m_paramInfo;
    }

    /**
     * @brief Returns the size of a data parameter with the specified name,
     *        in bytes. Returns 0 if such parameter doesn't exist.
     */
    uint32
    getDataParamSize(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if parameter with the specified name exists.
     */
    bool
    hasParam(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if texture parameter with the specified name exists.
     */
    bool
    hasTexture(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if load/store texture parameter with the specified name
     *        exists.
     */
    bool
    hasLoadStoreTexture(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if buffer parameter with the specified name exists.
     */
    bool
    hasBuffer(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if sampler state parameter with the specified name exists.
     */
    bool
    hasSamplerState(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Checks if a parameter block with the specified name exists.
     */
    bool
    hasParamBlock(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Gets a descriptor for a parameter block buffer with the specified
     *        name.
     */
    GPUParamBlockDesc*
    getParamBlockDesc(GPU_PROGRAM_TYPE::E type, const String& name) const;

    /**
     * @brief Marks the sim thread object as dirty, causing it to sync its
     *        contents with its core thread counterpart.
     */
    virtual void
    _markCoreDirty() {}

    /**
     * @copydoc IResourceListener::markListenerResourcesDirty
     */
    virtual void
    _markResourcesDirty() {}

   protected:
    GPUParamsBase(const SPtr<GPUPipelineParamInfoBase>& paramInfo);

    /**
     * @brief Gets a descriptor for a data parameter with the specified name.
     */
    GPUParamDataDesc*
    getParamDesc(GPU_PROGRAM_TYPE::E type, const String& name) const;

    SPtr<GPUPipelineParamInfoBase> m_paramInfo;
  };

  /**
   * @brief Template version of GPUParams that contains functionality for both
   *        sim and core thread versions of stored data.
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParams : public GPUParamsBase
  {
   public:
    using GPUParamsType = CoreVariantType<GPUParams, Core>;
    using TextureType = CoreVariantHandleType<Texture, Core>;
    using BufferType = SPtr<CoreVariantType<GPUBuffer, Core>>;
    using SamplerType = SPtr<CoreVariantType<SamplerState, Core>>;
    using ParamsBufferType = SPtr<CoreVariantType<GPUParamBlockBuffer, Core>>;

    virtual ~TGPUParams();

    /**
     * @brief Returns a handle for the parameter with the specified name.
     *        Handle may then be stored and used for quickly setting or
     *        retrieving values to/from that parameter.
     *
     * Throws exception if parameter with that name and type doesn't exist.
     *
     * Parameter handles will be invalidated when their parent GPUParams
     * object changes.
     */
    template<class T>
    void
    getParam(GPU_PROGRAM_TYPE::E type,
             const String& name,
             TGPUDataParam<T, Core>& output) const;

    /**
     * @copydoc getParam
     */
    void
    getStructParam(GPU_PROGRAM_TYPE::E type,
                   const String& name,
                   TGPUParamStruct<Core>& output) const;

    /**
     * @copydoc getParam
     */
    void
    getTextureParam(GPU_PROGRAM_TYPE::E type,
                    const String& name,
                    TGPUParamTexture<Core>& output) const;

    /**
     * @copydoc getParam
     */
    void
    getLoadStoreTextureParam(GPU_PROGRAM_TYPE::E type,
                             const String& name,
                             TGPUParamLoadStoreTexture<Core>& output) const;

    /**
     * @copydoc getParam
     */
    void
    getBufferParam(GPU_PROGRAM_TYPE::E type,
                   const String& name,
                   TGPUParamBuffer<Core>& output) const;

    /**
     * @copydoc getParam
     */
    void
    getSamplerStateParam(GPU_PROGRAM_TYPE::E type,
                         const String& name,
                         TGPUParamSampState<Core>& output) const;

    /**
     * @brief Gets a parameter block buffer from the specified set/slot
     *        combination.
     */
    ParamsBufferType
    getParamBlockBuffer(uint32 set, uint32 slot) const;

    /**
     * @brief Gets a texture bound to the specified set/slot combination.
     */
    TextureType
    getTexture(uint32 set, uint32 slot) const;

    /**
     * @brief Gets a load/store texture bound to the specified set/slot
     *        combination.
     */
    TextureType
    getLoadStoreTexture(uint32 set, uint32 slot) const;

    /**
     * @brief Gets a buffer bound to the specified set/slot combination.
     */
    BufferType
    getBuffer(uint32 set, uint32 slot) const;

    /**
     * @brief Gets a sampler state bound to the specified set/slot combination.
     */
    SamplerType
    getSamplerState(uint32 set, uint32 slot) const;

    /**
     * @brief Gets information that determines which texture surfaces to bind
     *        as a sampled texture parameter.
     */
    const TextureSurface&
    getTextureSurface(uint32 set, uint32 slot) const;

    /**
     * @brief Gets information that determines which texture surfaces to bind
     *        as a load/store parameter.
     */
    const TextureSurface&
    getLoadStoreSurface(uint32 set, uint32 slot) const;

    /**
     * @brief Assigns the provided parameter block buffer to a buffer with the
     *        specified name, for the specified GPU program stage. Any
     *        following parameter reads or writes that are referencing that
     *        buffer will use the new buffer.
     * @note  It is up to the caller to guarantee the provided buffer matches
     *        parameter block descriptor for this slot.
     */
    void
    setParamBlockBuffer(GPU_PROGRAM_TYPE::E type,
                        const String& name,
                        const ParamsBufferType& paramBlockBuffer);

    /**
     * @brief Assigns the provided parameter block buffer to a buffer with the
     *        specified name, for any stages that reference the buffer.
     *        Any following parameter reads or writes that are referencing
     *        that buffer will use the new buffer.
     * @note  It is up to the caller to guarantee the provided buffer matches
     *        parameter block descriptor for this slot.
     * @note  It is up to the caller that all stages using this buffer name
     *        refer to the same buffer type.
     */
    void
    setParamBlockBuffer(const String& name,
                        const ParamsBufferType& paramBlockBuffer);

    /**
     * @brief Sets the parameter buffer with the specified set/slot
     *        combination.Any following parameter reads or writes that are
     *        referencing that buffer will use the new buffer. Set/slot
     *        information for a specific buffer can be extracted from
     *        GPUProgram's GPUParamDesc structure.
     * @note  It is up to the caller to guarantee the provided buffer matches
     *        parameter block descriptor for this slot.
     */
    virtual void
    setParamBlockBuffer(uint32 set,
                        uint32 slot,
                        const ParamsBufferType& paramBlockBuffer);

    /**
     * @brief Sets a texture at the specified set/slot combination.
     */
    virtual void
    setTexture(uint32 set,
               uint32 slot,
               const TextureType& texture,
               const TextureSurface& surface = TextureSurface::COMPLETE);

    /**
     * @brief Sets a load/store texture at the specified set/slot combination.
     */
    virtual void
    setLoadStoreTexture(uint32 set,
                        uint32 slot,
                        const TextureType& texture,
                        const TextureSurface& surface);

    /**
     * @brief Sets a buffer at the specified set/slot combination.
     */
    virtual void
    setBuffer(uint32 set, uint32 slot, const BufferType& buffer);

    /**
     * @brief Sets a sampler state at the specified set/slot combination.
     */
    virtual void
    setSamplerState(uint32 set, uint32 slot, const SamplerType& sampler);

    /**
     * @brief Assigns a data value to the parameter with the specified name.
     */
    template<class T>
    void
    setParam(GPU_PROGRAM_TYPE::E type, const String& name, const T& value) {
      TGPUDataParam<T, Core> param;
      getParam(type, name, param);
      param.set(value);
    }

    /**
     * @brief Assigns a texture to the parameter with the specified name.
     */
    void
    setTexture(GPU_PROGRAM_TYPE::E type,
               const String& name,
               const TextureType& texture,
               const TextureSurface& surface = TextureSurface::COMPLETE) {
      TGPUParamTexture<Core> param;
      getTextureParam(type, name, param);
      param.set(texture, surface);
    }

    /**
     * @brief Assigns a load/store texture to the parameter with the specified
     *        name.
     */
    void
    setLoadStoreTexture(GPU_PROGRAM_TYPE::E type,
                        const String& name,
                        const TextureType& texture,
                        const TextureSurface& surface) {
      TGPUParamLoadStoreTexture<Core> param;
      getLoadStoreTextureParam(type, name, param);
      param.set(texture, surface);
    }

    /**
     * @brief Assigns a buffer to the parameter with the specified name.
     */
    void
    setBuffer(GPU_PROGRAM_TYPE::E type,
              const String& name,
              const BufferType& buffer) {
      TGPUParamBuffer<Core> param;
      getBufferParam(type, name, param);
      param.set(buffer);
    }

    /**
     * @brief Assigns a sampler state to the parameter with the specified name.
     */
    void
    setSamplerState(GPU_PROGRAM_TYPE::E type,
                    const String& name,
                    const SamplerType& sampler) {
      TGPUParamSampState<Core> param;
      getSamplerStateParam(type, name, param);
      param.set(sampler);
    }

   protected:
    TGPUParams(const SPtr<GPUPipelineParamInfoBase>& paramInfo);

    /**
     * @copydoc CoreObject::getThisPtr
     */
    virtual SPtr<GPUParamsType>
    _getThisPtr() const = 0;

    /**
     * @brief Data for a single bound texture.
     */
    struct TextureData
    {
      TextureType texture;
      TextureSurface surface;
    };

    ParamsBufferType* m_paramBlockBuffers = nullptr;
    TextureData* m_sampledTextureData = nullptr;
    TextureData* m_loadStoreTextureData = nullptr;
    BufferType* m_buffers = nullptr;
    SamplerType* m_samplerStates = nullptr;
  };

  /**
   * @brief Contains descriptions for all parameters in a set of programs
   *      (one for each stage) and allows you to write and read those
   *      parameters. All parameter values are stored internally on the CPU,
   *      and are only submitted to the GPU once the parameters are bound to
   *      the pipeline.
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT GPUParams
    : public CoreObject, public TGPUParams<false>, public IResourceListener
  {
   public:
    ~GPUParams() = default;

    /**
     * @brief Retrieves a core implementation of a mesh usable only from the
     *        core thread.
     */
    SPtr<geCoreThread::GPUParams>
    getCore() const;

    /**
     * @brief Creates new GPUParams object that can serve for changing the GPU
     *        program parameters on the specified pipeline.
     * @param[in] pipelineState Pipeline state for which this object can set
     *                          parameters for.
     * @return  New GPUParams object.
     */
    static SPtr<GPUParams>
    create(const SPtr<GraphicsPipelineState>& pipelineState);

    /**
     * @copydoc GPUParams::create(const SPtr<GraphicsPipelineState>&)
     */
    static SPtr<GPUParams>
    create(const SPtr<ComputePipelineState>& pipelineState);

    /**
     * @brief Creates a new set of GPU parameters using an object describing
     *        the parameters for a pipeline.
     * @param[in] paramInfo Description of GPU parameters for a specific GPU
     *                      pipeline state.
     */
    static SPtr<GPUParams>
    create(const SPtr<GPUPipelineParamInfo>& paramInfo);

    /**
     * @brief Contains a lookup table for sizes of all data parameters.
     *        Sizes are in bytes.
     */
    const static GPUDataParamInfos PARAM_SIZES;

    /**
     * @copydoc GPUParamsBase::_markCoreDirty
     */
    void
    _markCoreDirty() override;

    /**
     * @copydoc IResourceListener::markListenerResourcesDirty
     */
    void
    _markResourcesDirty() override;

   protected:
    friend class HardwareBufferManager;

    GPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo);

    /**
     * @copydoc CoreObject::getThisPtr
     */
    SPtr<GPUParams>
    _getThisPtr() const override;

    /**
     * @copydoc CoreObject::createCore
     */
    SPtr<geCoreThread::CoreObject>
    createCore() const override;

    /**
     * @copydoc CoreObject::syncToCore
     */
    CoreSyncData
    syncToCore(FrameAlloc* allocator) override;

    /**
     * @copydoc IResourceListener::getListenerResources
     */
    void
    getListenerResources(Vector<HResource>& resources) override;

    /**
     * @copydoc IResourceListener::notifyResourceLoaded
     */
    void
    notifyResourceLoaded(const HResource& /*resource*/) override {
      markCoreDirty();
    }

    /**
     * @copydoc IResourceListener::notifyResourceChanged
     */
    void
    notifyResourceChanged(const HResource& /*resource*/) override {
      markCoreDirty();
    }
  };

  namespace geCoreThread {
    /**
     * @brief Core thread version of geEngineSDK::GPUParams.
     * @note  Core thread only.
     */
    class GE_CORE_EXPORT GPUParams : public CoreObject, public TGPUParams<true>
    {
     public:
      virtual ~GPUParams() = default;

      /**
       * @copydoc geEngineSDK::GPUParams::
       *            create(const SPtr<GraphicsPipelineState>&)
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the buffer be created on.
       */
      static SPtr<GPUParams>
      create(const SPtr<GraphicsPipelineState>& pipelineState,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::GPUParams::
       *            create(const SPtr<ComputePipelineState>&)
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the buffer be created on.
       */
      static SPtr<GPUParams>
      create(const SPtr<ComputePipelineState>& pipelineState,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

      /**
       * @copydoc geEngineSDK::GPUParams::
       *            create(const SPtr<GPUPipelineParamInfo>&)
       * @param[in] deviceMask  Mask that determines on which GPU devices
       *                        should the buffer be created on.
       */
      static SPtr<GPUParams>
      create(const SPtr<GPUPipelineParamInfo>& paramInfo,
             GPU_DEVICE_FLAGS::E deviceMask = GPU_DEVICE_FLAGS::kDEFAULT);

     protected:
      friend class geEngineSDK::GPUParams;
      friend class HardwareBufferManager;

      GPUParams(const SPtr<GPUPipelineParamInfo>& paramInfo,
                GPU_DEVICE_FLAGS::E deviceMask);

      /**
       * @copydoc CoreObject::getThisPtr
       */
      SPtr<GPUParams>
      _getThisPtr() const override;

      /**
       * @copydoc CoreObject::syncToCore
       */
      void
      syncToCore(const CoreSyncData& data) override;
    };
  }
}
