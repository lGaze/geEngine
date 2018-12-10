#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include <gtest/gtest.h>

#include <gePrerequisitesUtil.h>
#include <geComplex.h>

using namespace geEngineSDK;

TEST(geComplex, Complex) {
  Complex<float> c(10.0f, 4.0f);
  EXPECT_FLOAT_EQ(c.real(), 10.0f);
  EXPECT_FLOAT_EQ(c.imag(), 4.0f);

  Complex<float> c2(15.0f, 5.0f);
  EXPECT_FLOAT_EQ(c2.real(), 15.0f);
  EXPECT_FLOAT_EQ(c2.imag(), 5.0f);

  Complex<float> c3 = c + c2;
  EXPECT_FLOAT_EQ(c3.real(), 25.0f);
  EXPECT_FLOAT_EQ(c3.imag(), 9.0f);

  Complex<float> c4 = c - c2;
  EXPECT_FLOAT_EQ(c4.real(), -5.0f);
  EXPECT_FLOAT_EQ(c4.imag(), -1.0f);

  Complex<float> c5 = c * c2;
  EXPECT_FLOAT_EQ(c5.real(), 130.0f);
  EXPECT_FLOAT_EQ(c5.imag(), 110.0f);

  Complex<float> c6 = c / c2;
  EXPECT_FLOAT_EQ(c6.real(), 0.680000007f);
  EXPECT_FLOAT_EQ(c6.imag(), 0.0399999991f);

  EXPECT_FLOAT_EQ(Complex<float>::abs(c), 10.7703295f);
  EXPECT_FLOAT_EQ(Complex<float>::arg(c), 0.380506366f);
  EXPECT_FLOAT_EQ(Complex<float>::norm(c), 116.0f);

  Complex<float> c7 = Complex<float>::conj(c);
  EXPECT_FLOAT_EQ(c7.real(), 10.0f);
  EXPECT_FLOAT_EQ(c7.imag(), -4.0f);
  c7 = 0;

  c7 = Complex<float>::polar(2.0f, 0.5f);
  EXPECT_FLOAT_EQ(c7.real(), 1.75516510f);
  EXPECT_FLOAT_EQ(c7.imag(), 0.958851099f);
  c7 = 0;

  c7 = Complex<float>::cos(c);
  EXPECT_FLOAT_EQ(c7.real(), -22.9135609f);
  EXPECT_FLOAT_EQ(c7.imag(), 14.8462915f);
  c7 = 0;

  c7 = Complex<float>::cosh(c);
  EXPECT_FLOAT_EQ(c7.real(), -7198.72949f);
  EXPECT_FLOAT_EQ(c7.imag(), -8334.84180f);
  c7 = 0;

  c7 = Complex<float>::exp(c);
  EXPECT_FLOAT_EQ(c7.real(), -14397.4580f);
  EXPECT_FLOAT_EQ(c7.imag(), -16669.6836f);
  c7 = 0;

  c7 = Complex<float>::log(c);
  EXPECT_FLOAT_EQ(c7.real(), 2.37679505f);
  EXPECT_FLOAT_EQ(c7.imag(), 0.380506366f);
  c7 = 0;

  c7 = Complex<float>::pow(c, 2.0);
  EXPECT_FLOAT_EQ(c7.real(), 84.0000000f);
  EXPECT_FLOAT_EQ(c7.imag(), 79.9999924f);
  c7 = 0;

  c7 = Complex<float>::sin(c);
  EXPECT_FLOAT_EQ(c7.real(), -14.8562555f);
  EXPECT_FLOAT_EQ(c7.imag(), -22.8981915f);
  c7 = 0;

  c7 = Complex<float>::sinh(c);
  EXPECT_FLOAT_EQ(c7.real(), -7198.72900f);
  EXPECT_FLOAT_EQ(c7.imag(), -8334.84277f);
  c7 = 0;

  c7 = Complex<float>::sqrt(c);
  EXPECT_FLOAT_EQ(c7.real(), 3.22260213f);
  EXPECT_FLOAT_EQ(c7.imag(), 0.620616496f);
  c7 = 0;
}
