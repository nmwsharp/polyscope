// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

// Various types / enums / forward declarations which are broadly useful

namespace polyscope {

enum class NavigateStyle { Turntable = 0, Free, Planar, Arcball };
enum class UpDir { XUp = 0, YUp, ZUp, NegXUp, NegYUp, NegZUp };
enum class FrontDir { XFront = 0, YFront, ZFront, NegXFront, NegYFront, NegZFront };
enum class BackgroundView { None = 0 };
enum class ProjectionMode { Perspective = 0, Orthographic };
enum class TransparencyMode { None = 0, Simple, Pretty };
enum class GroundPlaneMode { None, Tile, TileReflection, ShadowOnly };
enum class BackFacePolicy { Identical, Different, Custom, Cull };

enum class PointRenderMode { Sphere = 0, Quad };
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE, CORNER };
enum class MeshShadeStyle { Smooth = 0, Flat, TriFlat };
enum class VolumeMeshElement { VERTEX = 0, EDGE, FACE, CELL };
enum class VolumeCellType { TET = 0, HEX };

enum class ImplicitRenderMode { SphereMarch, FixedStep };
enum class ImageOrigin { LowerLeft, UpperLeft };

enum class ParamCoordsType { UNIT = 0, WORLD };                         // UNIT -> [0,1], WORLD -> length-valued
enum class ParamVizStyle { CHECKER = 0, GRID, LOCAL_CHECK, LOCAL_RAD }; // TODO add "UV" with test UV map

// What is the meaningful range of these values?
// Used to set meaningful colormaps
// STANDARD: [-inf, inf], zero does not mean anything special (ie, position)
// SYMMETRIC: [-inf, inf], zero is special (ie, net profit/loss)
// MAGNITUDE: [0, inf], zero is special (ie, length of a vector)
enum class DataType { STANDARD = 0, SYMMETRIC, MAGNITUDE };


}; // namespace polyscope
