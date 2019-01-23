/*****************************************************************************/
/**
 * @file    geGPUParam.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/11
 * @brief   A handle that allows you to set a GPUProgram parameter.
 *
 * A handle that allows you to set a GPUProgram parameter.
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

#include <geVector2.h>
#include <geVector3.h>
#include <geVector4.h>
#include <geMatrix4.h>
#include <geVectorNI.h>
#include <geColor.h>

namespace geEngineSDK {
  using std::nullptr_t;

  /**
   * @brief Policy class that allows us to re-use this template class for
   *        matrices which might need transposing, and other types which do
   *        not. Matrix needs to be transposed for certain render systems
   *        depending on how they store them in memory.
   */
  template<class Type>
  struct TransposePolicy
  {
    static Type
    transpose(const Type& value) {
      return value;
    }

    static bool
    transposeEnabled(bool /*enabled*/) {
      return false;
    }
  };

  /**
   * @brief Transpose policy for 4x4 matrix.
   */
  template<>
  struct TransposePolicy<Matrix4>
  {
    static Matrix4
    transpose(const Matrix4& value) {
      return value.getTransposed();
    }

    static bool
    transposeEnabled(bool enabled) {
      return enabled;
    }
  };

  /**
   * @brief A handle that allows you to set a GPUProgram parameter.
   *        Internally keeps a reference to the GPU parameter buffer and the
   *        necessary offsets. You should specialize this type for specific
   *        parameter types.
   *
   *        Object of this type must be returned by a Material. Setting/Getting
   *        parameter values will internally access a GPU parameter buffer
   *        attached to the Material this parameter was created from. Anything
   *        rendered with that material will then use those set values.
   *
   * @note  Normally you can set a GPUProgram parameter by calling various
   *        set/get methods on a Material. This class primarily used an as
   *        optimization in performance critical bits of code where it is
   *        important to locate and set parameters quickly without any lookups
   *        (Mentioned set/get methods expect a parameter name). You just
   *        retrieve the handle once and then set the parameter value many
   *        times with minimal performance impact.
   * @see   Material
   */
  template<class T, bool Core>
  class GE_CORE_EXPORT TGPUDataParam
  {
   private:
    using GPUParamBufferType = SPtr<CoreVariantType<GPUParamBlockBuffer, Core>>;
    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;

   public:
    TGPUDataParam();
    TGPUDataParam(GPUParamDataDesc* paramDesc, const GPUParamsType& parent);

    /**
     * @brief Sets a parameter value at the specified array index.
     *        If parameter does not contain an array leave the index at 0.
     * @note  Like with all GPU parameters, the actual GPU buffer will not be
     *        updated until rendering with material this parameter was created
     *        from starts on the core thread.
     */
    void
    set(const T& value, uint32 arrayIdx = 0) const;

    /**
     * @brief Returns a value of a parameter at the specified array index.
     *        If parameter does not contain an array leave the index at 0.
     * @note  No GPU reads are done. Data returned was cached when it was
     *        written.
     */
    T
    get(uint32 arrayIdx = 0) const;

    /**
     * @brief Returns meta-data about the parameter.
     */
    const GPUParamDataDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamDataDesc* m_paramDesc;
  };

  /**
   * @copydoc TGPUDataParam
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParamStruct
  {
   public:
    using GPUParamBufferType = SPtr<CoreVariantType<GPUParamBlockBuffer, Core>>;
    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;

    TGPUParamStruct();
    TGPUParamStruct(GPUParamDataDesc* paramDesc, const GPUParamsType& parent);

    /**
     * @copydoc TGPUDataParam::set
     */
    void
    set(const void* value, uint32 sizeBytes, uint32 arrayIdx = 0) const;

    /**
     * @copydoc TGPUDataParam::get
     */
    void
    get(void* value, uint32 sizeBytes, uint32 arrayIdx = 0) const;

    /**
     * @brief Returns the size of the struct in bytes.
     */
    uint32
    getElementSize() const;

    /**
     * @brief Returns meta-data about the parameter.
     */
    const GPUParamDataDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamDataDesc* m_paramDesc;
  };

  /**
   * @copydoc TGPUDataParam
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParamTexture
  {
   private:
    friend class GPUParams;
    friend class geCoreThread::GPUParams;

    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;
    using TextureType = CoreVariantHandleType<Texture, Core>;

   public:
    TGPUParamTexture();
    TGPUParamTexture(GPUParamObjectDesc* paramDesc,
                     const GPUParamsType& parent);

    /**
     * @copydoc TGPUDataParam::set
     */
    void
    set(const TextureType& texture,
        const TextureSurface& surface = TextureSurface::COMPLETE) const;

    /**
     * @copydoc TGPUDataParam::get
     */
    TextureType
    get() const;

    /**
     * @copydoc TGPUDataParam::getDesc
     */
    const GPUParamObjectDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamObjectDesc* m_paramDesc;
  };

  /**
   * @copydoc TGPUDataParam
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParamLoadStoreTexture
  {
   private:
    friend class GPUParams;
    friend class geCoreThread::GPUParams;

    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;
    using TextureType = CoreVariantHandleType<Texture, Core>;

   public:
    TGPUParamLoadStoreTexture();
    TGPUParamLoadStoreTexture(GPUParamObjectDesc* paramDesc,
                              const GPUParamsType& parent);

    /**
     * @copydoc TGPUDataParam::set
     */
    void
    set(const TextureType& texture,
        const TextureSurface& surface = TextureSurface()) const;

    /**
     * @copydoc TGPUDataParam::get
     */
    TextureType
    get() const;

    /**
     * @copydoc TGPUDataParam::getDesc
     */
    const GPUParamObjectDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamObjectDesc* m_paramDesc;
  };

  /**
   * @copydoc TGPUDataParam
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParamBuffer
  {
   private:
    friend class GPUParams;
    friend class geCoreThread::GPUParams;

    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;
    using BufferType = SPtr<CoreVariantType<GPUBuffer, Core>>;

   public:
    TGPUParamBuffer();
    TGPUParamBuffer(GPUParamObjectDesc* paramDesc, const GPUParamsType& parent);

    /**
     * @copydoc TGPUDataParam::set
     */
    void
    set(const BufferType& buffer) const;

    /**
     * @copydoc TGPUDataParam::get
     */
    BufferType
    get() const;

    /**
     * @copydoc TGPUDataParam::getDesc
     */
    const GPUParamObjectDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamObjectDesc* m_paramDesc;
  };

  /**
   * @copydoc TGPUDataParam
   */
  template<bool Core>
  class GE_CORE_EXPORT TGPUParamSampState
  {
   private:
    friend class GPUParams;
    friend class geCoreThread::GPUParams;

    using GPUParamsType = SPtr<CoreVariantType<GPUParams, Core>>;
    using SamplerStateType = SPtr<CoreVariantType<SamplerState, Core>>;

   public:
    TGPUParamSampState();
    TGPUParamSampState(GPUParamObjectDesc* paramDesc,
                       const GPUParamsType& parent);

    /**
     * @copydoc TGPUDataParam::set
     */
    void
    set(const SamplerStateType& samplerState) const;

    /**
     * @copydoc TGPUDataParam::get
     */
    SamplerStateType
    get() const;

    /**
     * @copydoc TGPUDataParam::getDesc
     */
    const GPUParamObjectDesc&
    getDesc() const {
      return *m_paramDesc;
    }

    /**
     * @brief Checks if param is initialized.
     */
    bool
    operator==(const nullptr_t&) const {
      return nullptr == m_paramDesc;
    }

   protected:
    GPUParamsType m_parent;
    GPUParamObjectDesc* m_paramDesc;
  };

  using GPUParamFloat = TGPUDataParam<float, false>;
  using GPUParamVec2  = TGPUDataParam<Vector2, false>;
  using GPUParamVec3  = TGPUDataParam<Vector3, false>;
  using GPUParamVec4  = TGPUDataParam<Vector4, false>;
  using GPUParamInt   = TGPUDataParam<int32, false>;
  using GPUParamVec2I = TGPUDataParam<Vector2I, false>;
  using GPUParamVec3I = TGPUDataParam<Vector3I, false>;
  using GPUParamVec4I = TGPUDataParam<Vector4I, false>;
  using GPUParamMat4  = TGPUDataParam<Matrix4, false>;
  using GPUParamColor = TGPUDataParam<LinearColor, false>;

  using GPUParamStruct            = TGPUParamStruct<false>;
  using GPUParamTexture           = TGPUParamTexture<false>;
  using GPUParamBuffer            = TGPUParamBuffer<false>;
  using GPUParamSampState         = TGPUParamSampState<false>;
  using GPUParamLoadStoreTexture  = TGPUParamLoadStoreTexture<false>;

  namespace geCoreThread {
    using GPUParamFloat = TGPUDataParam<float, true>;
    using GPUParamVec2  = TGPUDataParam<Vector2, true>;
    using GPUParamVec3  = TGPUDataParam<Vector3, true>;
    using GPUParamVec4  = TGPUDataParam<Vector4, true>;
    using GPUParamInt   = TGPUDataParam<int32, true>;
    using GPUParamVec2I = TGPUDataParam<Vector2I, true>;
    using GPUParamVec3I = TGPUDataParam<Vector3I, true>;
    using GPUParamVec4I = TGPUDataParam<Vector4I, true>;
    using GPUParamMat4  = TGPUDataParam<Matrix4, true>;
    using GPUParamColor = TGPUDataParam<LinearColor, true>;

    using GPUParamStruct            = TGPUParamStruct<true>;
    using GPUParamTexture           = TGPUParamTexture<true>;
    using GPUParamBuffer            = TGPUParamBuffer<true>;
    using GPUParamSampState         = TGPUParamSampState<true>;
    using GPUParamLoadStoreTexture  = TGPUParamLoadStoreTexture<true>;
  }
}
