// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

// Various types / enums / forward declarations which are broadly useful

namespace polyscope {

enum class NavigateStyle { Turntable = 0, Free, Planar, Arcball, None, FirstPerson };
enum class UpDir { XUp = 0, YUp, ZUp, NegXUp, NegYUp, NegZUp };
enum class FrontDir { XFront = 0, YFront, ZFront, NegXFront, NegYFront, NegZFront };
enum class BackgroundView { None = 0 };
enum class ProjectionMode { Perspective = 0, Orthographic };
enum class TransparencyMode { None = 0, Simple, Pretty };
enum class GroundPlaneMode { None, Tile, TileReflection, ShadowOnly };
enum class GroundPlaneHeightMode { Automatic = 0, Manual };
enum class BackFacePolicy { Identical, Different, Custom, Cull };

enum class PointRenderMode { Sphere = 0, Quad };
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE, CORNER };
enum class MeshShadeStyle { Smooth = 0, Flat, TriFlat };
enum class VolumeMeshElement { VERTEX = 0, EDGE, FACE, CELL };
enum class VolumeCellType { TET = 0, HEX };
enum class IsolineStyle { Stripe = 0, Contour };

enum class ImplicitRenderMode { SphereMarch, FixedStep };
enum class ImageOrigin { LowerLeft, UpperLeft };
enum class FilterMode { Nearest = 0, Linear };

enum class ParamCoordsType { UNIT = 0, WORLD }; // UNIT -> [0,1], WORLD -> length-valued
enum class ParamVizStyle {
  CHECKER = 0,
  GRID,
  LOCAL_CHECK,
  LOCAL_RAD,
  CHECKER_ISLANDS
}; // TODO add "UV" with test UV map

enum class ManagedBufferType {
  Float,
  Double,
  Vec2,
  Vec3,
  Vec4,
  Arr2Vec3,
  Arr3Vec3,
  Arr4Vec3,
  UInt32,
  Int32,
  UVec2,
  UVec3,
  UVec4
};


// What is the meaningful range of these values?
// Used to set meaningful colormaps
// STANDARD: [-inf, inf], zero does not mean anything special (ie, position)
// SYMMETRIC: [-inf, inf], zero is special (ie, net profit/loss)
// MAGNITUDE: [0, inf], zero is special (ie, length of a vector)
// CATEGORICAL: data is integers corresponding to labels, etc
enum class DataType { STANDARD = 0, SYMMETRIC, MAGNITUDE, CATEGORICAL };


}; // namespace polyscope
