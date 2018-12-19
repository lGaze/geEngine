/*****************************************************************************/
/**
 * @file    geProfilerGPU.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/12/18
 * @brief   Profiler that measures time and amount of various GPU operations.
 *
 * Profiler that measures time and amount of various GPU operations.
 *
 * @note    Core thread only except where noted otherwise.
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
#include "geRenderStats.h"

#include <geModule.h>
#include <gePoolAlloc.h>

namespace geEngineSDK {
  using std::move;

  /**
   * @brief Contains various profiler statistics about a single GPU profiling
   *        sample.
   */
  struct GPUProfileSample
  {
    /**
     * @brief Name of the sample for easier identification.
     */
    String name;

    /**
     * @brief Time in milliseconds it took to execute the sampled block.
     */
    float timeMs;

    /**
     * @brief Number of draw calls that happened.
     */
    uint32 numDrawCalls;

    /**
     * @brief How many times was render target changed.
     */
    uint32 numRenderTargetChanges;

    /**
     * @brief How many times did a buffer swap happen on a double buffered
     *        render target.
     */
    uint32 numPresents;

    /**
     * @brief How many times was render target cleared.
     */
    uint32 numClears;

    /**
     * @brief Total number of vertices sent to the GPU.
     */
    uint32 numVertices;

    /**
     * @brief Total number of primitives sent to the GPU.
     */
    uint32 numPrimitives;

    /**
     * @brief Number of samples drawn by the GPU.
     */
    uint32 numDrawnSamples;

    /**
     * @brief How many times did the pipeline state change.
     */
    uint32 numPipelineStateChanges;

    /**
     * @brief How many times were GPU parameters bound.
     */
    uint32 numGpuParamBinds;

    /**
     * @brief How many times was a vertex buffer bound.
     */
    uint32 numVertexBufferBinds;

    /**
     * @brief How many times was an index buffer bound.
     */
    uint32 numIndexBufferBinds;

    /**
     * @brief How many times were GPU resources written to.
     */
    uint32 numResourceWrites;

    /**
     * @brief How many times were GPU resources read from.
     */
    uint32 numResourceReads;

    /**
     * @brief How many GPU objects were created.
     */
    uint32 numObjectsCreated;

    /**
     * @brief How many GPU objects were destroyed.
     */
    uint32 numObjectsDestroyed;

    Vector<GPUProfileSample> children;
  };

  /**
   * @brief Profiler report containing information about GPU sampling data
   *        from a single frame.
   */
  struct GPUProfilerReport
  {
    /**
     * @brief Sample containing data for entire frame.
     */
    GPUProfileSample frameSample;
  };

  /**
   * @brief Profiler that measures time and amount of various GPU operations.
   * @note  Core thread only except where noted otherwise.
   */
  class GE_CORE_EXPORT ProfilerGPU : public Module<ProfilerGPU>
  {
   private:
    struct ProfiledSample
    {
      ProfilerString name;
      RenderStatsData startStats;
      RenderStatsData endStats;
      SPtr<geCoreThread::TimerQuery> activeTimeQuery;
      SPtr<geCoreThread::OcclusionQuery> activeOcclusionQuery;
      Vector<ProfiledSample*> children;
    };

   public:
    ProfilerGPU();
    ~ProfilerGPU();

    /**
     * @brief Signals a start of a new frame. Every frame will generate a
     *        separate profiling report. This call must be followed by
     *        endFrame(), and any sampling operations must happen between
     *        beginFrame() and endFrame().
     */
    void
    beginFrame();

    /**
     * @brief Signals an end of the currently sampled frame. Results of the
     *        sampling will be available once getNumAvailableReports
     *        increments. This may take a while as the sampling is scheduled on
     *        the core thread and on the GPU.
     */
    void
    endFrame();

    /**
     * @brief Begins sample measurement. Must be followed by endSample().
     * @param[in] name  Unique name for the sample you can later use to find
     *            the sampling data.
     * @note  Must be called between beginFrame()/endFrame() calls.
     */
    void
    beginSample(ProfilerString name);

    /**
     * @brief Ends sample measurement.
     * @param[in] name  Unique name for the sample.
     * @note  Unique name is primarily needed to more easily identify
     *        mismatched begin/end sample pairs. Otherwise the name in
     *        beginSample() would be enough. Must be called between
     *        beginFrame()/endFrame() calls.
     */
    void
    endSample(const ProfilerString& name);

    /**
     * @brief Returns number of profiling reports that are ready but haven't
     *        been retrieved yet.
     * @note  There is an internal limit of maximum number of available
     *        reports, where oldest ones will get deleted so make sure to call
     *        this often if you don't want to miss some.
     * @note  Thread safe.
     */
    uint32
    getNumAvailableReports();

    /**
     * @brief Gets the oldest report available and removes it from the internal
     *        list. Throws an exception if no reports are available.
     * @note  Thread safe.
     */
    GPUProfilerReport
    getNextReport();

   public:
    /**
     * @brief To be called once per frame from the Core thread.
     */
    void
    _update();

   private:
    /**
     * @brief Assigns start values for the provided sample.
     */
    void
    beginSampleInternal(ProfiledSample& sample, bool issueOcclusion);

    /**
     * @brief Assigns end values for the provided sample.
     */
    void
    endSampleInternal(ProfiledSample& sample);

    /**
     * @brief Creates a new timer query or returns an existing free query.
     */
    SPtr<geCoreThread::TimerQuery>
    getTimerQuery() const;

    /**
     * @brief Creates a new occlusion query or returns an existing free query.
     */
    SPtr<geCoreThread::OcclusionQuery>
    getOcclusionQuery() const;

    /**
     * @brief Frees the memory used by all the child samples.
     */
    void
    freeSample(ProfiledSample& sample);

    /**
     * @brief Resolves an active sample and converts it to report sample.
     */
    void
    resolveSample(const ProfiledSample& sample,
                  GPUProfileSample& reportSample);

   private:
    ProfiledSample m_frameSample;
    bool m_isFrameActive;
    Stack<ProfiledSample*> m_activeSamples;

    Queue<ProfiledSample> m_unresolvedFrames;
    GPUProfilerReport* m_readyReports;

    static const uint32 MAX_QUEUE_ELEMENTS;
    uint32 m_reportHeadPos;
    uint32 m_reportCount;

    PoolAlloc<sizeof(ProfiledSample), 256> m_samplePool;

    mutable Stack<SPtr<geCoreThread::TimerQuery>> m_freeTimerQueries;
    mutable Stack<SPtr<geCoreThread::OcclusionQuery>> m_freeOcclusionQueries;

    Mutex m_mutex;
  };

  /**
   * @brief Provides global access to ProfilerGPU instance.
   */
  GE_CORE_EXPORT ProfilerGPU&
  g_profilerGPU();

  /**
   * @brief Profiling macros that allow profiling functionality to be disabled
   *        at compile time.
   */
#if GE_PROFILING_ENABLED
# define GE_GPU_PROFILE_BEGIN(name) g_profilerGPU().beginSample(name);
# define GE_GPU_PROFILE_END(name) g_profilerGPU().endSample(name);
#else
# define GE_GPU_PROFILE_BEGIN(name)
# define GE_GPU_PROFILE_END(name)
#endif

  /**
   * @brief Helper class that performs GPU profiling in the current block.
   *        Profiling sample is started when the class is constructed and
   *        ended upon destruction.
   */
  struct ProfileGPUBlock
  {
#if GE_PROFILING_ENABLED
    ProfileGPUBlock(ProfilerString name) {
      m_sampleName = move(name);
      g_profilerGPU().beginSample(m_sampleName);
    }
#else
    ProfileGPUBlock(const ProfilerString& name) {}
#endif

#if GE_PROFILING_ENABLED
    ~ProfileGPUBlock() {
      g_profilerGPU().endSample(m_sampleName);
    }
#endif

   private:
#if GE_PROFILING_ENABLED
    ProfilerString m_sampleName;
#endif
  };
}
