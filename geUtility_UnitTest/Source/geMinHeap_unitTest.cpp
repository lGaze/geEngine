#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gtest/gtest.h>

#include <gePrerequisitesUtil.h>
#include <geMinHeap.h>

using namespace geEngineSDK;

TEST(geMinHeap, MinHeap) {
  struct SomeElem
  {
    int32 a;
    int32 b;
  };

  MinHeap<SomeElem, int> m;
  m.resize(8);
  EXPECT_TRUE(m.valid());

  SomeElem elements;
  elements.a = 4;
  elements.b = 5;

  m.insert(elements, 10);
  EXPECT_EQ(m[0].key.a, 4);
  EXPECT_EQ(m[0].key.b, 5);
  EXPECT_EQ(m[0].value, 10);
  EXPECT_EQ(m.size(), 1U);

  int v = 11;
  m.insert(elements, v);
  EXPECT_EQ(m[1].key.a, 4);
  EXPECT_EQ(m[1].key.b, 5);
  EXPECT_EQ(m[1].value, 11);
  EXPECT_EQ(m.size(), 2U);

  SomeElem minKey;
  int minValue;

  m.minimum(minKey, minValue);
  EXPECT_EQ(minKey.a, 4);
  EXPECT_EQ(minKey.b, 5);
  EXPECT_EQ(minValue, 10);

  m.erase(elements, v);
  EXPECT_EQ(m.size(), 1U);
}
