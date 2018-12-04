/*****************************************************************************/
/**
 * @file    geBitfield.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/12/03
 * @brief   References a single bit in a Bitfield.
 *
 * References a single bit in a Bitfield.
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
#include "geMath.h"
#include "geBitwise.h"

namespace geEngineSDK {
  using std::exchange;

  class Bitfield;

  /**
   * @brief References a single bit in a Bitfield.
   */
  class BitReferenceConst
  {
   public:
    BitReferenceConst(const uint32& data, uint32 bitMask)
      : m_data(data),
        m_bitMask(bitMask)
    {}

    operator bool() const {
      return (m_data & m_bitMask) != 0;
    }

   protected:
    const uint32& m_data;
    uint32 m_bitMask;
  };

  /**
   * @brief References a single bit in a Bitfield and allows it to be modified.
   */
  class BitReference
  {
   public:
    BitReference(uint32& data, uint32 bitMask)
      : m_data(data),
        m_bitMask(bitMask)
    {}

    operator bool() const {
      return (m_data & m_bitMask) != 0;
    }

    BitReference&
    operator=(bool value) {
      if (value) {
        m_data |= m_bitMask;
      }
      else {
        m_data &= ~m_bitMask;
      }

      return *this;
    }

    BitReference&
    operator=(const BitReference& rhs) {
      *this = static_cast<bool>(rhs);
      return *this;
    }

   protected:
    uint32& m_data;
    uint32 m_bitMask;
  };

  /**
   * @brief Helper template used for specifying types for const and non-const
   *        iterator variants for Bitfield.
   */
  template<bool CONST>
  struct TBitfieldIteratorTypes {};

  template<>
  struct TBitfieldIteratorTypes<true>
  {
    using ArrayType = const Bitfield&;
    using ReferenceType = BitReferenceConst;
  };

  template<>
  struct TBitfieldIteratorTypes<false>
  {
    using ArrayType = Bitfield&;
    using ReferenceType = BitReference;
  };

  /**
   * @brief Iterator for iterating over individual bits in a Bitfield.
   */
  template<bool CONST>
  class TBitfieldIterator
  {
   public:
    using ArrayType = typename TBitfieldIteratorTypes<CONST>::ArrayType;
    using ReferenceType = typename TBitfieldIteratorTypes<CONST>::ReferenceType;

    TBitfieldIterator(ArrayType owner,
                      uint32 bitIndex,
                      uint32 dwordIndex,
                      uint32 mask)
      : m_owner(owner),
        m_bitIndex(bitIndex),
        m_dwordIndex(dwordIndex),
        m_mask(mask)
    {}

    TBitfieldIterator&
    operator++() {
      ++m_bitIndex;
      m_mask <<= 1;

      if (!m_mask) {
        ++m_dwordIndex;
        m_mask = 1;
      }

      return *this;
    }

    operator bool() const {
      return m_bitIndex < m_owner.size();
    }

    bool
    operator!() const {
      return !(bool)*this;
    }

    bool
    operator!=(const TBitfieldIterator& rhs) {
      return m_bitIndex != rhs.m_bitIndex;
    }

    ReferenceType
    operator*() const {
      GE_ASSERT(static_cast<bool>(*this));
      return ReferenceType(m_owner.mData[m_dwordIndex], m_mask);
    }

   private:
    ArrayType m_owner;
    uint32 m_bitIndex;
    uint32 m_dwordIndex;
    uint32 m_mask;
  };

  /**
   * @brief Dynamically sized field that contains a sequential list of bits.
   *        The bits are compactly stored and allow for quick sequential
   *        searches (compared to single or multi-byte type sequential
   *        searches).
   */
  class Bitfield
  {
    static constexpr uint32 BITS_PER_DWORD = sizeof(uint32) * 8;
    static constexpr uint32 BITS_PER_DWORD_LOG2 = 5;

   public:
    using Iterator = TBitfieldIterator<false>;
    using ConstIterator = TBitfieldIterator<true>;

    /**
     * @brief Initializes the bitfield with enough storage for @p count bits
     *        and sets them to the initial value of @p value.
     */
    Bitfield(bool value = false, uint32 count = 0)
      : m_numBits(count) {
      if (count > 0) {
        realloc(count);
        reset(value);
      }
    }

    ~Bitfield() {
      if (m_data) {
        ge_free(m_data);
      }
    }

    Bitfield(const Bitfield& other)
      : m_numBits(other.m_numBits) {
      if (other.m_maxBits) {
        realloc(other.m_maxBits);

        const uint32 numBytes = Math::divideAndRoundUp(other.m_numBits,
                                                       BITS_PER_DWORD) *
                                sizeof(uint32);
        memcpy(m_data, other.m_data, numBytes);
      }
    }

    Bitfield(Bitfield&& other)
      : m_data(exchange(other.m_data, nullptr)),
        m_maxBits(exchange(other.m_maxBits, 0)),
        m_numBits(exchange(other.m_numBits, 0))
    {}

    Bitfield&
    operator=(const Bitfield& rhs) {
      if (&rhs != this) {
        clear(true);
        m_numBits = rhs.m_numBits;

        if (rhs.m_maxBits) {
          realloc(rhs.m_maxBits);

          const uint32 numBytes = Math::divideAndRoundUp(rhs.m_numBits,
                                                         BITS_PER_DWORD) *
                                  sizeof(uint32);
          memcpy(m_data, rhs.m_data, numBytes);
        }
      }

      return *this;
    }

    Bitfield&
    operator=(Bitfield&& rhs) {
      if (&rhs != this) {
        if (m_data) {
          ge_free(m_data);
        }

        m_data = exchange(rhs.m_data, nullptr);
        m_numBits = exchange(rhs.m_numBits, 0);
        m_maxBits = exchange(rhs.m_maxBits, 0);
      }

      return *this;
    }

    BitReference
    operator[](uint32 idx) {
      GE_ASSERT(idx < m_numBits);

      const uint32 bitMask = 1 << (idx & (BITS_PER_DWORD - 1));
      uint32& data = m_data[idx >> BITS_PER_DWORD_LOG2];

      return BitReference(data, bitMask);
    }

    BitReferenceConst
    operator[](uint32 idx) const {
      GE_ASSERT(idx < m_numBits);

      const uint32 bitMask = 1 << (idx & (BITS_PER_DWORD - 1));
      uint32& data = m_data[idx >> BITS_PER_DWORD_LOG2];

      return BitReferenceConst(data, bitMask);
    }

    /**
     * @brief Adds a new bit value to the end of the bitfield and returns the
     *        index of the added bit.
     */
    uint32
    add(bool value) {
      if (m_numBits >= m_maxBits) {
        //Grow
        const uint32 newMaxBits = m_maxBits + 4 * BITS_PER_DWORD + m_maxBits / 2;
        realloc(newMaxBits);
      }

      const uint32 index = m_numBits;
      ++m_numBits;

      (*this)[index] = value;
      return index;
    }

    /**
     * @brief Removes a bit at the specified index.
     */
    void
    remove(uint32 index) {
      GE_ASSERT(index < m_numBits);

      const uint32 dwordIndex = index >> BITS_PER_DWORD_LOG2;
      const uint32 mask = 1 << (index & (BITS_PER_DWORD - 1));

      const uint32 curDwordBits = m_data[dwordIndex];

      //Mask the dword we want to remove the bit from
      const uint32 firstHalfMask = mask - 1; //These stay the same
      
      //These get shifted so the removed bit gets moved outside the mask
      const uint32 secondHalfMask = ~firstHalfMask;

      m_data[dwordIndex] = (curDwordBits & firstHalfMask) |
                           (((curDwordBits >> 1) & secondHalfMask));

      //Grab the last bit from the next dword and put it as the last bit in the
      //current dword. Then shift the next dword and repeat until all following
      //dwords are processed.
      const uint32 lastDwordIndex = (m_numBits - 1) >> BITS_PER_DWORD_LOG2;
      for (uint32 i = dwordIndex; i < lastDwordIndex; i++) {
        //First bit from next dword goes at the end of the current dword
        m_data[i] |= (m_data[i + 1] & 0x1) << 31;

        //Following dword gets shifted, removing the bit we just moved
        m_data[i + 1] >>= 1;
      }

      --m_numBits;
    }

    /**
     * @brief Attempts to find the first non-zero bit in the field. Returns -1
     *        if all bits are zero or the field is empty.
     */
    uint32
    find(bool value) const {
      const uint32 mask = value ? 0 : NumLimit::MAX_UINT32;
      const uint32 numDWords = Math::divideAndRoundUp(m_numBits, BITS_PER_DWORD);

      for (uint32 i = 0; i < numDWords; ++i) {
        if (mask == m_data[i]) {
          continue;
        }

        const uint32 bits = value ? m_data[i] : ~m_data[i];
        const uint32 bitIndex = i * BITS_PER_DWORD +
                                Bitwise::leastSignificantBit(bits);

        if (bitIndex < m_numBits) {
          return bitIndex;
        }
      }

      return NumLimit::MAX_UINT32;
    }

    /**
     * @brief Counts the number of values in the bit field.
     */
    uint32
    count(bool value) const {
      //NOTE: Implement this faster via popcnt and similar instructions
      uint32 counter = 0;
      for (const auto& entry : *this) {
        if (entry == value) {
          ++counter;
        }
      }

      return counter;
    }

    /**
     * @brief Resets all the bits in the field to the specified value.
     */
    void
    reset(bool value = false) {
      if (0 == m_numBits) {
        return;
      }

      const int32_t mask = value ? 0xFF : 0;
      const uint32 numBytes = Math::divideAndRoundUp(m_numBits,
                                                     BITS_PER_DWORD) *
                              sizeof(uint32);
      memset(m_data, mask, numBytes);
    }

    /**
     * @brief Removes all the bits from the field. If @p free is true then the
     *        underlying memory buffers will be freed as well.
     */
    void
    clear(bool free = false) {
      m_numBits = 0;

      if (free) {
        if (m_data) {
          ge_free(m_data);
          m_data = nullptr;
        }

        m_maxBits = 0;
      }
    }

    /**
     * @brief Returns the number of bits in the bitfield
     */
    uint32
    size() const {
      return m_numBits;
    }

    /**
     * @brief Returns a non-const iterator pointing to the first bit in the
     *        bitfield.
     */
    Iterator
    begin() {
      return Iterator(*this, 0, 0, 1);
    }

    /**
     * @brief Returns a non-const iterator pointing past the last bit in the
     *        bitfield.
     */
    Iterator
    end() {
      uint32 bitIndex = m_numBits;
      uint32 dwordIndex = bitIndex >> BITS_PER_DWORD_LOG2;
      uint32 mask = 1 << (bitIndex & (BITS_PER_DWORD - 1));

      return Iterator(*this, bitIndex, dwordIndex, mask);
    }

    /**
     * @brief Returns a const iterator pointing to the first bit in the
     *        bitfield.
     */
    ConstIterator
    begin() const {
      return ConstIterator(*this, 0, 0, 1);
    }

    /**
     * @brief Returns a const iterator pointing past the last bit in the
     *        bitfield.
     */
    ConstIterator
    end() const {
      uint32 bitIndex = m_numBits;
      uint32 dwordIndex = bitIndex >> BITS_PER_DWORD_LOG2;
      uint32 mask = 1 << (bitIndex & (BITS_PER_DWORD - 1));

      return ConstIterator(*this, bitIndex, dwordIndex, mask);
    }

   private:
    template<bool CONST>
    friend class TBitfieldIterator;

    /**
     * @brief Reallocates the internal buffer making enough room for @p numBits
     *        (rounded to a multiple of DWORD).
     */
    void
    realloc(uint32 numBits) {
      numBits = Math::divideAndRoundUp(numBits, BITS_PER_DWORD) * BITS_PER_DWORD;

      if (numBits != m_maxBits) {
        GE_ASSERT(numBits > m_maxBits);

        const uint32 numDwords = Math::divideAndRoundUp(numBits, BITS_PER_DWORD);

        //NOTE: Eventually add support for custom allocators
        auto buffer = ge_allocN<uint32>(numDwords);
        if (m_data) {
          const uint32 numBytes = Math::divideAndRoundUp(m_maxBits,
                                                         BITS_PER_DWORD) *
                                  sizeof(uint32);
          memcpy(buffer, m_data, numBytes);
          ge_free(m_data);
        }

        m_data = buffer;
        m_maxBits = numBits;
      }
    }

    uint32* m_data = nullptr;
    uint32 m_maxBits = 0;
    uint32 m_numBits;
  };
}

namespace std {
  template<>
  inline void
  swap(geEngineSDK::BitReference& lhs, geEngineSDK::BitReference& rhs) {
    const bool temp = lhs;
    lhs = rhs;
    rhs = temp;
  }

  inline void
  swap(geEngineSDK::BitReference&& lhs, geEngineSDK::BitReference&& rhs) {
    const bool temp = lhs;
    lhs = rhs;
    rhs = temp;
  }
};
