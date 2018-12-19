/*****************************************************************************/
/**
 * @file    geProfilerCPU.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/27
 * @brief   Provides various performance measuring methods.
 *
 * Provides various performance measuring methods.
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

#include <geModule.h>

namespace geEngineSDK {
  using std::chrono::high_resolution_clock;

  class CPUProfilerReport;

  /**
  * @brief CPU sampling type.
  */
  namespace ACTIVE_SAMPLING_TYPE {
    enum E {
      /**
      * Sample using milliseconds.
      */
      kBasic,

      /**
      * Sample using CPU cycles.
      */
      kPrecise
    };
  }

  /**
   * @brief Provides various performance measuring methods.
   * @note  Thread safe. Matching begin* / end* calls must belong to the same
   *        thread though.
   */
  class GE_CORE_EXPORT ProfilerCPU : public Module<ProfilerCPU>
  {
    /**
     * @brief Timer class responsible for tracking elapsed time.
     */
    class ProfilerTimer
    {
     public:
      ProfilerTimer();

      /**
       * @brief Sets the start time for the timer.
       */
      void
      start();

      /**
       * @brief Stops the timer and calculates the elapsed time from start time
       *        to now.
       */
      void
      stop();

      /**
       * @brief Resets the elapsed time to zero.
       */
      void
      reset();

      double m_time;
     private:
      double m_startTime;
      high_resolution_clock m_hrClock;

      /**
       * @brief Returns time elapsed since CPU was started in milliseconds.
       */
      inline double
      getCurrentTime() const;
    };

    /**
     * @brief Timer class responsible for tracking number of elapsed CPU
     *        cycles.
     */
    class ProfilerTimerPrecise
    {
     public:
      ProfilerTimerPrecise() = default;

      /**
       * @brief Starts the counter marking the current number of executed CPU
       *        cycles since CPU was started.
       */
      void
      start();

      /**
       * @brief Ends the counter and calculates the number of CPU cycles
       *        between now and the start time.
       */
      void
      stop();

      /**
       * @brief Resets the cycle count to zero.
       */
      void
      reset();

      uint64 m_cycles = 0;
     private:
      uint64 m_startCycles;

      /**
       * @brief Queries the CPU for the current number of CPU cycles executed
       *        since the program was started.
       */
      static inline uint64
      getNumCycles();
    };

    /**
     * @brief Contains data about a single profiler sample
     *        (counting time in milliseconds).
     * @note  A sample is created whenever a named profile block is entered.
     *        For example if you have a function you are profiling, and it gets
     *        called 10 times, there will be 10 samples.
     */
    struct ProfileSample
    {
      ProfileSample(double _time, uint64 _numAllocs, uint64 _numFrees)
        : time(_time),
          numAllocs(_numAllocs),
          numFrees(_numFrees)
      {}

      double time;
      uint64 numAllocs;
      uint64 numFrees;
    };

    /**
     * @brief Contains data about a single precise profiler sample
     *        (counting CPU cycles).
     * @note  A sample is created whenever a named profile block is entered.
     *        For example if you have a function you are profiling, and it gets
     *        called 10 times, there will be 10 samples.
     */
    struct PreciseProfileSample
    {
      PreciseProfileSample(uint64 _cycles, uint64 _numAllocs, uint64 _numFrees)
        : cycles(_cycles),
          numAllocs(_numAllocs),
          numFrees(_numFrees)
      {}

      uint64 cycles;
      uint64 numAllocs;
      uint64 numFrees;
    };

    /**
     * @brief Contains basic (time based) profiling data contained in a
     *        profiling block.
     */
    struct ProfileData
    {
      ProfileData(FrameAlloc* alloc);

      /**
       * @brief Begins a new sample and records current sample state.
       *        Previous sample must not be active.
       */
      void
      beginSample();

      /**
       * @brief Records current sample state and creates a new sample based on
       *        start and end state. Adds the sample to the sample list.
       */
      void
      endSample();

      /**
       * @brief Removes the last added sample from the sample list and makes it
       *        active again. You must call endSample() when done as if you
       *        called beginSample().
       */
      void
      resumeLastSample();

      Vector<ProfileSample, StdFrameAlloc<ProfileSample>> samples;
      ProfilerTimer timer;
      uint64 memAllocs;
      uint64 memFrees;
    };

    /**
     * @brief Contains precise (CPU cycle based) profiling data contained in a
     *        profiling block.
     */
    struct PreciseProfileData
    {
      PreciseProfileData(FrameAlloc* alloc);

      /**
       * @brief Begins a new sample and records current sample state.
       *        Previous sample must not be active.
       */
      void
      beginSample();

      /**
       * @brief Records current sample state and creates a new sample based on
       *        start and end state. Adds the sample to the sample list.
       */
      void
      endSample();

      /**
       * @brief Removes the last added sample from the sample list and makes it
       *        active again. You must call endSample() when done as if you
       *        called beginSample.
       */
      void
      resumeLastSample();

      Vector<PreciseProfileSample, StdFrameAlloc<PreciseProfileSample>> samples;
      ProfilerTimerPrecise timer;
      uint64 memAllocs;
      uint64 memFrees;
    };

    /**
     * @brief Contains all sampling information about a single named profiling
     *        block. Each block has its own sampling information and optionally
     *        child blocks.
     */
    struct ProfiledBlock
    {
      ProfiledBlock(FrameAlloc* alloc);
      ~ProfiledBlock();

      /**
       * @brief Attempts to find a child block with the specified name.
       *        Returns null if not found.
       */
      ProfiledBlock*
      findChild(const char* _name) const;

      char* name = nullptr;
      ProfileData basic;
      PreciseProfileData precise;
      Vector<ProfiledBlock*, StdFrameAlloc<ProfiledBlock*>> children;
    };

    /**
     * @brief Contains data about the currently active profiling block.
     */
    struct ActiveBlock
    {
      ActiveBlock()
        : type(ACTIVE_SAMPLING_TYPE::kBasic),
          block(nullptr)
      {}

      ActiveBlock(ACTIVE_SAMPLING_TYPE::E _type, ProfiledBlock* _block)
        : type(_type),
          block(_block)
      {}

      ACTIVE_SAMPLING_TYPE::E type;
      ProfiledBlock* block;
    };

    /**
     * @brief Contains data about an active profiling thread.
     */
    struct ThreadInfo
    {
      ThreadInfo();

      /**
       * @brief Starts profiling on the thread. New primary profiling block is
       *        created with the given name.
       */
      void
      begin(const char* _name);

      /**
       * @brief Ends profiling on the thread. You should end all samples before
       *        calling this, but if you don't they will be terminated
       *        automatically.
       */
      void
      end();

      /**
       * @brief Deletes all internal profiling data and makes the object ready
       *        for another iteration. Should be called after end in order to
       *        delete any existing data.
       */
      void
      reset();

      /**
       * @brief Gets the primary profiling block used by the thread.
       */
      ProfiledBlock*
      getBlock(const char* name);

      /**
       * @brief Deletes the provided block.
       */
      void
      releaseBlock(ProfiledBlock* block);

      static GE_THREADLOCAL ThreadInfo* activeThread;
      bool isActive;
      ProfiledBlock* rootBlock;
      FrameAlloc frameAlloc;
      ActiveBlock activeBlock;
      Stack<ActiveBlock, StdFrameAlloc<ActiveBlock>>* activeBlocks;
    };

   public:
    ProfilerCPU();
    ~ProfilerCPU();

    /**
     * @brief Registers a new thread we will be doing sampling in. This needs
     *        to be called before any beginSample* / endSample* calls are made
     *        in that thread.
     * @param[in] name  Name that will allow you to more easily identify the
     *                  thread.
     */
    void
    beginThread(const char* name);

    /**
     * @brief Ends sampling for the current thread.
     *        No beginSample* / endSample* calls after this point.
     */
    void
    endThread();

    /**
     * @brief Begins sample measurement. Must be followed by endSample().
     * @param[in] name  Unique name for the sample you can later use to find
     *                  the sampling data.
     */
    void
    beginSample(const char* name);

    /**
     * @brief Ends sample measurement.
     * @param[in] name  Unique name for the sample.
     * @note  Unique name is primarily needed to more easily identify
     *        mismatched begin/end sample pairs. Otherwise the name in
     *        beginSample() would be enough.
     */
    void
    endSample(const char* name);

    /**
     * @brief Begins precise sample measurement. Must be followed by
     *        endSamplePrecise().
     * @param[in] name  Unique name for the sample you can later use to find
     *                  the sampling data.
     * @note  This method uses very precise CPU counters to determine variety
     *        of data not provided by standard beginSample(). However due to
     *        the way these counters work you should not use this method for
     *        larger parts of code. It does not consider context switches so
     *        if the OS decides to switch context between measurements you will
     *        get invalid data.
     */
    void
    beginSamplePrecise(const char* name);

    /**
     * @brief Ends precise sample measurement.
     * @param[in] name  Unique name for the sample.
     * @note  Unique name is primarily needed to more easily identify
     *        mismatched begin/end sample pairs. Otherwise the name in
     *        beginSamplePrecise() would be enough.
     */
    void
    endSamplePrecise(const char* name);

    /**
     * @brief Clears all sampling data and ends any unfinished sampling blocks.
     */
    void
    reset();

    /**
     * @brief Generates a report from all previously sampled data.
     * @note  Generating a report will stop all in-progress sampling.
     *        You should make sure you call endSample* manually beforehand so
     *        this doesn't have to happen.
     */
    CPUProfilerReport
    generateReport();

   private:
    /**
     * @brief Calculates overhead that the timing and sampling methods
     *        themselves introduce so we might get more accurate measurements
     *        when creating reports.
     */
    void
    estimateTimerOverhead();

   private:
    double m_basicTimerOverhead;
    uint64 m_preciseTimerOverhead;
    double m_basicSamplingOverheadMs;
    double m_preciseSamplingOverheadMs;
    uint64 m_basicSamplingOverheadCycles;
    uint64 m_preciseSamplingOverheadCycles;
    ProfilerVector<ThreadInfo*> m_activeThreads;
    Mutex m_threadSync;
  };

  /**
   * @brief Profiling entry containing information about a single CPU profiling block containing timing information.
   */
  struct GE_CORE_EXPORT CPUProfilerBasicSamplingEntry
  {
    struct GE_CORE_EXPORT Data
    {
      Data();

      /**
       * Name of the profiling block.
       */
      String name;

      /**
       * Number of times the block was entered.
       */
      uint32 numCalls;

      /**
       * Number of memory allocations that happened within the block.
       */
      uint64 memAllocs;

      /**
       * Number of memory deallocations that happened within the block.
       */
      uint64 memFrees;

      /**
       * Average time it took to execute the block, per call. In milliseconds.
       */
      double avgTimeMs;

      /**
       * Maximum time of a single call in the block. In milliseconds.
       */
      double maxTimeMs;

      /**
       * Total time the block took, across all calls. In milliseconds.
       */
      double totalTimeMs;

      /**
       * Average time it took to execute the block, per call.
       * Ignores time used by child blocks. In milliseconds.
       */
      double avgSelfTimeMs;

      /**
       * Total time the block took, across all calls.
       * Ignores time used by child blocks. In milliseconds.
       */
      double totalSelfTimeMs;

      /**
       * Estimated overhead of profiling methods, only for this exact block.
       * In milliseconds.
       */
      double estimatedSelfOverheadMs;

      /**
       * Estimated overhead of profiling methods for this block and all
       * children. In milliseconds.
       */
      double estimatedOverheadMs;

      /**
       * Percent of parent block time this block took to execute.
       * Ranging [0.0, 1.0].
       */
      float pctOfParent;
    } data;

    ProfilerVector<CPUProfilerBasicSamplingEntry> childEntries;
  };

  /**
   * @brief Profiling entry containing information about a single CPU profiling
   *        block containing CPU cycle count based information.
   */
  struct GE_CORE_EXPORT CPUProfilerPreciseSamplingEntry
  {
    struct GE_CORE_EXPORT Data
    {
      Data();

      /**
       * Name of the profiling block.
       */
      String name;

      /**
       * Number of times the block was entered.
       */
      uint32 numCalls;

      /**
       * Number of memory allocations that happened within the block.
       */
      uint64 memAllocs;

      /**
       * Number of memory deallocations that happened within the block.
       */
      uint64 memFrees;

      /**
       * Average number of cycles it took to execute the block, per call.
       */
      uint64 avgCycles;

      /**
       * Maximum number of cycles of a single call in the block.
       */
      uint64 maxCycles;

      /**
       * Total number of cycles across all calls in the block.
       */
      uint64 totalCycles;

      /**
       * Average number of cycles it took to execute the block, per call.
       * Ignores cycles used by child blocks.
       */
      uint64 avgSelfCycles;

      /**
       * Total number of cycles across all calls in the block.
       * Ignores time used by child blocks.
       */
      uint64 totalSelfCycles;

      /**
       * Estimated overhead of profiling methods, only for this exact block.
       * In cycles.
       */
      uint64 estimatedSelfOverhead;

      /**
       * Estimated overhead of profiling methods for this block and all children. In cycles.
       */
      uint64 estimatedOverhead;

      /**
       * Percent of parent block cycles used by this block.
       * Ranging [0.0, 1.0].
       */
      float pctOfParent;
    } data;

    ProfilerVector<CPUProfilerPreciseSamplingEntry> childEntries;
  };

  /**
   * @brief CPU profiling report containing all profiling information for a
   *        single profiling session.
   */
  class GE_CORE_EXPORT CPUProfilerReport
  {
   public:
    CPUProfilerReport() = default;

    /**
     * @brief Returns root entry for the basic (time based) sampling data.
     *        Root entry always contains the profiling block associated with
     *        the entire thread.
     */
    const CPUProfilerBasicSamplingEntry&
    getBasicSamplingData() const {
      return m_basicSamplingRootEntry;
    }

    /**
     * @brief Returns root entry for the precise (CPU cycles) sampling data.
     *        Root entry always contains the profiling block associated with
     *        the entire thread.
     */
    const CPUProfilerPreciseSamplingEntry&
    getPreciseSamplingData() const {
      return m_preciseSamplingRootEntry;
    }

   private:
    friend class ProfilerCPU;

    CPUProfilerBasicSamplingEntry m_basicSamplingRootEntry;
    CPUProfilerPreciseSamplingEntry m_preciseSamplingRootEntry;
  };

  /**
   * @brief Provides global access to ProfilerCPU instance.
   */
  GE_CORE_EXPORT ProfilerCPU&
  g_profilerCPU();

  /**
   * @brief Shortcut for profiling a single function call.
   */
#define PROFILE_CALL(call, name)                                              \
	geEngineSDK::g_profilerCPU().beginSample(name);                             \
	call;                                                                       \
	geEngineSDK::g_profilerCPU().endSample(name);
}
