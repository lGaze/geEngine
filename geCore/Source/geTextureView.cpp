/*****************************************************************************/
/**
 * @file    geTextureView.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/19
 * @brief   Texture views allow you to reference only a part of a texture.
 *
 * Texture views allow you to reference only a part of a texture. They may
 * reference one or multiple mip-levels on one or multiple texture array slices.
 * Selected mip level will apply to all slices.
 * They also allow you to re-purpose a texture (for example make a render
 * target a bindable texture).
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geTextureView.h"
#include "geTexture.h"

namespace geEngineSDK {
  namespace geCoreThread {
    size_t
    TextureView::HashFunction::operator()(const TEXTURE_VIEW_DESC &key) const {
      size_t seed = 0;
      hash_combine(seed, key.mostDetailMip);
      hash_combine(seed, key.numMips);
      hash_combine(seed, key.firstArraySlice);
      hash_combine(seed, key.numArraySlices);
      hash_combine(seed, key.usage);
      return seed;
    }

    bool
    TextureView::EqualFunction::operator()(const TEXTURE_VIEW_DESC &a,
                                           const TEXTURE_VIEW_DESC &b) const {
      return a.mostDetailMip == b.mostDetailMip &&
             a.numMips == b.numMips &&
             a.firstArraySlice == b.firstArraySlice &&
             a.numArraySlices == b.numArraySlices &&
             a.usage == b.usage;
    }

    TextureView::TextureView(const TEXTURE_VIEW_DESC& desc)
      : m_desc(desc)
    {}
  }
}
