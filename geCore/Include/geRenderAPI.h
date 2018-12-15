/*****************************************************************************/
/**
 * @file    geRenderAPI.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/22
 * @brief   Provides access to geCoreThread::RenderAPI from the sim thread.
 *
 * Provides access to geCoreThread::RenderAPI from the simulation thread.
 * All the commands get queued on the core thread queue for the calling thread.
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
#include "geCommandQueue.h"
#include "geSamplerState.h"
#include "geRenderAPICapabilities.h"
#include "geRenderTarget.h"
#include "geRenderTexture.h"
#include "geRenderWindow.h"
#include "geGPUProgram.h"
#include "geVertexDeclaration.h"

#include <gePlane.h>
#include <geModule.h>
#include <geEvent.h>

namespace geEngineSDK {
  class RenderAPIInfo;
  class RenderAPIManager;

  /**
   * @brief Provides access to geCoreThread::RenderAPI from the simulation
   *        thread. All the commands get queued on the core thread queue for
   *        the calling thread.
   * @see   geCoreThread::RenderAPI
   * @note  Sim thread only.
   */
  class GE_CORE_EXPORT RenderAPI
  {
   public:
    /**
     * @see geCoreThread::RenderAPI::setGPUParams()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setGPUParams(const SPtr<GPUParams>& gpuParams);

    /**
     * @see geCoreThread::RenderAPI::setGraphicsPipeline()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setGraphicsPipeline(const SPtr<GraphicsPipelineState>& pipelineState);

    /**
     * @see geCoreThread::RenderAPI::setComputePipeline()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setComputePipeline(const SPtr<ComputePipelineState>& pipelineState);

    /**
     * @see geCoreThread::RenderAPI::setVertexBuffers()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setVertexBuffers(uint32 index, const Vector<SPtr<VertexBuffer>>& buffers);

    /**
     * @see geCoreThread::RenderAPI::setIndexBuffer()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setIndexBuffer(const SPtr<IndexBuffer>& buffer);

    /**
     * @see geCoreThread::RenderAPI::setVertexDeclaration()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setVertexDeclaration(const SPtr<VertexDeclaration>& vertexDeclaration);

    /**
     * @see geCoreThread::RenderAPI::setViewport()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setViewport(const Box2D& area);

    /**
     * @see geCoreThread::RenderAPI::setStencilRef()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setStencilRef(uint32 value);

    /**
     * @see geCoreThread::RenderAPI::setDrawOperation()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setDrawOperation(DrawOperationType op);

    /**
     * @see geCoreThread::RenderAPI::setScissorRect()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setScissorRect(uint32 left = 0,
                   uint32 top = 0,
                   uint32 right = 800,
                   uint32 bottom = 600);

    /**
     * @see geCoreThread::RenderAPI::setRenderTarget()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    setRenderTarget(const SPtr<RenderTarget>& target,
                    uint32 readOnlyFlags = 0,
                    RenderSurfaceMask loadMask = RENDER_SURFACE_MASK_BITS::kNONE);

    /**
     * @see geCoreThread::RenderAPI::clearRenderTarget()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    clearRenderTarget(uint32 buffers,
                      const LinearColor& color = LinearColor::Black,
                      float depth = 1.0f,
                      uint16 stencil = 0,
                      uint8 targetMask = 0xFF);

    /**
     * @see geCoreThread::RenderAPI::clearViewport()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    clearViewport(uint32 buffers,
                  const LinearColor& color = LinearColor::Black,
                  float depth = 1.0f,
                  uint16 stencil = 0,
                  uint8 targetMask = 0xFF);

    /**
     * @see geCoreThread::RenderAPI::swapBuffers()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    swapBuffers(const SPtr<RenderTarget>& target);

    /**
     * @see geCoreThread::RenderAPI::draw()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    draw(uint32 vertexOffset,
         uint32 vertexCount,
         uint32 instanceCount = 0);

    /**
     * @see geCoreThread::RenderAPI::drawIndexed()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    drawIndexed(uint32 startIndex,
                uint32 indexCount,
                uint32 vertexOffset,
                uint32 vertexCount,
                uint32 instanceCount = 0);

    /**
     * @see geCoreThread::RenderAPI::dispatchCompute()
     * @note This is an @ref asyncMethod "asynchronous method".
     */
    static void
    dispatchCompute(uint32 numGroupsX,
                    uint32 numGroupsY = 1,
                    uint32 numGroupsZ = 1);

    /**
     * @copydoc geCoreThread::RenderAPI::getVideoModeInfo
     */
    static const VideoModeInfo&
    getVideoModeInfo();

    /**
     * @copydoc geCoreThread::RenderAPI::convertProjectionMatrix
     */
    static void
    convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest);

    /**
     * @copydoc geCoreThread::RenderAPI::getAPIInfo
     */
    static const RenderAPIInfo&
    getAPIInfo();
  };

  /**
   * @brief Feature flags that describe which render API specific features are
   *        enabled.
   */
  namespace RENDER_API_FEATURE_FLAG {
    enum E {
      /**
       * If set, vertex color order will be reversed before being sent to the
       * shader.
       */
      kVertexColorFlip = 1 << 0,
      
      /**
       * If set, the Y axis in texture (UV) coordinates is assumed to be
       * pointing up, instead of down (which is the default).
       */
      kUVYAxisUp = 1 << 1,

      /**
       * If set, the Y axis in normalized device coordinates (NDC) is assumed
       * to be pointing down, instead of up (which is the default).
       */
      kNDCYAxisDown = 1 << 2,

      /**
       * If set, the matrices used by shaders are in column major order,
       * instead of in row major (which is the default).
       */
      kColumnMajorMatrices = 1 << 3,

      /**
       * If set, the render API has native support for multi-threaded command
       * buffer generation. Otherwise it is emulated and using command buffers
       * might not be beneficial.
       */
      kMultiThreadedCB = 1 << 4,

      /**
       * If set, the render API supports unordered stores to a texture with
       * more than one sample.
       */
      kMSAAImageStores = 1 << 5,

      /**
       * If set, the render API supports binds of parts of a texture (e.g. a
       * single mip level, or a single element of a texture array). This only
       * applies to texture reads and individual mip levels or layers can still
       * be bound as render texture targets regardless of this flag.
       */
      kTextureViews = 1 << 6,

      /**
       * If set, the render API supports compute shaders.
       */
      kCompute = 1 << 7,

      /**
       * If set, the render API supports load-store textures or buffers
       * (AKA unordered access (UAV).
       */
      kLoadStore = 1 << 8,

      /**
       * If set, GPUPrograms can cache shader byte code for faster future
       * compilation.
       */
      kByteCodeCaching = 1 << 9,

      /**
       * If set, the render API support rendering to multiple layers of a
       * render texture at once (via a geometry shader).
       */
      kRenderTargetLayers = 1 << 10,
    };
  }

  using RenderAPIFeatures = Flags<RENDER_API_FEATURE_FLAG::E>;
  GE_FLAGS_OPERATORS(RENDER_API_FEATURE_FLAG::E)

  /**
   * @brief Contains properties specific to a render API implementation.
   */
  class RenderAPIInfo
  {
   public:
    RenderAPIInfo(float horzTexelOffset,
                  float vertTexelOffset,
                  float minDepth,
                  float maxDepth,
                  VERTEX_ELEMENT_TYPE::E vertexColorType,
                  RenderAPIFeatures featureFlags)
      : m_horizontalTexelOffset(horzTexelOffset),
        m_verticalTexelOffset(vertTexelOffset),
        m_minDepth(minDepth),
        m_maxDepth(maxDepth),
        m_vertexColorType(vertexColorType),
        m_featureFlags(featureFlags)
    {}

    /**
     * @brief Gets the native type used for vertex colors.
     */
    VERTEX_ELEMENT_TYPE::E
    getColorVertexElementType() const {
      return m_vertexColorType;
    }

    /**
     * @brief Gets horizontal texel offset used for mapping texels to pixels in
     * this render system.
     */
    float
    getHorizontalTexelOffset() const {
      return m_horizontalTexelOffset;
    }

    /**
     * @brief Gets vertical texel offset used for mapping texels to pixels in
     *        this render system.
     */
    float
    getVerticalTexelOffset() const {
      return m_verticalTexelOffset;
    }

    /**
     * @brief Gets the minimum (closest) depth value used by this render
     *        system.
     */
    float
    getMinimumDepthInputValue() const {
      return m_minDepth;
    }

    /**
     * @brief Gets the maximum (farthest) depth value used by this render
     *        system.
     */
    float
    getMaximumDepthInputValue() const {
      return m_maxDepth;
    }

    /**
     * @brief Checks is a specific feature flag enabled.
     */
    bool
    isFlagSet(RENDER_API_FEATURE_FLAG::E flag) const {
      return m_featureFlags.isSet(flag);
    }

   private:
    float m_horizontalTexelOffset = 0.0f;
    float m_verticalTexelOffset = 0.0f;
    float m_minDepth = 0.0f;
    float m_maxDepth = 1.0f;
    VERTEX_ELEMENT_TYPE::E m_vertexColorType = VERTEX_ELEMENT_TYPE::kCOLOR_ABGR;
    RenderAPIFeatures m_featureFlags;
  };

  namespace geCoreThread {
    /**
     * @brief Provides low-level API access to rendering commands
     *        (internally wrapping DirectX/OpenGL/Vulkan or similar).
     * Methods that accept a CommandBuffer parameter get queued in the provided
     * command buffer, and don't get executed until executeCommands() method is
     * called. User is allowed to populate command buffers from non-core
     * threads, but they all must get executed from the core thread.
     * If a command buffer is not provided to such methods, they execute
     * immediately. Without a command buffer the methods are only allowed to be
     * called from the core thread.
     * @note  Accessible on any thread for methods accepting a CommandBuffer.
     *        Otherwise core thread unless specifically noted otherwise on
     *        per-method basis.
     */
    class GE_CORE_EXPORT RenderAPI : public Module<RenderAPI>
    {
     public:
      RenderAPI();
      virtual ~RenderAPI();

      /**
       * @brief Returns the name of the rendering system.
       * @note  Thread safe.
       */
      virtual const StringID&
      getName() const = 0;

      /**
       * @brief Applies a set of parameters that control execution of all
       *        currently bound GPU programs. These are the uniforms like
       *        textures, samplers, or uniform buffers. Caller is expected to
       *        ensure the provided parameters actually match the currently
       *        bound programs.
       */
      virtual void
      setGPUParams(const SPtr<GPUParams>& gpuParams,
                   const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets a pipeline state that controls how will subsequent draw
       *        commands render primitives.
       * @param[in] pipelineState   Pipeline state to bind, or null to unbind.
       * @param[in] commandBuffer   Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called. Buffer must support graphics
       *            operations.
       * @see GraphicsPipelineState
       */
      virtual void
      setGraphicsPipeline(const SPtr<GraphicsPipelineState>& pipelineState,
                          const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets a pipeline state that controls how will subsequent
       *        dispatch commands execute.
       * @param[in] pipelineState   Pipeline state to bind, or null to unbind.
       * @param[in] commandBuffer   Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called.
       *            Buffer must support graphics operations.
       */
      virtual void
      setComputePipeline(const SPtr<ComputePipelineState>& pipelineState,
                         const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * Sets the active viewport that will be used for all render operations.
       * @param[in] area          Area of the viewport, in normalized
       *                          ([0,1] range) coordinates.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      setViewport(const Box2D& area,
                  const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Allows you to set up a region in which rendering can take
       *        place. Coordinates are in pixels. No rendering will be done to
       *        render target pixels outside of the provided region.
       * @param[in] left      Left border of the scissor rectangle, in pixels.
       * @param[in] top       Top border of the scissor rectangle, in pixels.
       * @param[in] right     Right border of the scissor rectangle, in pixels.
       * @param[in] bottom    Bottom border of the scissor rectangle, in pixels.
       * @param[in] commandBuffer Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called.
       *            Buffer must support graphics operations.
       */
      virtual void
      setScissorRect(uint32 left,
                     uint32 top,
                     uint32 right,
                     uint32 bottom,
                     const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets a reference value that will be used for stencil compare
       *        operations.
       * @param[in] value         Reference value to set.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      setStencilRef(uint32 value,
                    const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets the provided vertex buffers starting at the specified
       *        source index. Set buffer to nullptr to clear the buffer at the
       *        specified index.
       * @param[in] index         Index at which to start binding the vertex
       *                          buffers.
       * @param[in] buffers       A list of buffers to bind to the pipeline.
       * @param[in] numBuffers    Number of buffers in the @p buffers list.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      setVertexBuffers(uint32 index,
                       SPtr<VertexBuffer>* buffers,
                       uint32 numBuffers,
                       const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets an index buffer to use when drawing. Indices in an index
       *        buffer reference vertices in the vertex buffer, which increases
       *        cache coherency and reduces the size of vertex buffers by
       *        eliminating duplicate data.
       * @param[in] buffer        Index buffer to bind, null to unbind.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      setIndexBuffer(const SPtr<IndexBuffer>& buffer,
                     const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets the vertex declaration to use when drawing. Vertex
       *        declaration is used to decode contents of a single vertex in a
       *        vertex buffer.
       * @param[in] vertexDeclaration Vertex declaration to bind.
       * @param[in] commandBuffer Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called.
       *            Buffer must support graphics operations.
       */
      virtual void
      setVertexDeclaration(const SPtr<VertexDeclaration>& vertexDeclaration,
                           const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Sets the draw operation that determines how to interpret the
       *        elements of the index or vertex buffers.
       * @param[in] op            Draw operation to enable.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      setDrawOperation(DrawOperationType op,
                       const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Draw an object based on currently bound GPU programs, vertex
       *        declaration and vertex buffers. Draws directly from the vertex
       *        buffer without using indices.
       * @param[in] vertexOffset  Offset into the currently bound vertex buffer
       *                          to start drawing from.
       * @param[in] vertexCount   Number of vertices to draw.
       * @param[in] instanceCount Number of times to draw the provided
       *                          geometry, each time with an (optionally)
       *                          separate per-instance data.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      draw(uint32 vertexOffset,
           uint32 vertexCount,
           uint32 instanceCount = 0,
           const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Draw an object based on currently bound GPU programs, vertex
       *        declaration, vertex and index buffers.
       * @param[in] startIndex    Offset into the currently bound index buffer
       *                          to start drawing from.
       * @param[in] indexCount    Number of indices to draw.
       * @param[in] vertexOffset  Offset to apply to each vertex index.
       * @param[in] vertexCount   Number of vertices to draw.
       * @param[in] instanceCount Number of times to draw the provided
       *                          geometry, each time with an (optionally)
       *                          separate per-instance data.
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      drawIndexed(uint32 startIndex,
                  uint32 indexCount,
                  uint32 vertexOffset,
                  uint32 vertexCount,
                  uint32 instanceCount = 0,
                  const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Executes the currently bound compute shader.
       * @param[in] numGroupsX    Number of groups to start in the X direction.
       *                          Must be in range [1, 65535].
       * @param[in] numGroupsY    Number of groups to start in the Y direction.
       *                          Must be in range [1, 65535].
       * @param[in] numGroupsZ    Number of groups to start in the Z direction.
       *                          Must be in range [1, 64].
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support compute or graphics
       *                          operations.
       */
      virtual void
      dispatchCompute(uint32 numGroupsX,
                      uint32 numGroupsY = 1,
                      uint32 numGroupsZ = 1,
                      const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Swap the front and back buffer of the specified render target.
       * @param[in] target    Render target to perform the buffer swap on.
       * @param[in] syncMask  Optional synchronization mask that determines for
       *                      which queues should the system wait before
       *                      performing the swap buffer operation. By default
       *                      the system waits for all queues. However if
       *                      certain queues are performing non-rendering
       *                      operations, or operations not related to the
       *                      provided render target, you can exclude them from
       *                      the sync mask for potentially better performance.
       *                      You can use CommandSyncMask to generate a valid
       *                      sync mask.
       */
      virtual void
      swapBuffers(const SPtr<RenderTarget>& target,
                  uint32 syncMask = 0xFFFFFFFF) = 0;

      /**
       * @brief Change the render target into which we want to draw.
       * @param[in] target          Render target to draw to.
       * @param[in] readOnlyFlags   Combination of one or more elements of
       *            FRAME_BUFFER_TYPE denoting which buffers will be bound for
       *            read-only operations. This is useful for depth or stencil
       *            buffers which need to be bound both for depth/stencil
       *            tests, as well as shader reads.
       * @param[in] loadMask        Determines which render target surfaces
       *            will have their current contents preserved. By default when
       *            a render target is bound its contents will be lost. You
       *            might need to preserve contents if you need to perform
       *            blending or similar operations with the existing contents
       *            of the render target.
       *
       *            Use the mask to select exactly which surfaces of the render
       *            target need their contents preserved.
       * @param[in] commandBuffer   Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called.
       *            Buffer must support graphics operations.
       */
      virtual void
      setRenderTarget(const SPtr<RenderTarget>& target,
                      uint32 readOnlyFlags = 0,
                      RenderSurfaceMask loadMask = RENDER_SURFACE_MASK_BITS::kNONE,
                      const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Clears the currently active render target.
       * @param[in] buffers     Combination of one or more elements of
       *                        FRAME_BUFFER_TYPE denoting which buffers are to
       *                        be cleared.
       * @param[in] color       Color to clear the color buffer with,
       *                        if enabled.
       * @param[in] depth       Value to initialize the depth buffer with,
       *                        if enabled.
       * @param[in] stencil     Value to initialize the stencil buffer with,
       *                        if enabled.
       * @param[in] targetMask  In case multiple render targets are bound, this
       *            allows you to control which ones to clear (0x01 first,
       *            0x02 second, 0x04 third, etc., and combinations).
       * @param[in] commandBuffer Optional command buffer to queue the
       *            operation on. If not provided operation is executed
       *            immediately. Otherwise it is executed when
       *            executeCommands() is called.
       *            Buffer must support graphics operations.
       */
      virtual void
      clearRenderTarget(uint32 buffers,
                        const LinearColor& color = LinearColor::Black,
                        float depth = 1.0f,
                        uint16 stencil = 0,
                        uint8 targetMask = 0xFF,
                        const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Clears the currently active viewport (meaning it clears just a
       *        sub-area of a render-target that is covered by the viewport, as
       *        opposed to clearRenderTarget() which always clears the entire
       *        render target).
       * @param[in] buffers       Combination of one or more elements of
       *                          FRAME_BUFFER_TYPE denoting which buffers are
       *                          to be cleared.
       * @param[in] color         Color to clear the color buffer with,
       *                          if enabled.
       * @param[in] depth         Value to initialize the depth buffer with,
       *                          if enabled.
       * @param[in] stencil       Value to initialize the stencil buffer with,
       *                          if enabled.
       * @param[in] targetMask    In case multiple render targets are bound,
       *                          this allows you to control which ones to
       *                          clear (0x01 first, 0x02 second, 0x04 third,
       *                          etc.,and combinations).
       * @param[in] commandBuffer Optional command buffer to queue the
       *                          operation on. If not provided operation is
       *                          executed immediately. Otherwise it is
       *                          executed when executeCommands() is called.
       *                          Buffer must support graphics operations.
       */
      virtual void
      clearViewport(uint32 buffers,
                    const LinearColor& color = LinearColor::Black,
                    float depth = 1.0f,
                    uint16 stencil = 0,
                    uint8 targetMask = 0xFF,
                    const SPtr<CommandBuffer>& commandBuffer = nullptr) = 0;

      /**
       * @brief Appends all commands from the provided secondary command buffer
       *        into the primary command buffer.
       */
      virtual void
      addCommands(const SPtr<CommandBuffer>& commandBuffer,
                  const SPtr<CommandBuffer>& secondary) = 0;

      /**
       * @brief Executes all commands in the provided command buffer.
       *        Command buffer cannot be secondary.
       * @param[in] commandBuffer Command buffer whose commands to execute.
       *            Set to null to submit the main command buffer.
       * @param[in] syncMask Optional synchronization mask that determines if
       *            the submitted command buffer depends on any other command
       *            buffers. Use the CommandSyncMask class to generate a mask
       *            using existing command buffers. This mask is only relevant
       *            if your command buffers are executing on different hardware
       *            queues, and are somehow dependant. If they are executing on
       *            the same queue (default) then they will execute
       *            sequentially in the order they are submitted. Otherwise, if
       *            there is a dependency, you must make state it explicitly
       *            here.
       * @note  Core thread only.
       */
      virtual void
      submitCommandBuffer(const SPtr<CommandBuffer>& commandBuffer,
                          uint32 syncMask = 0xFFFFFFFF) = 0;

      /**
       * @brief Gets the capabilities of a specific GPU.
       * @param[in] deviceIdx Index of the device to get the capabilities for.
       * @note  Thread safe.
       */
      const RenderAPICapabilities&
      getCapabilities(uint32 deviceIdx) const;

      /**
       * @brief Returns the number of devices supported by this render API.
       */
      uint32
      getNumDevices() const {
        return m_numDevices;
      }

      /**
       * @brief Returns information about available output devices and their
       *        video modes.
       * @note  Thread safe.
       */
      const VideoModeInfo&
      getVideoModeInfo() const {
        return *m_videoModeInfo;
      }

      /***********************************************************************/
      /**
       * Utility Methods
       */
      /***********************************************************************/

      /**
       * @brief Converts a default matrix into a matrix suitable for use by
       *        this specific render system.
       * @note  Thread safe.
       */
      virtual void
      convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest) = 0;

      /**
       * @brief Returns information about the specific API implementation.
       * @note  Thread safe.
       */
      virtual const RenderAPIInfo&
      getAPIInfo() const = 0;

      /**
       * @brief Generates a parameter block description and calculates
       *        per-parameter offsets for the provided GPU data parameters.
       *        The offsets are render API specific and correspond to std140
       *        layout for OpenGL, and the default layout in DirectX.
       * @param[in]	name    Name to assign the parameter block.
       * @param[in]	params  List of parameters in the parameter block. Only
       *            name, type and array size fields need to be populated, the
       *            rest will be populated when the method returns. If a
       *            parameter is a struct then the elementSize field needs to
       *            be populated with the size of the struct in bytes.
       * @return  Descriptor for the parameter block holding the provided
       *          parameters as laid out by the default render API layout.
       */
      virtual GPUParamBlockDesc
      generateParamBlockDesc(const String& name,
                             Vector<GPUParamDataDesc>& params) = 0;

      /***********************************************************************/
      /**
       * Internal Methods
       */
      /***********************************************************************/

     protected:
      /**
       * @brief Initializes the render API system and creates a primary render
       *        window.
       * @note  Sim thread only.
       */
      SPtr<geEngineSDK::RenderWindow>
      initialize(const RENDER_WINDOW_DESC& primaryWindowDesc);

      /**
       * @brief Initializes the render API system. Called before the primary
       *        render window is created.
       */
      virtual void
      initialize();

      /**
       * @brief Performs (optional) secondary initialization of the render API
       *        system. Called after the render window is created.
       */
      virtual void
      initializeWithWindow(const SPtr<RenderWindow>& primaryWindow);

      /**
       * @brief Shuts down the render API system and cleans up all resources.
       * @note  Sim thread.
       */
      void
      destroy();

      /**
       * @brief Performs render API system shutdown on the core thread.
       */
      virtual void
      destroyCore();

      /**
       * @brief Converts the number of vertices to number of primitives based
       *        on the specified draw operation.
       */
      uint32
      vertexCountToPrimCount(DrawOperationType type, uint32 elementCount);

      /***********************************************************************/
      /**
       * Internal Data
       */
      /***********************************************************************/
     protected:
      friend class geEngineSDK::RenderAPIManager;

      SPtr<RenderTarget> m_activeRenderTarget;

      RenderAPICapabilities* m_currentCapabilities;
      uint32 m_numDevices;
      SPtr<VideoModeInfo> m_videoModeInfo;
    };
  }
}
