// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

#include "polyscope/utilities.h"

// Various types / enums / forward declarations which are broadly useful

// The string templates allow to/from string like:
//   enum_to_string(ProjectionMode::Perspective)
//   enum_from_string<ProjectionMode>("Perspective")
//   bool success = try_enum_from_string<ProjectionMode>("Perspective", val_out);
// see utilities.h for implementation

// clang-format off

namespace polyscope {

// Navigate Style
enum class NavigateStyle { Turntable = 0, Free, Planar, Arcball, None, FirstPerson };
POLYSCOPE_DEFINE_ENUM_NAMES(NavigateStyle,
    {NavigateStyle::Turntable, "Turntable"},
    {NavigateStyle::Free, "Free"},
    {NavigateStyle::Planar, "Planar"},
    {NavigateStyle::Arcball, "Arcball"},
    {NavigateStyle::None, "None"},
    {NavigateStyle::FirstPerson, "First Person"}
);

// Up Direction
enum class UpDir { XUp = 0, YUp, ZUp, NegXUp, NegYUp, NegZUp };
POLYSCOPE_DEFINE_ENUM_NAMES(UpDir,
    {UpDir::XUp, "X Up"},
    {UpDir::YUp, "Y Up"},
    {UpDir::ZUp, "Z Up"},
    {UpDir::NegXUp, "NegX Up"},
    {UpDir::NegYUp, "NegY Up"},
    {UpDir::NegZUp, "NegZ Up"},
);

enum class FrontDir { XFront = 0, YFront, ZFront, NegXFront, NegYFront, NegZFront };
POLYSCOPE_DEFINE_ENUM_NAMES(FrontDir,
    {FrontDir::XFront, "X Forward"},
    {FrontDir::YFront, "Y Forward"},
    {FrontDir::ZFront, "Z Forward"},
    {FrontDir::NegXFront, "NegX Forward"},
    {FrontDir::NegYFront, "NegY Forward"},
    {FrontDir::NegZFront, "NegZ Forward"},
);

enum class BackgroundView { None = 0 };
POLYSCOPE_DEFINE_ENUM_NAMES(BackgroundView,
    {BackgroundView::None, "None"}
);


// Projection Mode
enum class ProjectionMode { Perspective = 0, Orthographic };
POLYSCOPE_DEFINE_ENUM_NAMES(ProjectionMode,
    {ProjectionMode::Perspective, "Perspective"},
    {ProjectionMode::Orthographic, "Orthographic"}
);

enum class ViewRelativeMode { CenterRelative = 0, LengthRelative };
POLYSCOPE_DEFINE_ENUM_NAMES(ViewRelativeMode,
    {ViewRelativeMode::CenterRelative, "Center Relative"},
    {ViewRelativeMode::LengthRelative, "Length Relative"}
);

enum class TransparencyMode { None = 0, Simple, Pretty };
POLYSCOPE_DEFINE_ENUM_NAMES(TransparencyMode,
    {TransparencyMode::None, "None"},
    {TransparencyMode::Simple, "Simple"},
    {TransparencyMode::Pretty, "Pretty"}
);

enum class GroundPlaneMode { None, Tile, TileReflection, ShadowOnly };
POLYSCOPE_DEFINE_ENUM_NAMES(GroundPlaneMode,
    {GroundPlaneMode::None, "None"},
    {GroundPlaneMode::Tile, "Tile"},
    {GroundPlaneMode::TileReflection, "Tile Reflection"},
    {GroundPlaneMode::ShadowOnly, "Shadow Only"}
);

enum class GroundPlaneHeightMode { Automatic = 0, Manual };
POLYSCOPE_DEFINE_ENUM_NAMES(GroundPlaneHeightMode,
    {GroundPlaneHeightMode::Automatic, "Automatic"},
    {GroundPlaneHeightMode::Manual, "Manual"}
);

enum class BackFacePolicy { Identical, Different, Custom, Cull };
POLYSCOPE_DEFINE_ENUM_NAMES(BackFacePolicy,
    {BackFacePolicy::Identical, "Identical"},
    {BackFacePolicy::Different, "Different"},
    {BackFacePolicy::Custom, "Custom"},
    {BackFacePolicy::Cull, "Cull"}
);

enum class LimitFPSMode { IgnoreLimits = 0, BlockToHitTarget, SkipFramesToHitTarget };
POLYSCOPE_DEFINE_ENUM_NAMES(LimitFPSMode,
    {LimitFPSMode::IgnoreLimits, "Ignore Limits"},
    {LimitFPSMode::BlockToHitTarget, "Block To Hit Target"},
    {LimitFPSMode::SkipFramesToHitTarget, "Skip Frames To Hit Target"}
);

enum class PointRenderMode { Sphere = 0, Quad };
POLYSCOPE_DEFINE_ENUM_NAMES(PointRenderMode,
    {PointRenderMode::Sphere, "Sphere"},
    {PointRenderMode::Quad, "Quad"}
);

enum class MeshElement { VERTEX = 0, FACE, EDGE, HALFEDGE, CORNER };
POLYSCOPE_DEFINE_ENUM_NAMES(MeshElement,
    {MeshElement::VERTEX, "Vertex"},
    {MeshElement::FACE, "Face"},
    {MeshElement::EDGE, "Edge"},
    {MeshElement::HALFEDGE, "Half-Edge"},
    {MeshElement::CORNER, "Corner"}
);

enum class MeshShadeStyle { Smooth = 0, Flat, TriFlat };
POLYSCOPE_DEFINE_ENUM_NAMES(MeshShadeStyle,
    {MeshShadeStyle::Smooth, "Smooth"},
    {MeshShadeStyle::Flat, "Flat"},
    {MeshShadeStyle::TriFlat, "Tri-Flat"}
);

enum class MeshSelectionMode { Auto = 0, VerticesOnly, FacesOnly };
POLYSCOPE_DEFINE_ENUM_NAMES(MeshSelectionMode,
    {MeshSelectionMode::Auto, "Auto"},
    {MeshSelectionMode::VerticesOnly, "Vertices Only"},
    {MeshSelectionMode::FacesOnly, "Faces Only"}
);

enum class CurveNetworkElement { NODE = 0, EDGE };
POLYSCOPE_DEFINE_ENUM_NAMES(CurveNetworkElement,
    {CurveNetworkElement::NODE, "Node"},
    {CurveNetworkElement::EDGE, "Edge"}
);

enum class VolumeMeshElement { VERTEX = 0, EDGE, FACE, CELL };
POLYSCOPE_DEFINE_ENUM_NAMES(VolumeMeshElement,
    {VolumeMeshElement::VERTEX, "Vertex"},
    {VolumeMeshElement::EDGE, "Edge"},
    {VolumeMeshElement::FACE, "Face"},
    {VolumeMeshElement::CELL, "Cell"}
);

enum class VolumeCellType { TET = 0, HEX };
POLYSCOPE_DEFINE_ENUM_NAMES(VolumeCellType,
    {VolumeCellType::TET, "Tet"},
    {VolumeCellType::HEX, "Hex"}
);

enum class VolumeGridElement { NODE = 0, CELL };
POLYSCOPE_DEFINE_ENUM_NAMES(VolumeGridElement,
    {VolumeGridElement::NODE, "Node"},
    {VolumeGridElement::CELL, "Cell"}
);

enum class SparseVolumeGridElement { CELL = 0, NODE };
POLYSCOPE_DEFINE_ENUM_NAMES(SparseVolumeGridElement,
    {SparseVolumeGridElement::CELL, "Cell"},
    {SparseVolumeGridElement::NODE, "Node"}
);

enum class IsolineStyle { Stripe = 0, Contour };
POLYSCOPE_DEFINE_ENUM_NAMES(IsolineStyle,
    {IsolineStyle::Stripe, "Stripe"},
    {IsolineStyle::Contour, "Contour"}
);

enum class ImplicitRenderMode { SphereMarch, FixedStep };
POLYSCOPE_DEFINE_ENUM_NAMES(ImplicitRenderMode,
    {ImplicitRenderMode::SphereMarch, "Sphere March"},
    {ImplicitRenderMode::FixedStep, "Fixed Step"}
);

enum class ImageOrigin { LowerLeft, UpperLeft };
POLYSCOPE_DEFINE_ENUM_NAMES(ImageOrigin,
    {ImageOrigin::LowerLeft, "Lower Left"},
    {ImageOrigin::UpperLeft, "Upper Left"}
);

enum class FilterMode { Nearest = 0, Linear };
POLYSCOPE_DEFINE_ENUM_NAMES(FilterMode,
    {FilterMode::Nearest, "Nearest"},
    {FilterMode::Linear, "Linear"}
);

enum class ParamCoordsType { UNIT = 0, WORLD }; // UNIT -> [0,1], WORLD -> length-valued
POLYSCOPE_DEFINE_ENUM_NAMES(ParamCoordsType,
    {ParamCoordsType::UNIT, "Unit"},
    {ParamCoordsType::WORLD, "World"}
);

enum class ParamVizStyle {
  CHECKER = 0,
  GRID,
  LOCAL_CHECK,
  LOCAL_RAD,
  CHECKER_ISLANDS
}; // TODO add "UV" with test UV map
POLYSCOPE_DEFINE_ENUM_NAMES(ParamVizStyle,
    {ParamVizStyle::CHECKER, "Checker"},
    {ParamVizStyle::GRID, "Grid"},
    {ParamVizStyle::LOCAL_CHECK, "Local Check"},
    {ParamVizStyle::LOCAL_RAD, "Local Rad"},
    {ParamVizStyle::CHECKER_ISLANDS, "Checker Islands"}
);

enum class ManagedBufferType {
  Float,
  Double,
  Vec2,
  Vec3,
  Vec4,
  Arr2Vec3,
  Arr3Vec3,
  Arr4Vec3,
  Int32,
  IVec2,
  IVec3,
  IVec4,
  UInt32,
  UVec2,
  UVec3,
  UVec4
};
POLYSCOPE_DEFINE_ENUM_NAMES(ManagedBufferType,
    {ManagedBufferType::Float, "Float"},
    {ManagedBufferType::Double, "Double"},
    {ManagedBufferType::Vec2, "Vec2"},
    {ManagedBufferType::Vec3, "Vec3"},
    {ManagedBufferType::Vec4, "Vec4"},
    {ManagedBufferType::Arr2Vec3, "Arr2Vec3"},
    {ManagedBufferType::Arr3Vec3, "Arr3Vec3"},
    {ManagedBufferType::Arr4Vec3, "Arr4Vec3"},
    {ManagedBufferType::Int32, "Int32"},
    {ManagedBufferType::IVec2, "IVec2"},
    {ManagedBufferType::IVec3, "IVec3"},
    {ManagedBufferType::IVec4, "IVec4"},
    {ManagedBufferType::UInt32, "UInt32"},
    {ManagedBufferType::UVec2, "UVec2"},
    {ManagedBufferType::UVec3, "UVec3"},
    {ManagedBufferType::UVec4, "UVec4"}
);


// What is the meaningful range of these values?
// Used to set meaningful colormaps
// STANDARD: [-inf, inf], zero does not mean anything special (ie, position)
// SYMMETRIC: [-inf, inf], zero is special (ie, net profit/loss)
// MAGNITUDE: [0, inf], zero is special (ie, length of a vector)
// CATEGORICAL: data is integers corresponding to labels, etc
enum class DataType { STANDARD = 0, SYMMETRIC, MAGNITUDE, CATEGORICAL };
POLYSCOPE_DEFINE_ENUM_NAMES(DataType,
    {DataType::STANDARD, "Standard"},
    {DataType::SYMMETRIC, "Symmetric"},
    {DataType::MAGNITUDE, "Magnitude"},
    {DataType::CATEGORICAL, "Categorical"}
);

// clang-format on

}; // namespace polyscope
