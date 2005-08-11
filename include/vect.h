// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2002-2005 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log$
// Revision 1.1  2005/08/11 19:56:39  smite-meister
// template classes
//
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Algebraic three-vector template class.

#ifndef vect_h
#define vect_h 1

//#include <math.h>

/// \brief Simple template class for three-vectors
///
/// Simple means that the class does not support the automatic promotion
/// of the left operand in binary operations, i.e. no templatized friend methods.
/// This makes the binary operations unsymmetric, the left operand dictates the return type.
template<typename T>
class vec_t
{
public:
  typedef vec_t<T> T_this; ///< type of *this
  T x, y, z; ///< the cartesian components of the vector

  /// constructors
  explicit inline vec_t()                 { x = y = z = 0; }
  inline vec_t(const vec_t &v)            { x = v.x; y = v.y; z = v.z; }
  explicit inline vec_t(T nx, T ny, T nz) { x = nx; y = ny; z = nz; }

  /// component extraction (also for assignment)
  //inline T& operator[](unsigned n) { return c[n]; }

  /// assignment
  template<typename U>
  inline T_this& operator= (const vec_t<U>& v) { x = v.x; y = v.y; z = v.z; return *this; }
  template<typename U>
  inline T_this& operator+=(const vec_t<U>& v) { x += v.x; y += v.y; z += v.z; return *this; }
  template<typename U>
  inline T_this& operator-=(const vec_t<U>& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
  template<typename U>
  inline T_this& operator*=(const U& a) { x *= a; y *= a; z *= a;  return *this; }
  template<typename U>
  inline T_this& operator/=(const U& a) { x /= a; y /= a; z /= a;  return *this; }

  /// unary minus
  inline T_this operator-() const { return T_this(-x, -y, -z); }

  /// utility methods
  T Norm()  const { return sqrt((x * x) + (y * y) + (z * z)); }
  T Norm2() const { return (x * x) + (y * y) + (z * z); }
  template<typename U>
  T_this Project(const vec_t<U>& v) const { return v*(((*this)*v)/v.Norm2()); }

  /// polar coordinate methods
  T Theta() const { return atan2(sqrt((x * x) + (y * y)), z); }
  T Phi()   const { return atan2(y, x); }

  /// output method
  void Print() const { std::cout << "(" << x << ',' <<  y << ',' << z << ")"; }

  /// Addition and subtraction
  template<typename U>
  inline T_this operator+(const vec_t<U>& v) const
  {
    //T_this res(*this);  res.x += v.x;  res.y += v.y;  res.z += v.z;  return res;
    return T_this(x + v.x, y + v.y, z + v.z);
  }

  template<typename U>
  inline T_this operator-(const vec_t<U>& v) const
  {
    //T_this res(*this);  res.x -= v.x;  res.y -= v.y;  res.z -= v.z;  return res;
    T_this(x - v.x, y - v.y, z - v.z); 
  }

  /// scalar multiplication (only from the right!)
  template<typename U>
  inline T_this operator*(const U& a) const
  {
    //T_this res(*this);  res.x *= a;  res.y *= a;  res.z *= a;  return res;
    //T_this res;  res.x = x*a;  res.y = y*a;  res.z = z*a;  return res;
    return T_this(x*a, y*a, z*a);
  }

  template<typename U>
  inline T_this operator/(const U& a) const
  {
    //T_this res(*this);  res.x /= a;  res.y /= a;  res.z /= a;  return res;
    return T_this(x/a, y/a, z/a);
  }

  /// comparisons
  template<typename U>
  inline bool operator==(const vec_t<U>& v) const { return (x == v.x && y == v.y && z == v.z); }
  template<typename U>
  inline bool operator!=(const vec_t<U>& v) const { return !(*this == v); }

  /// inner product
  template<typename U>
  inline T operator*(const vec_t<U>& v)
  {
    return (x * v.x) + (y * v.y) + (z * v.z);
  }

  /// cross (outer) product
  template<typename U>
  inline T_this Cross(const vec_t<U>& v)
  {
    return vec_t<T>(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
  }
};

/*
template<typename T>
vec_t<T> PolarVec(const T& radius, const T& theta, const T& phi)
{
  T rst = radius * sin(theta);
  return vec_t<T>(rst*cos(phi), rst*sin(phi), radius*cos(theta));
};
*/

#endif
