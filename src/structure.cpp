#include "polyscope/structure.h"

namespace polyscope {

Structure::Structure(std::string name_, StructureType type_) : name(name_), type(type_) {}

Structure::~Structure(){};

} // namespace polyscope