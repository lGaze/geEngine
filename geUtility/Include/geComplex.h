/*****************************************************************************/
/**
 * @file    geComplex.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/12/10
 * @brief   Complex number type. One real and one imaginary part.
 *
 * Complex number type. One real and one imaginary part.
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

namespace geEngineSDK {
  using std::ostream;

  template<class Type>
  class Complex
  {
   public:
    Complex() = default;

    Complex(const Type& r, const Type& i)
      : m_real(r),
        m_imag(i)
    {}

    Complex(const Complex& other)
      : m_real(other.real()),
        m_imag(other.imag())
    {}

    Complex<Type>&
    operator=(const Type& other) {
      m_real = other;
      m_imag = Type();
      return *this;
    }

    Complex<Type>&
    operator+=(const Type& other) {
      m_real += other;
      return *this;
    }

    Complex<Type>&
    operator-=(const Type& other) {
      m_real -= other;
      return *this;
    }

    Complex<Type>&
    operator*= (const Type& other) {
      m_real *= other;
      m_imag *= other;
      return *this;
    }

    Complex<Type>&
    operator/=(const Type& other) {
      m_real /= other;
      m_imag /= other;
      return *this;
    }

    Complex<Type>&
    operator=(const Complex<Type>& other) {
      m_real = other.real();
      m_imag = other.imag();
      return *this;
    }

    Complex<Type>&
    operator+=(const Complex<Type>& other) {
      m_real += other.real();
      m_imag += other.imag();
      return *this;
    }

    Complex<Type>&
    operator-=(const Complex<Type>& other) {
      m_real -= other.real();
      m_imag -= other.imag();
      return *this;
    }

    Complex<Type>&
    operator*=(const Complex<Type>& other) {
      const Type r = m_real * other.real() - m_imag * other.imag();
      m_imag = m_real * other.imag() + m_imag * other.real();
      m_real = r;
      return *this;
    }

    Complex<Type>&
    operator/=(const Complex<Type>& other) {
      const Type r = m_real * other.real() + m_imag * other.imag();
      const Type n = Complex::norm(other);
      m_imag = (m_imag * other.real() - m_real * other.imag()) / n;
      m_real = r / n;
      return *this;
    }

    Complex<Type>
    operator+(const Type& other) const {
      return Complex(m_real + other, m_imag);
    }

    Complex<Type>
    operator-(const Type& other) const {
      return Complex(m_real - other, m_imag);
    }

    Complex<Type>
    operator*(const Type& other) const {
      return Complex(m_real * other, m_imag);
    }

    Complex<Type>
    operator/(const Type& other) const {
      return Complex(m_real / other, m_imag);
    }

    Complex<Type>
    operator+(const Complex<Type>& other) const {
      return Complex(m_real + other.real(), m_imag + other.imag());
    }

    Complex<Type>
    operator-(const Complex<Type>& other) const {
      return Complex(m_real - other.real(), m_imag - other.imag());
    }

    Complex<Type>
    operator*(const Complex<Type>& other) const {
      Complex<Type> res = *this;
      res *= other;
      return res;
    }

    Complex<Type>
    operator/(const Complex<Type>& other) const {
      Complex<Type> res = *this;
      res /= other;
      return res;
    }

    bool
    operator==(const Complex<Type>& other) const {
      return m_real == other.real() && m_imag == other.imag();
    }

    bool
    operator==(const Type& other) const {
      return m_real == other && m_imag == Type();
    }

    bool
    operator!=(const Complex<Type>& other) const {
      return m_real != other.real() || m_imag != other.imag();
    }

    bool
    operator!=(const Type& other) const {
      return m_real != other || m_imag != Type();
    }

    Type&
    real() {
      return m_real;
    }

    Type&
    imag() {
      return m_imag;
    }

    const Type&
    real() const {
      return m_real;
    }

    const Type&
    imag() const {
      return m_imag;
    }

    static Type
    abs(const Complex<Type>& other) {
      Type x = other.real();
      Type y = other.imag();
      const Type s = Math::max(Math::abs(x), Math::abs(y));
      if (Type() == s) {
        return s;
      }

      x /= s;
      y /= s;

      return s * Math::sqrt(Math::square(x) + Math::square(y));
    }

    static Type
    arg(const Complex<Type>& other) {
      return Type(Math::atan2(other.imag(), other.real()).valueRadians());
    }

    static Type
    norm(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();

      return Math::square(x) + Math::square(y);
    }

    static Complex<Type>
    conj(const Complex<Type>& other) {
      return Complex(other.real(), -other.imag());
    }

    static Complex<Type>
    polar(const Type& r, const Type& t = 0) {
      return Complex(r * Math::cos(t), r * Math::sin(t));
    }

    static Complex<Type>
    cos(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();
      return Complex(Math::cos(x) * Math::cosh(y), -Math::sin(x) * Math::sinh(y));
    }

    static Complex<Type>
    cosh(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();
      return Complex(Math::cosh(x) * Math::cos(y), Math::sinh(x) * Math::sin(y));
    }

    static Complex<Type>
    exp(const Complex<Type>& other) {
      return Complex::polar(Math::exp(other.real()), other.imag());
    }

    static Complex<Type>
    log(const Complex<Type>& other) {
      return Complex(Math::logE(Complex::abs(other)), Complex::arg(other));
    }

    static Complex<Type>
    log10(const Complex<Type>& other) {
      return Complex::log(other) / Math::logE(Type(10));
    }

    static Complex<Type>
    pow(const Complex<Type>& other, const Type& i) {
      if (other.imag() == Type() && other.real() > Type()) {
        return Complex(Math::pow(other.real(), i), other.imag());
      }

      Complex<Type> t = Complex::log(other);
      return Complex::polar(Math::exp(i * t.real()), i * t.imag());
    }

    static Complex<Type>
    pow(const Complex<Type>& x, const Complex<Type>& y) {
      return Complex::exp(y * Complex::log(x));
    }

    static Complex<Type>
    pow(const Type& i, const Complex<Type>& other) {
      return i > Type() ?
        Complex::polar(Math::pow(i, other.real()), other.imag() * Math::logE(i)) :
        Complex::pow(Complex(i, Type()), other);
    }

    static Complex<Type>
    sin(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();
      return Complex(Math::sin(x) * Math::cosh(y), Math::cos(x) * Math::sinh(y));
    }

    static Complex<Type>
    sinh(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();
      return Complex(Math::sinh(x) * Math::cos(y), Math::cosh(x) * Math::sin(y));
    }

    static Complex<Type>
    sqrt(const Complex<Type>& other) {
      const Type x = other.real();
      const Type y = other.imag();

      if (Type() == x) {
        Type t = Math::sqrt(Math::abs(y) / 2);
        return Complex(t, y < Type() ? -t : t);
      }
      else {
        Type t = Math::sqrt(2 * (Complex::abs(other) + Math::abs(x)));
        Type u = t / 2;
        return x > Type() ? Complex(u, y / t)
          : Complex(Math::abs(y) / t, y < Type() ? -u : u);
      }
    }

    static Complex<Type>
    tan(const Complex<Type>& other) {
      return Complex::sin(other) / Complex::cos(other);
    }

    static Complex<Type>
    tanh(const Complex<Type>& other) {
      return Complex::sinh(other) / Complex::cosh(other);
    }

    friend ostream&
    operator<<(ostream& os, const Complex<Type> other) {
      return os << other.real() << ", " << other.imag();
    }

   private:
    Type m_real;
    Type m_imag;
  };
}
