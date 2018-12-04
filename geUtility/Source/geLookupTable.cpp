/*****************************************************************************/
/**
 * @file    geLookupTable.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/12/03
 * @brief   Set of samples got from sampling some function at equal intervals.
 *
 * Contains a set of samples resulting from sampling some function at equal
 * intervals. The table can then be used for sampling that function at
 * arbitrary time intervals. The sampling is fast but precision is limited to
 * the number of samples.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geLookupTable.h"
#include "geMath.h"

namespace geEngineSDK {
  LookupTable::LookupTable(Vector<float> values,
                           float startTime,
                           float endTime,
                           uint32 sampleSize)
    : m_values(std::move(values)),
      m_sampleSize(Math::max(sampleSize, 1U)),
      m_numSamples(static_cast<uint32>(m_values.size()) / m_sampleSize),
      m_timeStart(startTime) {
    if (endTime < startTime) {
      endTime = startTime;
    }

    float timeInterval;
    if (m_numSamples > 1) {
      timeInterval = (endTime - startTime) / (m_numSamples - 1);
    }
    else {
      timeInterval = 0.0f;
    }

    m_timeScale = 1.0f / timeInterval;
  }

  void
  LookupTable::evaluate(float t,
                        const float*& left,
                        const float*& right,
                        float& fraction) const {
    t -= m_timeStart;
    t *= m_timeScale;

    const auto index = static_cast<uint32>(t);
    fraction = Math::fractional(t);

    const uint32 leftIdx = Math::min(index, m_numSamples - 1);
    const uint32 rightIdx = Math::min(index + 1, m_numSamples - 1);

    left = &m_values[leftIdx * m_sampleSize];
    right = &m_values[rightIdx * m_sampleSize];
  }

  const float*
  LookupTable::getSample(uint32 idx) const {
    if (0 == m_numSamples) {
      return nullptr;
    }

    idx = Math::min(idx, m_numSamples - 1);
    return &m_values[idx * m_sampleSize];
  }
}
