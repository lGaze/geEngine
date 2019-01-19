/*****************************************************************************/
/**
 * @file    geTime.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2015/02/18
 * @brief   Manages all time related functionality.
 *
 * Manages all time related functionality.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geTime.h"
#include "geTimer.h"
#include "geMath.h"

namespace geEngineSDK {
  using std::time;
  using std::time_t;
  using std::strftime;
  using std::gmtime;
  using std::localtime;
  using std::memory_order_relaxed;

  constexpr uint32 Time::MAX_ACCUM_FIXED_UPDATES;
  constexpr uint32 Time::NEW_FIXED_UPDATES_PER_FRAME;

  const double Time::MICROSEC_TO_SEC = 1.0 / 1000000.0;

  Time::Time() {
    m_timer = ge_new<Timer>();
    m_appStartTime = m_timer->getStartMs();
    m_lastFrameTime = m_timer->getMicroseconds();
    m_appStartUpDate = std::time(nullptr);
  }

  Time::~Time() {
    ge_delete(m_timer);
  }

  void
  Time::_update() {
    uint64 currentFrameTime = m_timer->getMicroseconds();

    if (!m_firstFrame) {
      m_frameDelta = static_cast<float>((currentFrameTime - m_lastFrameTime) *
                                        MICROSEC_TO_SEC);
    }
    else {
      m_frameDelta = 0.0f;
      m_firstFrame = false;
    }

    m_timeSinceStartMs = static_cast<uint64>(currentFrameTime / 1000);
    m_timeSinceStart = m_timeSinceStartMs / 1000.0f;
    m_lastFrameTime = currentFrameTime;
    m_currentFrame.fetch_add(1, memory_order_relaxed);
  }

  uint32
  Time::_getFixedUpdateStep(uint64& step) {
    const uint64 currentTime = getTimePrecise();

    //Skip fixed update first frame (time delta is zero, and no input received yet)
    if (m_firstFixedFrame) {
      m_lastFixedUpdateTime = currentTime;
      m_firstFixedFrame = false;
    }

    const uint64 nextFrameTime = m_lastFixedUpdateTime + m_fixedStep;
    if (nextFrameTime <= currentTime) {
      const auto simulationAmount = static_cast<int64>(Math::max(currentTime -
                                                                   m_lastFixedUpdateTime,
                                                                 m_fixedStep));

      auto numIterations = static_cast<uint32>(Math::divideAndRoundUp(simulationAmount,
                                               static_cast<int64>(m_fixedStep)));

      //If too many iterations are required, increase time step.
      //This should only happen in extreme situations (or when debugging).
      auto stepus = static_cast<int64>(m_fixedStep);
      if (m_numRemainingFixedUpdates < numIterations) {
        stepus = Math::divideAndRoundUp(simulationAmount,
                                        static_cast<int64>(m_numRemainingFixedUpdates));
        numIterations = static_cast<uint32>(
                        Math::divideAndRoundUp(simulationAmount, static_cast<int64>(stepus)));
      }

      GE_ASSERT(m_numRemainingFixedUpdates >= numIterations);

      m_numRemainingFixedUpdates -= numIterations;
      m_numRemainingFixedUpdates = Math::min(MAX_ACCUM_FIXED_UPDATES,
                                             m_numRemainingFixedUpdates +
                                             NEW_FIXED_UPDATES_PER_FRAME);

      step = stepus;
      return numIterations;
    }

    step = 0;
    return 0;
  }

  void
  Time::_advanceFixedUpdate(uint64 step)
  {
    m_lastFixedUpdateTime += step;
  }

  uint64
  Time::getTimePrecise() const {
    return m_timer->getMicroseconds();
  }

  String
  Time::getCurrentDateTime(bool isUTC) {
    time_t t = time(nullptr);
    char out[100];
    if (isUTC) {
      strftime(out, sizeof(out), "%A, %B %d, %Y %T", gmtime(&t));
    }
    else {
      strftime(out, sizeof(out), "%A, %B %d, %Y %T", localtime(&t));
    }

    return String(out);
  }

  String
  Time::getCurrentTime(bool isUTC) {
    time_t t = time(nullptr);
    char out[15];
    if (isUTC) {
      strftime(out, sizeof(out), "%T", std::gmtime(&t));
    }
    else {
      strftime(out, sizeof(out), "%T", std::localtime(&t));
    }

    return String(out);
  }

  String
  Time::getAppStartUpDate(bool isUTC) {
    char out[100];
    if (isUTC) {
      strftime(out, sizeof(out), "%A, %B %d, %Y %T", gmtime(&m_appStartUpDate));
    }
    else {
      strftime(out, sizeof(out), "%A, %B %d, %Y %T", localtime(&m_appStartUpDate));
    }

    return String(out);
  }

  Time&
  g_time() {
    return Time::instance();
  }
}
