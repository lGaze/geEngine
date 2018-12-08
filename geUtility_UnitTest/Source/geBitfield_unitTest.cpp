#include <vld.h>
#include <DirectXMath.h>

#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gtest/gtest.h>

#include <gePrerequisitesUtil.h>
#include <geBitfield.h>

using namespace geEngineSDK;

TEST(geBitfield, BitField) {
  static constexpr uint32 COUNT = 100;
  static constexpr uint32 EXTRA_COUNT = 32;

  Bitfield bitfield(true, COUNT);

  //Basic iteration
  uint32 i = 0;
  for (auto iter : bitfield) {
    EXPECT_TRUE(iter == true);
    ++i;
  }

  uint32 curCount = COUNT;
  EXPECT_EQ(i, curCount);

  //Dynamic addition
  bitfield.add(false);
  bitfield.add(false);
  bitfield.add(true);
  bitfield.add(false);
  curCount += 4;

  //Realloc
  curCount += EXTRA_COUNT;
  for (uint32 j = 0; j < 32; ++j) {
    bitfield.add(false);
  }

  EXPECT_EQ(bitfield.size(), curCount);

  EXPECT_FALSE(bitfield[COUNT + 0]);
  EXPECT_FALSE(bitfield[COUNT + 1]);
  EXPECT_TRUE(bitfield[COUNT + 2]);
  EXPECT_FALSE(bitfield[COUNT + 3]);

  //Modify during iteration
  i = 0;
  for (auto iter : bitfield) {
    if (i >= 50 && i <= 70) {
      iter = false;
    }
    ++i;
  }

  //Modify directly using []
  bitfield[5] = false;
  bitfield[6] = false;

  for (uint32 j = 50; j < 70; ++j) {
    EXPECT_FALSE(bitfield[j]);
  }

  EXPECT_FALSE(bitfield[5]);
  EXPECT_FALSE(bitfield[6]);

  //Removal
  bitfield.remove(10);
  bitfield.remove(10);
  curCount -= 2;

  for (uint32 j = 48; j < 68; ++j) {
    EXPECT_FALSE(bitfield[j]);
  }

  EXPECT_FALSE(bitfield[5]);
  EXPECT_FALSE(bitfield[6]);

  EXPECT_TRUE(bitfield.size() == curCount);

  //Find
  EXPECT_EQ(bitfield.find(true), 0);
  EXPECT_EQ(bitfield.find(false), 5);
}
