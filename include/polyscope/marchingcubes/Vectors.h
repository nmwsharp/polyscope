#pragma once

#ifndef VECTORS_H
#define VECTORS_H

namespace polyscope {
namespace marchingcubes {
// File Name: Vectors.h
// Last Modified: 7/8/2000
// Author: Raghavendra Chandrashekara
// Email: rc99@doc.ic.ac.uk, rchandrashekara@hotmail.com
//
// Description: This file contains some useful structures.

// Edited and ported to polyscope by Christopher Yu.

typedef float POINT3D[3];
typedef float VECTOR3D[3];

struct POINT3DXYZ {
  float x, y, z;
  friend POINT3DXYZ operator+(const POINT3DXYZ& pt3dPoint1, const POINT3DXYZ& pt3dPoint2);
  friend POINT3DXYZ operator-(const POINT3DXYZ& pt3dPoint1, const POINT3DXYZ& pt3dPoint2);
  friend POINT3DXYZ operator*(const POINT3DXYZ& pt3dPoint, float fScale);
  friend POINT3DXYZ operator*(float fScale, const POINT3DXYZ& pt3dPoint);
  friend POINT3DXYZ operator/(const POINT3DXYZ& pt3dPoint, float fScale);
  friend POINT3DXYZ& operator*=(POINT3DXYZ& pt3dPoint, float fScale);
  friend POINT3DXYZ& operator/=(POINT3DXYZ& pt3dPoint, float fScale);
  friend POINT3DXYZ& operator+=(POINT3DXYZ& pt3dPoint1, const POINT3DXYZ& pt3dPoint2);
  friend POINT3DXYZ& operator-=(POINT3DXYZ& pt3dPoint1, const POINT3DXYZ& pt3dPoint2);
};

typedef POINT3DXYZ VECTOR3DXYZ;

} // namespace marchingcubes
} // namespace polyscope

#endif // VECTORS_H
