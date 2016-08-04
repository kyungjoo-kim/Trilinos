// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions: Alejandro Mota (amota@sandia.gov)
//
// ************************************************************************
// @HEADER

#if !defined(Intrepid_MiniTensor_Utilities_i_h)
#define Intrepid_MiniTensor_Utilities_i_h

#include <cfloat>
#include <cmath>
#include <limits>

namespace Intrepid {

//
//
//
template<typename T>
void
swap(T & a, T & b)
{
  // Guard against the same memory location.
  if (&a == &b) return;

  // XOR algorithm
  a ^= b;
  b ^= a;
  a ^= b;

  return;
}

//
//
//
template<typename T>
T
max(const T & a, const T & b)
{
  return a > b ? a : b;
}

//
//
//
template<typename T>
T
min(const T & a, const T & b)
{
  return a < b ? a : b;
}

//
// Sign function
//
template<typename T>
inline
int
sgn(T const & s)
{
  return (T(0) < s) - (s < T(0));
}

//
// Copysign function
//
template<typename T>
inline
T
copysign(T const & a, T const & b)
{
  return b >= 0 ? std::abs(a) : -std::abs(a);
}

//
// NaN function. Necessary to choose the proper underlying NaN
// for non-floating-point types.
// Assumption: non-floating-point types have a typedef that
// determines the underlying floating-point type.
//
template<typename T>
inline
typename Sacado::ScalarType<T>::type
not_a_number()
{
  return
      std::numeric_limits<typename Sacado::ScalarType<T>::type>::quiet_NaN();
}

//
// Machine epsilon function. Necessary to choose the proper underlying
// machine epsilon for non-floating-point types.
// Assumption: non-floating-point types have a typedef that
// determines the underlying floating-point type.
//
template<typename T>
inline
typename Sacado::ScalarType<T>::type
machine_epsilon()
{
  return
      std::numeric_limits<typename Sacado::ScalarType<T>::type>::epsilon();
}

//
// Number of digits for integer types.
//
template<typename T>
Index
num_digits()
{
  return 0;
}

template<>
Index
num_digits<Index>()
{
  return INDEX_SIZE;
}

template<>
Index
num_digits<LongIndex>()
{
  return LONG_INDEX_SIZE;
}

//
// The circle constant
//
template<typename T>
typename Sacado::ScalarType<T>::type
tau()
{
  typedef typename Sacado::ScalarType<T>::type S;
  return static_cast<S>(
      6.283185307179586476925286766559005768394338798750211641949889185
  );
}

//
// Random number generation. Teuchos [-1,1]
//
template<typename T>
inline
typename Sacado::ScalarType<T>::type
random()
{
  typedef typename Sacado::ScalarType<T>::type S;
  return Teuchos::ScalarTraits<S>().random();
}

//
// Uniform [0,1] random number generation.
//
template<typename T>
inline
typename Sacado::ScalarType<T>::type
random_uniform()
{
  typedef typename Sacado::ScalarType<T>::type S;
  return static_cast<S>(0.5 * random<S>() + 0.5);
}

//
// Normal N(0,1) random number generation.
//
template<typename T>
inline
typename Sacado::ScalarType<T>::type
random_normal()
{
  typedef typename Sacado::ScalarType<T>::type S;

  S const
  R = random_uniform<S>();

  S const
  Theta = tau<S>() * random_uniform<S>();

  return static_cast<S>(std::sqrt(-2.0 * std::log(R)) * cos(Theta));
}

//
// Fill in all levels of AD with specified constant.
//
template<typename T>
void
fill_AD(
    typename enable_if<is_same<T, typename ScalarType<T>::type>::value, T>::type & x,
    typename ScalarType<T>::type const c)
{
  x = c;
  return;
}

template<typename T>
void
fill_AD(
    typename enable_if<!is_same<T, typename ScalarType<T>::type>::value, T>::type & x,
    typename ScalarType<T>::type const c)
{
  auto const
  order = x.size();

  // No AD info. Nothing to do.
  if (order == 0) return;

  using S = typename Sacado::ValueType<T>::type;

  for (auto i = 0; i < order; ++i) {
    fill_AD<S>(x.fastAccessDx(i), c);
  }

  return;
}

//
// Compute a non-negative integer power by binary manipulation.
//
template<typename T>
T
integer_power(T const & X, Index const exponent)
{
  if (X == 0 || X == 1) return X;

  switch (exponent) {
    default:
      break;
    case 0:
      return 1;
      break;
    case 1:
      return X;
      break;
    case 2:
      return X * X;
      break;
    case 3:
      return X * X * X;
      break;
    case 4:
    {
      T const Y = X * X;
      return Y * Y;
    }
    break;
  }

  Index const
  rightmost_bit = 1;

  Index const
  number_digits = num_digits<Index>();

  Index const
  leftmost_bit = rightmost_bit << (number_digits - 1);

  Index
  t = 0;

  for (Index j = 0; j < number_digits; ++j) {

    if (((exponent << j) & leftmost_bit) != 0) {

      t = number_digits - j - 1;
      break;

    }

  }

  T
  P = X;

  Index
  i = 0;

  Index
  m = exponent;

  while ((m & rightmost_bit) == 0) {
    P = P * P;
    ++i;
    m = m >> 1;
  }

  T
  Y = P;

  for (Index j = i + 1; j <= t; ++j) {
    P = P * P;

    if (((exponent >> j) & rightmost_bit) != 0) {
      Y = Y * P;
    }
  }

  return Y;
}

//
// Utility for Kronecker delta in 2D
//
template<typename T>
inline
T
kronecker_delta(Index const i, Index const j)
{
  assert(0 <= i && i < 2);
  assert(0 <= j && j < 2);

  if (i == j) return T(1);

  return T(0);
}

//
// Utility for Kronecker delta in 3D
//
template<typename T>
inline
T
kronecker_delta(Index const i, Index const j, Index const k)
{
  assert(0 <= i && i < 3);
  assert(0 <= j && j < 3);
  assert(0 <= k && k < 3);

  if (i == j && j == k) return T(1);

  return T(0);
}

//
// Utility for Kronecker delta in 4D
//
template<typename T>
inline
T
kronecker_delta(Index const i, Index const j, Index const k, Index const l)
{
  assert(0 <= i && i < 4);
  assert(0 <= j && j < 4);
  assert(0 <= k && k < 4);
  assert(0 <= l && l < 4);

  if (i == j && j == k && k == l) return T(1);

  return T(0);
}

//
// Utility for Levi-Civita/permutation/alternating symbol in 2D
//
template<typename T>
inline
T
levi_civita(Index const i, Index const j)
{
  assert(0 <= i && i < 2);
  assert(0 <= j && j < 2);

  if (i == 0 && j == 1) return T(1);

  if (i == 1 && j == 0) return T(-1);

  return T(0);
}

//
// Utility for Levi-Civita/permutation/alternating symbol in 3D
//
template<typename T>
inline
T
levi_civita(Index const i, Index const j, Index const k)
{
  assert(0 <= i && i < 3);
  assert(0 <= j && j < 3);
  assert(0 <= k && k < 3);

  if (i == 0 && j == 1 && k == 2) return T(1);
  if (i == 1 && j == 2 && k == 0) return T(1);
  if (i == 2 && j == 0 && k == 1) return T(1);

  if (i == 2 && j == 1 && k == 0) return T(-1);
  if (i == 0 && j == 2 && k == 1) return T(-1);
  if (i == 1 && j == 0 && k == 2) return T(-1);

  return T(0);
}

//
// Utility for Levi-Civita/permutation/alternating symbol in 4D
//
template<typename T>
inline
T
levi_civita(Index const i, Index const j, Index const k, Index const l)
{
  assert(0 <= i && i < 4);
  assert(0 <= j && j < 4);
  assert(0 <= k && k < 4);
  assert(0 <= l && l < 4);

  if (i == 0 && j == 1 && k == 2 && l == 3) return T(1);
  if (i == 1 && j == 2 && k == 3 && l == 0) return T(1);
  if (i == 2 && j == 3 && k == 0 && l == 1) return T(1);
  if (i == 3 && j == 0 && k == 1 && l == 2) return T(1);

  if (i == 3 && j == 2 && k == 1 && l == 0) return T(-1);
  if (i == 0 && j == 3 && k == 2 && l == 1) return T(-1);
  if (i == 1 && j == 0 && k == 3 && l == 2) return T(-1);
  if (i == 2 && j == 1 && k == 0 && l == 3) return T(-1);

  return T(0);
}

} // namespace Intrepid

#endif // Intrepid_MiniTensor_Utilities_i_h
