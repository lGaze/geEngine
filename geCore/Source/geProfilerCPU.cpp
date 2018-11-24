/*****************************************************************************/
/**
 * @file    geProfilerCPU.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/27
 * @brief   Provides various performance measuring methods.
 *
 * Provides various performance measuring methods.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geProfilerCPU.h"
#include "gePlatform.h"

#include <geDebug.h>
#include <chrono>

#if GE_COMPILER == GE_COMPILER_MSVC
# include <intrin.h>
#endif

#if GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
# include <geCpuid.h>
#endif

#if GE_COMPILER == GE_COMPILER_CLANG
# if GE_PLATFORM == GE_PLATFORM_WIN32
#   include <intrin.h>
# else
#   include <x86intrin.h>
# endif
#endif

namespace geEngineSDK {
  using namespace std::chrono;

  ProfilerCPU::ProfilerTimer::ProfilerTimer()
    : m_startTime(0.0f) {
      m_time = 0.0f;
  }

  void
  ProfilerCPU::ProfilerTimer::start() {
    m_startTime = getCurrentTime();
  }

  void
  ProfilerCPU::ProfilerTimer::stop() {
    m_time += getCurrentTime() - m_startTime;
  }

  void
  ProfilerCPU::ProfilerTimer::reset() {
    m_time = 0.0f;
  }

  inline double
  ProfilerCPU::ProfilerTimer::getCurrentTime() const {
    high_resolution_clock::time_point timeNow = m_hrClock.now();
    nanoseconds timeNowNs = timeNow.time_since_epoch();
    return timeNowNs.count() * 0.000001;
  }

  void
  ProfilerCPU::ProfilerTimerPrecise::start() {
    m_startCycles = getNumCycles();
  }

  void
  ProfilerCPU::ProfilerTimerPrecise::stop() {
    m_cycles += getNumCycles() - m_startCycles;
  }

  void
  ProfilerCPU::ProfilerTimerPrecise::reset() {
    m_cycles = 0;
  }

  inline uint64
  ProfilerCPU::ProfilerTimerPrecise::getNumCycles() {
#if GE_COMPILER == GE_COMPILER_GNUC || GE_COMPILER == GE_COMPILER_CLANG
    unsigned int a = 0;
    unsigned int b[4];
    __get_cpuid(a, &b[0], &b[1], &b[2], &b[3]);
# if GE_ARCH_TYPE == GE_ARCHITECTURE_x86_64
    uint32 __a, __d;
    __asm__ __volatile__("rdtsc" : "=a" (__a), "=d" (__d));
    return (static_cast<uint64>(__a) | static_cast<uint64>(__d) << 32);
# else
    uint64 x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
# endif
#elif GE_COMPILER == GE_COMPILER_MSVC
    int a[4];
    int b = 0;
    __cpuid(a, b);
    return __rdtsc();
#else
    static_assert(false, "Unsupported compiler");
#endif		
  }

  ProfilerCPU::ProfileData::ProfileData(FrameAlloc* alloc)
    : samples(alloc)
  {}

  void
  ProfilerCPU::ProfileData::beginSample() {
    memAllocs = MemoryCounter::getNumAllocs();
    memFrees = MemoryCounter::getNumFrees();
    timer.reset();
    timer.start();
  }

  void
  ProfilerCPU::ProfileData::endSample() {
    timer.stop();
    uint64 numAllocs = MemoryCounter::getNumAllocs() - memAllocs;
    uint64 numFrees = MemoryCounter::getNumFrees() - memFrees;
    samples.emplace_back(timer.m_time, numAllocs, numFrees);
  }

  void
  ProfilerCPU::ProfileData::resumeLastSample() {
    timer.start();
    samples.erase(samples.end() - 1);
  }

  ProfilerCPU::PreciseProfileData::PreciseProfileData(FrameAlloc* alloc)
    : samples(alloc)
  {}

  void
  ProfilerCPU::PreciseProfileData::beginSample() {
    memAllocs = MemoryCounter::getNumAllocs();
    memFrees = MemoryCounter::getNumFrees();
    timer.reset();
    timer.start();
  }

  void
  ProfilerCPU::PreciseProfileData::endSample() {
    timer.stop();
    uint64 numAllocs = MemoryCounter::getNumAllocs() - memAllocs;
    uint64 numFrees = MemoryCounter::getNumFrees() - memFrees;
    samples.emplace_back(timer.m_cycles, numAllocs, numFrees);
  }

  void
  ProfilerCPU::PreciseProfileData::resumeLastSample() {
    timer.start();
    samples.erase(samples.end() - 1);
  }

  GE_THREADLOCAL ProfilerCPU::ThreadInfo* ProfilerCPU::ThreadInfo::activeThread = nullptr;

  ProfilerCPU::ThreadInfo::ThreadInfo()
    : isActive(false),
      rootBlock(nullptr),
      frameAlloc(1024 * 512),
      activeBlocks(nullptr)
  {}

  void
  ProfilerCPU::ThreadInfo::begin(const char* _name) {
    if (isActive) {
      LOGWRN("Profiler::beginThread called on a thread that was already "
             "being sampled");
      return;
    }

    if (nullptr == rootBlock) {
      rootBlock = getBlock(_name);
    }

    activeBlock = ActiveBlock(ACTIVE_SAMPLING_TYPE::kBasic, rootBlock);
    if (nullptr == activeBlocks) {
      activeBlocks = frameAlloc.construct<Stack<ActiveBlock,
        StdFrameAlloc<ActiveBlock>>>(StdFrameAlloc<ActiveBlock>(&frameAlloc));
    }

    activeBlocks->push(activeBlock);
    rootBlock->basic.beginSample();
    isActive = true;
  }

  void
  ProfilerCPU::ThreadInfo::end() {
    if (ACTIVE_SAMPLING_TYPE::kBasic == activeBlock.type) {
      activeBlock.block->basic.endSample();
    }
    else {
      activeBlock.block->precise.endSample();
    }

    activeBlocks->pop();

    if (!isActive) {
      LOGWRN("Profiler::endThread called on a thread that isn't being sampled.");
    }

    if (!activeBlocks->empty()) {
      LOGWRN("Profiler::endThread called but not all sample pairs were closed. "
             "Sampling data will not be valid.");

      while (!activeBlocks->empty()) {
        ActiveBlock& curBlock = activeBlocks->top();
        if (ACTIVE_SAMPLING_TYPE::kBasic == curBlock.type) {
          curBlock.block->basic.endSample();
        }
        else {
          curBlock.block->precise.endSample();
        }

        activeBlocks->pop();
      }
    }

    isActive = false;
    activeBlock = ActiveBlock();
    frameAlloc.free(activeBlocks);
    activeBlocks = nullptr;
  }

  void
  ProfilerCPU::ThreadInfo::reset() {
    if (isActive) {
      end();
    }

    if (nullptr != rootBlock) {
      releaseBlock(rootBlock);
    }

    rootBlock = nullptr;

    //NOTE: This never actually frees memory
    frameAlloc.clear();
  }

  ProfilerCPU::ProfiledBlock*
  ProfilerCPU::ThreadInfo::getBlock(const char* name) {
    auto block = frameAlloc.construct<ProfiledBlock>(&frameAlloc);
    block->name = reinterpret_cast<char*>(frameAlloc.alloc((strlen(name)+1)*sizeof(char)));
    strcpy(block->name, name);

    return block;
  }

  void
  ProfilerCPU::ThreadInfo::releaseBlock(ProfiledBlock* block) {
    frameAlloc.free(reinterpret_cast<uint8*>(block->name));
    frameAlloc.free(block);
  }

  ProfilerCPU::ProfiledBlock::ProfiledBlock(FrameAlloc* alloc)
    : basic(alloc),
      precise(alloc),
      children(alloc)
  {}

  ProfilerCPU::ProfiledBlock::~ProfiledBlock() {
    ThreadInfo* pThread = ThreadInfo::activeThread;

    for (auto& child : children) {
      pThread->releaseBlock(child);
    }

    children.clear();
  }

  ProfilerCPU::ProfiledBlock*
  ProfilerCPU::ProfiledBlock::findChild(const char* _name) const {
    for (auto& child : children) {
      if (strcmp(child->name, _name) == 0) {
        return child;
      }
    }

    return nullptr;
  }

  ProfilerCPU::ProfilerCPU()
    : m_basicTimerOverhead(0.0),
      m_preciseTimerOverhead(0),
      m_basicSamplingOverheadMs(0.0),
      m_preciseSamplingOverheadMs(0.0),
      m_basicSamplingOverheadCycles(0),
      m_preciseSamplingOverheadCycles(0) {
    //TODO: We only estimate overhead on program start. It might be better to
    //estimate it each time beginThread is called, and keep separate values
    //per thread.
    estimateTimerOverhead();
  }

  ProfilerCPU::~ProfilerCPU() {
    reset();
    Lock lock(m_threadSync);

    for (auto& threadInfo : m_activeThreads) {
      ge_delete<ThreadInfo, ProfilerAlloc>(threadInfo);
    }
  }

  void
  ProfilerCPU::beginThread(const char* name) {
    ThreadInfo* pThread = ThreadInfo::activeThread;
    if (nullptr == pThread) {
      ThreadInfo::activeThread = ge_new<ThreadInfo, ProfilerAlloc>();
      pThread = ThreadInfo::activeThread;
      {
        Lock lock(m_threadSync);
        m_activeThreads.push_back(pThread);
      }
    }

    pThread->begin(name);
  }

  void
  ProfilerCPU::endThread() {
    //I don't do a null check where on purpose, so endSample can be called ASAP
    ThreadInfo::activeThread->end();
  }

  void
  ProfilerCPU::beginSample(const char* name) {
    ThreadInfo* pThread = ThreadInfo::activeThread;
    if (nullptr == pThread || !pThread->isActive) {
      beginThread("Unknown");
      pThread = ThreadInfo::activeThread;
    }

    ProfiledBlock* parent = pThread->activeBlock.block;
    ProfiledBlock* block = nullptr;

    if (nullptr != parent) {
      block = parent->findChild(name);
    }

    if (nullptr == block) {
      block = pThread->getBlock(name);

      if (nullptr != parent) {
        parent->children.push_back(block);
      }
      else {
        pThread->rootBlock->children.push_back(block);
      }
    }

    pThread->activeBlock = ActiveBlock(ACTIVE_SAMPLING_TYPE::kBasic, block);
    pThread->activeBlocks->push(pThread->activeBlock);

    block->basic.beginSample();
  }

  void
  ProfilerCPU::endSample(const char* name) {
    ThreadInfo* pThread = ThreadInfo::activeThread;
    ProfiledBlock* block = pThread->activeBlock.block;

#if GE_DEBUG_MODE
    if (nullptr == block) {
      LOGWRN("Mismatched CPUProfiler::endSample. "
             "No beginSample was called.");
      return;
    }

    if (ACTIVE_SAMPLING_TYPE::kPrecise == pThread->activeBlock.type) {
      LOGWRN("Mismatched CPUProfiler::endSample. "
             "Was expecting Profiler::endSamplePrecise.");
      return;
    }

    if (strcmp(block->name, name) != 0) {
      LOGWRN("Mismatched CPUProfiler::endSample. Was expecting \"" +
             String(block->name) + "\" but got \"" +
             String(name) + "\". Sampling data will not be valid.");
      return;
    }
#else
    GE_UNREFERENCED_PARAMETER(name);
#endif

    block->basic.endSample();

    pThread->activeBlocks->pop();

    if (!pThread->activeBlocks->empty()) {
      pThread->activeBlock = pThread->activeBlocks->top();
    }
    else {
      pThread->activeBlock = ActiveBlock();
    }
  }

  void
  ProfilerCPU::beginSamplePrecise(const char* name) {
    //NOTE: There is a (small) possibility a context switch will happen during
    //this measurement in which case result will be skewed.
    //Increasing thread priority might help. This is generally only a problem
    //with code that executes a long time (10-15+ ms - depending on OS quant
    //length)
    ThreadInfo* pThread = ThreadInfo::activeThread;
    if (nullptr == pThread || !pThread->isActive) {
      beginThread("Unknown");
    }

    ProfiledBlock* parent = pThread->activeBlock.block;
    ProfiledBlock* block = nullptr;

    if (nullptr != parent) {
      block = parent->findChild(name);
    }

    if (nullptr == block) {
      block = pThread->getBlock(name);

      if (nullptr != parent) {
        parent->children.push_back(block);
      }
      else {
        pThread->rootBlock->children.push_back(block);
      }
    }

    pThread->activeBlock = ActiveBlock(ACTIVE_SAMPLING_TYPE::kPrecise, block);
    pThread->activeBlocks->push(pThread->activeBlock);

    block->precise.beginSample();
  }

  void
  ProfilerCPU::endSamplePrecise(const char* name) {
    ThreadInfo* pThread = ThreadInfo::activeThread;
    ProfiledBlock* block = pThread->activeBlock.block;

#if GE_DEBUG_MODE
    if (nullptr == block) {
      LOGWRN("Mismatched Profiler::endSamplePrecise. "
             "No beginSamplePrecise was called.");
      return;
    }

    if (ACTIVE_SAMPLING_TYPE::kBasic == pThread->activeBlock.type) {
      LOGWRN("Mismatched CPUProfiler::endSamplePrecise. "
             "Was expecting Profiler::endSample.");
      return;
    }

    if (strcmp(block->name, name) != 0) {
      LOGWRN("Mismatched Profiler::endSamplePrecise. Was expecting \"" +
             String(block->name) + "\" but got \"" +
             String(name) + "\". Sampling data will not be valid.");
      return;
    }
#else
    GE_UNREFERENCED_PARAMETER(name);
#endif
    block->precise.endSample();
    pThread->activeBlocks->pop();

    if (!pThread->activeBlocks->empty()) {
      pThread->activeBlock = pThread->activeBlocks->top();
    }
    else {
      pThread->activeBlock = ActiveBlock();
    }
  }

  void
  ProfilerCPU::reset() {
    ThreadInfo* pThread = ThreadInfo::activeThread;

    if (nullptr != pThread) {
      pThread->reset();
    }
  }

  CPUProfilerReport
  ProfilerCPU::generateReport() {
    CPUProfilerReport report;

    ThreadInfo* pThread = ThreadInfo::activeThread;
    if (nullptr == pThread) {
      return report;
    }

    if (pThread->isActive) {
      pThread->end();
    }

    //We need to separate out basic and precise data and form two separate
    //hierarchies
    if (nullptr == pThread->rootBlock) {
      return report;
    }

    struct TempEntry
    {
      TempEntry(ProfiledBlock* _parentBlock, uint32 _entryIdx)
        : parentBlock(_parentBlock),
          entryIdx(_entryIdx)
      {}

      ProfiledBlock* parentBlock;
      uint32 entryIdx;
      ProfilerVector<uint32> childIndexes;
    };

    ProfilerVector<CPUProfilerBasicSamplingEntry> basicEntries;
    ProfilerVector<CPUProfilerPreciseSamplingEntry> preciseEntries;

    //Fill up flatHierarchy array in a way so we always process children before
    //parents
    ProfilerStack<uint32> todo;
    ProfilerVector<TempEntry> flatHierarchy;

    uint32 entryIdx = 0;
    todo.push(entryIdx);
    flatHierarchy.emplace_back(pThread->rootBlock, entryIdx);
    ++entryIdx;

    while (!todo.empty()) {
      uint32 curDataIdx = todo.top();
      ProfiledBlock* curBlock = flatHierarchy[curDataIdx].parentBlock;

      todo.pop();

      for (auto& child : curBlock->children) {
        flatHierarchy[curDataIdx].childIndexes.push_back(entryIdx);
        todo.push(entryIdx);
        flatHierarchy.emplace_back(child, entryIdx);
        ++entryIdx;
      }
    }

    //Calculate sampling data for all entries
    basicEntries.resize(flatHierarchy.size());
    preciseEntries.resize(flatHierarchy.size());

    for (auto iter = flatHierarchy.rbegin(); iter != flatHierarchy.rend(); ++iter) {
      TempEntry& curData = *iter;
      ProfiledBlock* curBlock = curData.parentBlock;

      CPUProfilerBasicSamplingEntry* entryBasic = &basicEntries[curData.entryIdx];
      CPUProfilerPreciseSamplingEntry* entryPrecise = &preciseEntries[curData.entryIdx];

      //Calculate basic data
      entryBasic->data.name = String(curBlock->name);

      entryBasic->data.memAllocs = 0;
      entryBasic->data.memFrees = 0;
      entryBasic->data.totalTimeMs = 0.0;
      entryBasic->data.maxTimeMs = 0.0;
      for (auto& sample : curBlock->basic.samples) {
        entryBasic->data.totalTimeMs += sample.time;
        entryBasic->data.maxTimeMs = Math::max(entryBasic->data.maxTimeMs, sample.time);
        entryBasic->data.memAllocs += sample.numAllocs;
        entryBasic->data.memFrees += sample.numFrees;
      }

      entryBasic->data.numCalls = static_cast<uint32>(curBlock->basic.samples.size());

      if (entryBasic->data.numCalls > 0) {
        entryBasic->data.avgTimeMs = entryBasic->data.totalTimeMs / entryBasic->data.numCalls;
      }

      double totalChildTime = 0.0;
      for (auto& childIdx : curData.childIndexes) {
        CPUProfilerBasicSamplingEntry* childEntry = &basicEntries[childIdx];
        totalChildTime += childEntry->data.totalTimeMs;
        childEntry->data.pctOfParent = static_cast<float>(childEntry->data.totalTimeMs /
                                                          entryBasic->data.totalTimeMs);

        entryBasic->data.estimatedOverheadMs += childEntry->data.estimatedOverheadMs;
      }

      entryBasic->data.estimatedOverheadMs += curBlock->basic.samples.size() *
                                              m_basicSamplingOverheadMs;
      entryBasic->data.estimatedOverheadMs += curBlock->precise.samples.size() *
                                              m_preciseSamplingOverheadMs;

      entryBasic->data.totalSelfTimeMs = entryBasic->data.totalTimeMs - totalChildTime;

      if (entryBasic->data.numCalls > 0) {
        entryBasic->data.avgSelfTimeMs = entryBasic->data.totalSelfTimeMs /
                                         entryBasic->data.numCalls;
      }

      entryBasic->data.estimatedSelfOverheadMs = m_basicTimerOverhead;

      //Calculate precise data
      entryPrecise->data.name = String(curBlock->name);

      entryPrecise->data.memAllocs = 0;
      entryPrecise->data.memFrees = 0;
      entryPrecise->data.totalCycles = 0;
      entryPrecise->data.maxCycles = 0;
      for (auto& sample : curBlock->precise.samples) {
        entryPrecise->data.totalCycles += sample.cycles;
        entryPrecise->data.maxCycles = Math::max(entryPrecise->data.maxCycles, sample.cycles);
        entryPrecise->data.memAllocs += sample.numAllocs;
        entryPrecise->data.memFrees += sample.numFrees;
      }

      entryPrecise->data.numCalls = static_cast<uint32>(curBlock->precise.samples.size());

      if (entryPrecise->data.numCalls > 0) {
        entryPrecise->data.avgCycles = entryPrecise->data.totalCycles /
                                       entryPrecise->data.numCalls;
      }

      uint64 totalChildCycles = 0;
      for (auto& childIdx : curData.childIndexes) {
        CPUProfilerPreciseSamplingEntry* childEntry = &preciseEntries[childIdx];
        totalChildCycles += childEntry->data.totalCycles;
        childEntry->data.pctOfParent = childEntry->data.totalCycles /
                                       static_cast<float>(entryPrecise->data.totalCycles);

        entryPrecise->data.estimatedOverhead += childEntry->data.estimatedOverhead;
      }

      entryPrecise->data.estimatedOverhead += curBlock->precise.samples.size() *
                                              m_preciseSamplingOverheadCycles;
      entryPrecise->data.estimatedOverhead += curBlock->basic.samples.size() *
                                              m_basicSamplingOverheadCycles;

      entryPrecise->data.totalSelfCycles = entryPrecise->data.totalCycles - totalChildCycles;

      if (entryPrecise->data.numCalls > 0) {
        entryPrecise->data.avgSelfCycles = entryPrecise->data.totalSelfCycles /
                                           entryPrecise->data.numCalls;
      }

      entryPrecise->data.estimatedSelfOverhead = m_preciseTimerOverhead;
    }

    //Prune empty basic entries
    ProfilerStack<uint32> finalBasicHierarchyTodo;
    ProfilerStack<uint32> parentBasicEntryIndexes;
    ProfilerVector<TempEntry> newBasicEntries;

    finalBasicHierarchyTodo.push(0);

    entryIdx = 0;
    parentBasicEntryIndexes.push(entryIdx);
    newBasicEntries.emplace_back(nullptr, entryIdx);
    ++entryIdx;

    while (!finalBasicHierarchyTodo.empty()) {
      uint32 parentEntryIdx = parentBasicEntryIndexes.top();
      parentBasicEntryIndexes.pop();

      uint32 curEntryIdx = finalBasicHierarchyTodo.top();
      TempEntry& curEntry = flatHierarchy[curEntryIdx];
      finalBasicHierarchyTodo.pop();

      for (auto& childIdx : curEntry.childIndexes) {
        finalBasicHierarchyTodo.push(childIdx);

        CPUProfilerBasicSamplingEntry& basicEntry = basicEntries[childIdx];
        if (basicEntry.data.numCalls > 0) {
          newBasicEntries.emplace_back(nullptr, childIdx);
          newBasicEntries[parentEntryIdx].childIndexes.push_back(entryIdx);
          parentBasicEntryIndexes.push(entryIdx);
          ++entryIdx;
        }
        else {
          parentBasicEntryIndexes.push(parentEntryIdx);
        }
      }
    }

    if (!newBasicEntries.empty()) {
      ProfilerVector<CPUProfilerBasicSamplingEntry*> finalBasicEntries;

      report.m_basicSamplingRootEntry = basicEntries[newBasicEntries[0].entryIdx];
      finalBasicEntries.push_back(&report.m_basicSamplingRootEntry);
      finalBasicHierarchyTodo.push(0);

      while (!finalBasicHierarchyTodo.empty()) {
        uint32 curEntryIdx = finalBasicHierarchyTodo.top();
        finalBasicHierarchyTodo.pop();

        TempEntry& curEntry = newBasicEntries[curEntryIdx];

        CPUProfilerBasicSamplingEntry* basicEntry = finalBasicEntries[curEntryIdx];

        basicEntry->childEntries.resize(curEntry.childIndexes.size());
        uint32 idx = 0;

        for (auto& childIdx : curEntry.childIndexes) {
          TempEntry& childEntry = newBasicEntries[childIdx];
          basicEntry->childEntries[idx] = basicEntries[childEntry.entryIdx];
          finalBasicEntries.push_back(&(basicEntry->childEntries[idx]));
          finalBasicHierarchyTodo.push(childIdx);
          ++idx;
        }
      }
    }

    //Prune empty precise entries
    ProfilerStack<uint32> finalPreciseHierarchyTodo;
    ProfilerStack<uint32> parentPreciseEntryIndexes;
    ProfilerVector<TempEntry> newPreciseEntries;

    finalPreciseHierarchyTodo.push(0);

    entryIdx = 0;
    parentPreciseEntryIndexes.push(entryIdx);
    newPreciseEntries.emplace_back(nullptr, entryIdx);
    ++entryIdx;

    while (!finalPreciseHierarchyTodo.empty()) {
      uint32 parentEntryIdx = parentPreciseEntryIndexes.top();
      parentPreciseEntryIndexes.pop();

      uint32 curEntryIdx = finalPreciseHierarchyTodo.top();
      TempEntry& curEntry = flatHierarchy[curEntryIdx];
      finalPreciseHierarchyTodo.pop();

      for (auto& childIdx : curEntry.childIndexes) {
        finalPreciseHierarchyTodo.push(childIdx);

        CPUProfilerPreciseSamplingEntry& preciseEntry = preciseEntries[childIdx];
        if (preciseEntry.data.numCalls > 0) {
          newPreciseEntries.emplace_back(nullptr, childIdx);
          newPreciseEntries[parentEntryIdx].childIndexes.push_back(entryIdx);
          parentPreciseEntryIndexes.push(entryIdx);
          ++entryIdx;
        }
        else {
          parentPreciseEntryIndexes.push(parentEntryIdx);
        }
      }
    }

    if (!newPreciseEntries.empty()) {
      ProfilerVector<CPUProfilerPreciseSamplingEntry*> finalPreciseEntries;

      report.m_preciseSamplingRootEntry = preciseEntries[newPreciseEntries[0].entryIdx];
      finalPreciseEntries.push_back(&report.m_preciseSamplingRootEntry);
      finalPreciseHierarchyTodo.push(0);

      while (!finalPreciseHierarchyTodo.empty()) {
        uint32 curEntryIdx = finalPreciseHierarchyTodo.top();
        finalPreciseHierarchyTodo.pop();

        TempEntry& curEntry = newPreciseEntries[curEntryIdx];
        CPUProfilerPreciseSamplingEntry* preciseEntry = finalPreciseEntries[curEntryIdx];

        preciseEntry->childEntries.resize(curEntry.childIndexes.size());
        uint32 idx = 0;

        for (auto& childIdx : curEntry.childIndexes) {
          TempEntry& childEntry = newPreciseEntries[childIdx];
          preciseEntry->childEntries[idx] = preciseEntries[childEntry.entryIdx];
          finalPreciseEntries.push_back(&preciseEntry->childEntries.back());
          finalPreciseHierarchyTodo.push(childIdx);
          ++idx;
        }
      }
    }

    return report;
  }

  void
  ProfilerCPU::estimateTimerOverhead() {
    //Get an idea of how long timer calls and RDTSC takes
    const uint32 reps = 1000, sampleReps = 20;

    m_basicTimerOverhead = 1000000.0;
    m_preciseTimerOverhead = 1000000;
    for (uint32 tries = 0; tries < 20; ++tries) {
      ProfilerTimer timer;
      for (uint32 i = 0; i < reps; ++i) {
        timer.start();
        timer.stop();
      }

      double avgTime = double(timer.m_time) / double(reps);
      if (avgTime < m_basicTimerOverhead) {
        m_basicTimerOverhead = avgTime;
      }

      ProfilerTimerPrecise timerPrecise;
      for (uint32 i = 0; i < reps; ++i) {
        timerPrecise.start();
        timerPrecise.stop();
      }

      uint64 avgCycles = timerPrecise.m_cycles / reps;

      if (avgCycles < m_preciseTimerOverhead) {
        m_preciseTimerOverhead = avgCycles;
      }
    }

    m_basicSamplingOverheadMs = 1000000.0;
    m_preciseSamplingOverheadMs = 1000000.0;
    m_basicSamplingOverheadCycles = 1000000;
    m_preciseSamplingOverheadCycles = 1000000;
    for (uint32 tries = 0; tries < 3; ++tries) {
      //AVERAGE TIME IN MS FOR BASIC SAMPLING
      ProfilerTimer timerA;
      timerA.start();
      beginThread("Main");

      //Two different cases that can effect performance, one where sample
      //already exists and other where new one needs to be created
      for (uint32 i = 0; i < sampleReps; ++i) {
        beginSample("TestAvg1");
        endSample("TestAvg1");
        beginSample("TestAvg2");
        endSample("TestAvg2");
        beginSample("TestAvg3");
        endSample("TestAvg3");
        beginSample("TestAvg4");
        endSample("TestAvg4");
        beginSample("TestAvg5");
        endSample("TestAvg5");
        beginSample("TestAvg6");
        endSample("TestAvg6");
        beginSample("TestAvg7");
        endSample("TestAvg7");
        beginSample("TestAvg8");
        endSample("TestAvg8");
        beginSample("TestAvg9");
        endSample("TestAvg9");
        beginSample("TestAvg10");
        endSample("TestAvg10");
      }

      for (uint32 i = 0; i < sampleReps * 5; ++i) {
        beginSample(("TestAvg#" + toString(i)).c_str());
        endSample(("TestAvg#" + toString(i)).c_str());
      }

      endThread();

      timerA.stop();
      reset();

      double avgTimeBasic = timerA.m_time /
                            double(sampleReps * 10 + sampleReps * 5) -
                            m_basicTimerOverhead;

      if (avgTimeBasic < m_basicSamplingOverheadMs) {
        m_basicSamplingOverheadMs = avgTimeBasic;
      }

      //AVERAGE CYCLES FOR BASIC SAMPLING

      ProfilerTimerPrecise timerPreciseA;
      timerPreciseA.start();

      beginThread("Main");

      // Two different cases that can effect performance, one where sample
      //already exists and other where new one needs to be created
      for (uint32 i = 0; i < sampleReps; ++i) {
        beginSample("TestAvg1");
        endSample("TestAvg1");
        beginSample("TestAvg2");
        endSample("TestAvg2");
        beginSample("TestAvg3");
        endSample("TestAvg3");
        beginSample("TestAvg4");
        endSample("TestAvg4");
        beginSample("TestAvg5");
        endSample("TestAvg5");
        beginSample("TestAvg6");
        endSample("TestAvg6");
        beginSample("TestAvg7");
        endSample("TestAvg7");
        beginSample("TestAvg8");
        endSample("TestAvg8");
        beginSample("TestAvg9");
        endSample("TestAvg9");
        beginSample("TestAvg10");
        endSample("TestAvg10");
      }

      for (uint32 i = 0; i < sampleReps * 5; ++i) {
        beginSample(("TestAvg#" + toString(i)).c_str());
        endSample(("TestAvg#" + toString(i)).c_str());
      }

      endThread();
      timerPreciseA.stop();

      reset();

      uint64 avgCyclesBasic = timerPreciseA.m_cycles /
                              (sampleReps * 10 + sampleReps * 5) -
                              m_preciseTimerOverhead;

      if (avgCyclesBasic < m_basicSamplingOverheadCycles) {
        m_basicSamplingOverheadCycles = avgCyclesBasic;
      }

      //AVERAGE TIME IN MS FOR PRECISE SAMPLING

      ProfilerTimer timerB;
      timerB.start();
      beginThread("Main");

      //Two different cases that can effect performance, one where sample
      //already exists and other where new one needs to be created
      for (uint32 i = 0; i < sampleReps; ++i) {
        beginSamplePrecise("TestAvg1");
        endSamplePrecise("TestAvg1");
        beginSamplePrecise("TestAvg2");
        endSamplePrecise("TestAvg2");
        beginSamplePrecise("TestAvg3");
        endSamplePrecise("TestAvg3");
        beginSamplePrecise("TestAvg4");
        endSamplePrecise("TestAvg4");
        beginSamplePrecise("TestAvg5");
        endSamplePrecise("TestAvg5");
        beginSamplePrecise("TestAvg6");
        endSamplePrecise("TestAvg6");
        beginSamplePrecise("TestAvg7");
        endSamplePrecise("TestAvg7");
        beginSamplePrecise("TestAvg8");
        endSamplePrecise("TestAvg8");
        beginSamplePrecise("TestAvg9");
        endSamplePrecise("TestAvg9");
        beginSamplePrecise("TestAvg10");
        endSamplePrecise("TestAvg10");
      }

      for (uint32 i = 0; i < sampleReps * 5; ++i) {
        beginSamplePrecise(("TestAvg#" + toString(i)).c_str());
        endSamplePrecise(("TestAvg#" + toString(i)).c_str());
      }

      endThread();
      timerB.stop();
      reset();

      double avgTimesPrecise = timerB.m_time /
                               (sampleReps * 10 + sampleReps * 5);

      if (avgTimesPrecise < m_preciseSamplingOverheadMs) {
        m_preciseSamplingOverheadMs = avgTimesPrecise;
      }

      //AVERAGE CYCLES FOR PRECISE SAMPLING

      ProfilerTimerPrecise timerPreciseB;
      timerPreciseB.start();
      beginThread("Main");

      //Two different cases that can effect performance, one where sample
      //already exists and other where new one needs to be created
      for (uint32 i = 0; i < sampleReps; ++i) {
        beginSamplePrecise("TestAvg1");
        endSamplePrecise("TestAvg1");
        beginSamplePrecise("TestAvg2");
        endSamplePrecise("TestAvg2");
        beginSamplePrecise("TestAvg3");
        endSamplePrecise("TestAvg3");
        beginSamplePrecise("TestAvg4");
        endSamplePrecise("TestAvg4");
        beginSamplePrecise("TestAvg5");
        endSamplePrecise("TestAvg5");
        beginSamplePrecise("TestAvg6");
        endSamplePrecise("TestAvg6");
        beginSamplePrecise("TestAvg7");
        endSamplePrecise("TestAvg7");
        beginSamplePrecise("TestAvg8");
        endSamplePrecise("TestAvg8");
        beginSamplePrecise("TestAvg9");
        endSamplePrecise("TestAvg9");
        beginSamplePrecise("TestAvg10");
        endSamplePrecise("TestAvg10");
      }

      for (uint32 i = 0; i < sampleReps * 5; ++i) {
        beginSamplePrecise(("TestAvg#" + toString(i)).c_str());
        endSamplePrecise(("TestAvg#" + toString(i)).c_str());
      }

      endThread();
      timerPreciseB.stop();
      reset();

      uint64 avgCyclesPrecise = timerPreciseB.m_cycles /
                                (sampleReps * 10 + sampleReps * 5);

      if (avgCyclesPrecise < m_preciseSamplingOverheadCycles) {
        m_preciseSamplingOverheadCycles = avgCyclesPrecise;
      }
    }
  }

  CPUProfilerBasicSamplingEntry::Data::Data()
    : numCalls(0),
      avgTimeMs(0.0),
      maxTimeMs(0.0),
      totalTimeMs(0.0),
      avgSelfTimeMs(0.0),
      totalSelfTimeMs(0.0),
      estimatedSelfOverheadMs(0.0),
      estimatedOverheadMs(0.0),
      pctOfParent(1.0f)
  {}

  CPUProfilerPreciseSamplingEntry::Data::Data()
    : numCalls(0),
      avgCycles(0),
      maxCycles(0),
      totalCycles(0),
      avgSelfCycles(0),
      totalSelfCycles(0),
      estimatedSelfOverhead(0),
      estimatedOverhead(0),
      pctOfParent(1.0f)
  {}

  ProfilerCPU&
  g_profilerCPU() {
    return ProfilerCPU::instance();
  }
}
