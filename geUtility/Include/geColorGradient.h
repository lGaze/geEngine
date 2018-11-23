/*****************************************************************************/
/**
 * @file    geColorGradient.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/06/06
 * @brief   Class to manage a multi-keys color gradient.
 *
 * Class to manage a multi-keys color gradient.
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
#include "gePrerequisitesUtil.h"
#include "geColor.h"
#include "gePoolAlloc.h"

namespace geEngineSDK {
  struct GE_SCRIPT_EXPORT(m:Image, pl:true) ColorGradientKey
  {
    ColorGradientKey() = default;
    ColorGradientKey(const LinearColor& _color, float _time)
      : color(_color),
        time(_time)
    {}

    LinearColor color;
    float time = 0.0f;
  };

  class GE_UTILITY_EXPORT GE_SCRIPT_EXPORT(m:Image) ColorGradient final
  {
    constexpr static uint32 MAX_KEYS = 8;
   public:
    GE_SCRIPT_EXPORT()
    ColorGradient() = default;

    GE_SCRIPT_EXPORT()
    ColorGradient(const LinearColor& color);

    GE_SCRIPT_EXPORT()
    ColorGradient(const Vector<ColorGradientKey>& keys);

    ~ColorGradient() = default;

    LinearColor
    evaluate(float t) const;

    /**
     * @brief Keys that control the gradient, sorted by time from first to last.
     *        Key times should be in range [0, 1].
     */
    GE_SCRIPT_EXPORT() void
    setKeys(const Vector<ColorGradientKey>& keys, float duration = 1.0f);

    /** @copydoc setKeys */
    GE_SCRIPT_EXPORT() Vector<ColorGradientKey>
    getKeys() const;

    /**
     * @brief Specify a "gradient" that represents a single color value.
     */
    GE_SCRIPT_EXPORT() void
    setConstant(const LinearColor& color);

    /**
     * @brief Returns the duration over which the gradient values are
     *        interpolated over. Corresponds to the time value of the final
     *        keyframe.
     */
    float
    getDuration() const {
      return m_duration;
    }

    /** Returns the time of the first and last keyframe in the gradient. */
    std::pair<float, float>
    getTimeRange() const;

   private:
    friend struct RTTIPlainType<ColorGradient>;

    LinearColor m_colors[MAX_KEYS];
    float m_times[MAX_KEYS];
    uint32 m_numKeys = 0;
    float m_duration = 0.0f;
  };

  IMPLEMENT_GLOBAL_POOL(ColorGradient, 32)
}
