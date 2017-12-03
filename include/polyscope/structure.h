#pragma once

#include <map>
#include <string>
#include <iostream>

#include "geometrycentral/vector3.h"

namespace polyscope {

// Enum of structure types
enum class StructureType {SurfaceMesh=0, PointCloud, CameraView};

class Structure {
 public:
  // === Member functions ===

  // Base constructor which sets the name
  Structure(std::string name, StructureType type);

  virtual ~Structure() = 0;

  // Render the the structure on screen
  virtual void draw() = 0;

  // Do setup work related to drawing, including allocating openGL data
  virtual void prepare() = 0;

  // Build the imgui display
  virtual void drawUI() = 0;
  virtual void drawSharedStructureUI() = 0;

  // Render for picking
  virtual void drawPick() = 0;

  // A characteristic length for the structure
  virtual double lengthScale() = 0;

  // Axis-aligned bounding box for the structure
  virtual std::tuple<geometrycentral::Vector3, geometrycentral::Vector3>
  boundingBox() = 0;

  // === Member variables ===
  const std::string name;
  const StructureType type; // useful for printing to GUI
};

// Make structure type printable
inline std::string getStructureTypeName(StructureType type){
  switch(type) {
    case StructureType::SurfaceMesh:
       return "Surface Mesh";
    case StructureType::PointCloud:
       return "Point Cloud";
    case StructureType::CameraView:
       return "Camera View";
    }
}
inline std::ostream& operator<<(std::ostream& out, const StructureType value){
  return out << getStructureTypeName(value);
}



}  // namespace polyscope