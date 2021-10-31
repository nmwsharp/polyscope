// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

// Various types / enums / forward declarations which are broadly useful

namespace polyscope {

enum class BackgroundView { None = 0 };
enum class ProjectionMode { Perspective = 0, Orthographic };
enum class TransparencyMode { None = 0, Simple, Pretty };
enum class GroundPlaneMode { None, Tile, TileReflection, ShadowOnly };
enum class BackFacePolicy { Identical, Different, Custom, Cull };
enum class ShadeStyle { FLAT = 0, SMOOTH };

enum class PointRenderMode { Sphere = 0, Quad};
enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE, CORNER };
enum class VolumeMeshElement { VERTEX = 0, EDGE, FACE, CELL };
enum class VolumeCellType { TET = 0, HEX };

}; // namespace polyscope
