/*****************************************************************************/
/**
 * @file    geProfilerGPU.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geProfilerGPU.h"
#include "geRenderStats.h"
#include "geTimerQuery.h"
#include "geOcclusionQuery.h"

#include <geException.h>

namespace geEngineSDK {
  using std::move;

  const uint32 ProfilerGPU::MAX_QUEUE_ELEMENTS = 5;

  ProfilerGPU::ProfilerGPU()
    : m_isFrameActive(false),
      m_readyReports(nullptr),
      m_reportHeadPos(0),
      m_reportCount(0) {
    m_readyReports = ge_newN<GPUProfilerReport>(MAX_QUEUE_ELEMENTS);
  }

  ProfilerGPU::~ProfilerGPU() {
    while (!m_unresolvedFrames.empty()) {
      ProfiledSample& frameSample = m_unresolvedFrames.front();
      freeSample(frameSample);
      m_unresolvedFrames.pop();
    }

    ge_deleteN(m_readyReports, MAX_QUEUE_ELEMENTS);
  }

  void
  ProfilerGPU::beginFrame() {
    if (m_isFrameActive) {
      LOGERR("Cannot begin a frame because another frame is active.");
      return;
    }

    m_frameSample = ProfiledSample();
    m_frameSample.name = "Frame";
    beginSampleInternal(m_frameSample, true);

    m_isFrameActive = true;
  }

  void
  ProfilerGPU::endFrame() {
    if (!m_activeSamples.empty()) {
      LOGERR("Attempting to end a frame while a sample is active.");
      return;
    }

    if (!m_isFrameActive) {
      return;
    }

    endSampleInternal(m_frameSample);

    m_unresolvedFrames.push(m_frameSample);
    m_isFrameActive = false;
  }

  void
  ProfilerGPU::beginSample(ProfilerString name) {
    if (!m_isFrameActive) {
      LOGERR("Cannot begin a sample because no frame is active.");
      return;
    }

    auto sample = m_samplePool.construct<ProfiledSample>();
    sample->name = move(name);
    beginSampleInternal(*sample, false);

    if (m_activeSamples.empty()) {
      m_frameSample.children.push_back(sample);
    }
    else {
      ProfiledSample* parent = m_activeSamples.top();
      parent->children.push_back(sample);
    }

    m_activeSamples.push(sample);
  }

  void
  ProfilerGPU::endSample(const ProfilerString& name) {
    if (m_activeSamples.empty()) {
      return;
    }

    ProfiledSample* lastSample = m_activeSamples.top();
    if (name != lastSample->name) {
      String errorStr = "Attempting to end a sample that doesn't match. Got: " +
        String(name.c_str()) +
        ". Expected: " +
        String(lastSample->name.c_str());

      LOGERR(errorStr);
      return;
    }

    endSampleInternal(*lastSample);
    m_activeSamples.pop();
  }

  uint32
  ProfilerGPU::getNumAvailableReports() {
    Lock lock(m_mutex);
    return m_reportCount;
  }

  GPUProfilerReport
  ProfilerGPU::getNextReport() {
    Lock lock(m_mutex);

    if (0 == m_reportCount) {
      LOGERR("No reports are available.");
      return GPUProfilerReport();
    }

    GPUProfilerReport report = m_readyReports[m_reportHeadPos];

    m_reportHeadPos = (m_reportHeadPos + 1) % MAX_QUEUE_ELEMENTS;
    --m_reportCount;

    return report;
  }

  void
  ProfilerGPU::_update() {
    while (!m_unresolvedFrames.empty()) {
      ProfiledSample& frameSample = m_unresolvedFrames.front();

      //Frame sample timer query is the last query we issued so if it is
      //complete, we may assume all queries are complete.
      if (frameSample.activeTimeQuery->isReady()) {
        GPUProfilerReport report;
        resolveSample(frameSample, report.frameSample);

        freeSample(frameSample);
        m_unresolvedFrames.pop();

        {
          Lock lock(m_mutex);
          m_readyReports[(m_reportHeadPos + m_reportCount) % MAX_QUEUE_ELEMENTS] = report;
          if (MAX_QUEUE_ELEMENTS == m_reportCount) {
            m_reportHeadPos = (m_reportHeadPos + 1) % MAX_QUEUE_ELEMENTS;
          }
          else {
            ++m_reportCount;
          }
        }
      }
      else {
        break;
      }
    }
  }

  void
  ProfilerGPU::freeSample(ProfiledSample& sample) {
    for (auto& entry : sample.children) {
      freeSample(*entry);
      m_samplePool.destruct(entry);
    }

    sample.children.clear();

    m_freeTimerQueries.push(sample.activeTimeQuery);

    if (sample.activeOcclusionQuery) {
      m_freeOcclusionQueries.push(sample.activeOcclusionQuery);
    }
  }

  void
  ProfilerGPU::resolveSample(const ProfiledSample& sample,
                             GPUProfileSample& reportSample) {
    reportSample.name.assign(sample.name.data(), sample.name.size());
    reportSample.timeMs = sample.activeTimeQuery->getTimeMs();

    if (sample.activeOcclusionQuery) {
      reportSample.numDrawnSamples = sample.activeOcclusionQuery->getNumSamples();
    }
    else {
      reportSample.numDrawnSamples = 0;
    }

    reportSample.numDrawCalls = static_cast<uint32>
      (sample.endStats.numDrawCalls - sample.startStats.numDrawCalls);
    reportSample.numRenderTargetChanges = static_cast<uint32>
      (sample.endStats.numRenderTargetChanges - sample.startStats.numRenderTargetChanges);
    reportSample.numPresents = static_cast<uint32>
      (sample.endStats.numPresents - sample.startStats.numPresents);
    reportSample.numClears = static_cast<uint32>
      (sample.endStats.numClears - sample.startStats.numClears);

    reportSample.numVertices = static_cast<uint32>
      (sample.endStats.numVertices - sample.startStats.numVertices);
    reportSample.numPrimitives = static_cast<uint32>
      (sample.endStats.numPrimitives - sample.startStats.numPrimitives);

    reportSample.numPipelineStateChanges = static_cast<uint32>
      (sample.endStats.numPipelineStateChanges - sample.startStats.numPipelineStateChanges);

    reportSample.numGpuParamBinds = static_cast<uint32>
      (sample.endStats.numGPUParamBinds - sample.startStats.numGPUParamBinds);
    reportSample.numVertexBufferBinds = static_cast<uint32>
      (sample.endStats.numVertexBufferBinds - sample.startStats.numVertexBufferBinds);
    reportSample.numIndexBufferBinds = static_cast<uint32>
      (sample.endStats.numIndexBufferBinds - sample.startStats.numIndexBufferBinds);

    reportSample.numResourceWrites = static_cast<uint32>
      (sample.endStats.numResourceWrites - sample.startStats.numResourceWrites);
    reportSample.numResourceReads = static_cast<uint32>
      (sample.endStats.numResourceReads - sample.startStats.numResourceReads);

    reportSample.numObjectsCreated = static_cast<uint32>
      (sample.endStats.numObjectsCreated - sample.startStats.numObjectsCreated);
    reportSample.numObjectsDestroyed = static_cast<uint32>
      (sample.endStats.numObjectsDestroyed - sample.startStats.numObjectsDestroyed);

    for (auto& entry : sample.children) {
      reportSample.children.emplace_back();
      resolveSample(*entry, reportSample.children.back());
    }
  }

  void
  ProfilerGPU::beginSampleInternal(ProfiledSample& sample, bool issueOcclusion) {
    sample.startStats = RenderStats::instance().getData();
    sample.activeTimeQuery = getTimerQuery();
    sample.activeTimeQuery->begin();

    if (issueOcclusion) {
      sample.activeOcclusionQuery = getOcclusionQuery();
      sample.activeOcclusionQuery->begin();
    }
  }

  void
  ProfilerGPU::endSampleInternal(ProfiledSample& sample) {
    sample.endStats = RenderStats::instance().getData();

    if (sample.activeOcclusionQuery) {
      sample.activeOcclusionQuery->end();
    }
    sample.activeTimeQuery->end();
  }

  SPtr<geCoreThread::TimerQuery>
  ProfilerGPU::getTimerQuery() const {
    if (!m_freeTimerQueries.empty()) {
      SPtr<geCoreThread::TimerQuery> timerQuery = m_freeTimerQueries.top();
      m_freeTimerQueries.pop();
      return timerQuery;
    }

    return geCoreThread::TimerQuery::create();
  }

  SPtr<geCoreThread::OcclusionQuery>
  ProfilerGPU::getOcclusionQuery() const {
    if (!m_freeOcclusionQueries.empty()) {
      SPtr<geCoreThread::OcclusionQuery> occlusionQuery = m_freeOcclusionQueries.top();
      m_freeOcclusionQueries.pop();
      return occlusionQuery;
    }

    return geCoreThread::OcclusionQuery::create(false);
  }

  ProfilerGPU&
  g_profilerGPU() {
    return ProfilerGPU::instance();
  }
}
