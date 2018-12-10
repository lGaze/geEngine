#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gtest/gtest.h>

#include <gePrerequisitesUtil.h>
#include <geDynArray.h>

using namespace geEngineSDK;

TEST(geDynArray, DynArray) {
  struct SomeElem
  {
    int32 a = 10;
    int32 b = 0;
  };

  //Make sure initial construction works
  DynArray<SomeElem> v(4);
  EXPECT_EQ(v.size(), 4U);
  EXPECT_EQ(v.capacity(), 4U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[3].a, 10);
  EXPECT_EQ(v[3].b, 0);

  //Add an element
  v.add({ 3, 4 });
  EXPECT_EQ(v.size(), 5U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[3].a, 10);
  EXPECT_EQ(v[3].b, 0);
  EXPECT_EQ(v[4].a, 3);
  EXPECT_EQ(v[4].b, 4);

  //Make a copy
  DynArray<SomeElem> v2 = v;
  EXPECT_EQ(v2.size(), 5U);
  EXPECT_EQ(v2[0].a, 10);
  EXPECT_EQ(v2[3].a, 10);
  EXPECT_EQ(v2[3].b, 0);
  EXPECT_EQ(v2[4].a, 3);
  EXPECT_EQ(v2[4].b, 4);

  //Pop an element
  v2.pop();
  EXPECT_EQ(v2.size(), 4U);
  EXPECT_EQ(v2[0].a, 10);
  EXPECT_EQ(v2[3].a, 10);
  EXPECT_EQ(v2[3].b, 0);

  //Remove an element
  v.remove(2);
  EXPECT_EQ(v.size(), 4U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[2].a, 10);
  EXPECT_EQ(v[3].a, 3);
  EXPECT_EQ(v[3].b, 4);

  //Insert an element
  v.insert(v.begin() + 2, { 99, 100 });
  EXPECT_EQ(v.size(), 5U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[2].a, 99);
  EXPECT_EQ(v[3].a, 10);
  EXPECT_EQ(v[4].a, 3);
  EXPECT_EQ(v[4].b, 4);

  //Insert a list
  v.insert(v.begin() + 1, { { 55, 100 }, { 56, 100 }, { 57, 100 } });
  EXPECT_EQ(v.size(), 8U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[1].a, 55);
  EXPECT_EQ(v[2].a, 56);
  EXPECT_EQ(v[3].a, 57);
  EXPECT_EQ(v[4].a, 10);
  EXPECT_EQ(v[5].a, 99);
  EXPECT_EQ(v[6].a, 10);
  EXPECT_EQ(v[7].a, 3);
  EXPECT_EQ(v[7].b, 4);

  //Erase a range of elements
  v.erase(v.begin() + 2, v.begin() + 5);
  EXPECT_EQ(v.size(), 5U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[1].a, 55);
  EXPECT_EQ(v[2].a, 99);
  EXPECT_EQ(v[3].a, 10);
  EXPECT_EQ(v[4].a, 3);
  EXPECT_EQ(v[4].b, 4);

  //Insert a range
  v.insert(v.begin() + 1, v2.begin() + 1, v2.begin() + 3);
  EXPECT_EQ(v.size(), 7U);
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[1].a, 10);
  EXPECT_EQ(v[2].a, 10);
  EXPECT_EQ(v[3].a, 55);
  EXPECT_EQ(v[4].a, 99);
  EXPECT_EQ(v[5].a, 10);
  EXPECT_EQ(v[6].a, 3);
  EXPECT_EQ(v[6].b, 4);

  //Shrink capacity
  v.shrink();
  EXPECT_EQ(v.size(), v.capacity());
  EXPECT_EQ(v[0].a, 10);
  EXPECT_EQ(v[1].a, 10);
  EXPECT_EQ(v[2].a, 10);
  EXPECT_EQ(v[3].a, 55);
  EXPECT_EQ(v[4].a, 99);
  EXPECT_EQ(v[5].a, 10);
  EXPECT_EQ(v[6].a, 3);
  EXPECT_EQ(v[6].b, 4);

  //Move it
  DynArray<SomeElem> v3 = std::move(v2);
  EXPECT_EQ(v2.size(), 0U);
  EXPECT_EQ(v3.size(), 4U);
  EXPECT_EQ(v3[0].a, 10);
  EXPECT_EQ(v3[3].a, 10);
  EXPECT_EQ(v3[3].b, 0);
}
