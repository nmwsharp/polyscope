// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/floating_quantity_structure.h"

#include "polyscope/pick.h"
#include "polyscope/polyscope.h"
#include "polyscope/render/engine.h"

#include "imgui.h"

#include <fstream>
#include <iostream>

namespace polyscope {

// Initialize statics
const std::string FloatingQuantityStructure::structureTypeName = "Floating Quantities";

// Constructor
FloatingQuantityStructure::FloatingQuantityStructure(std::string name)
    : QuantityStructure<FloatingQuantityStructure>(name, structureTypeName) {}


void FloatingQuantityStructure::buildCustomUI() { }

void FloatingQuantityStructure::buildCustomOptionsUI() {}

void FloatingQuantityStructure::draw() {
  for (auto& qp : quantities) {
    qp.second->draw();
  }
}

// override the structure UI, since this one is a bit different
void FloatingQuantityStructure::buildUI() {
  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels


  //if (ImGui::TreeNode(name.c_str())) {

    bool currEnabled = isEnabled();
    ImGui::Checkbox("Enable All", &currEnabled);
    setEnabled(currEnabled);
    //ImGui::SameLine();

    // Options popup
    /*
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      // Transform
      if (ImGui::BeginMenu("Transform")) {
        if (ImGui::MenuItem("Center")) centerBoundingBox();
        if (ImGui::MenuItem("Unit Scale")) rescaleToUnit();
        if (ImGui::MenuItem("Reset")) resetTransform();
        if (ImGui::MenuItem("Show Gizmo", NULL, &transformGizmo.enabled.get()))
          transformGizmo.enabled.manuallyChanged();
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Transparency")) {
        if (ImGui::SliderFloat("Alpha", &transparency.get(), 0., 1., "%.3f")) setTransparency(transparency.get());
        ImGui::TextUnformatted("Note: Change the transparency mode");
        ImGui::TextUnformatted("      in Appearance --> Transparency.");
        ImGui::TextUnformatted("Current mode: ");
        ImGui::SameLine();
        ImGui::TextUnformatted(modeName(render::engine->getTransparencyMode()).c_str());
        ImGui::EndMenu();
      }

      // Toggle whether slice planes apply
      if (ImGui::BeginMenu("Slice planes")) {
        if (state::slicePlanes.empty()) {
          // if there are none, show a helpful message
          if (ImGui::Button("Add slice plane")) {
            openSlicePlaneMenu = true;
            addSceneSlicePlane(true);
          }
        } else {
          // otherwise, show toggles for each
          ImGui::TextUnformatted("Applies to this structure:");
          ImGui::Indent(20);
          for (SlicePlane* s : state::slicePlanes) {
            bool ignorePlane = getIgnoreSlicePlane(s->name);
            if (ImGui::MenuItem(s->name.c_str(), NULL, !ignorePlane)) setIgnoreSlicePlane(s->name, !ignorePlane);
          }
          ImGui::Indent(-20);
        }
        ImGui::TextUnformatted("");
        ImGui::Separator();
        ImGui::TextUnformatted("Note: Manage slice planes in");
        ImGui::TextUnformatted("      View --> Slice Planes.");

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Slice plane options")) {
        if (ImGui::MenuItem("cull whole elements", NULL, getCullWholeElements()))
          setCullWholeElements(!getCullWholeElements());
        ImGui::EndMenu();
      }


      // Selection
      if (ImGui::BeginMenu("Structure Selection")) {
        if (ImGui::MenuItem("Enable all of type")) setEnabledAllOfType(true);
        if (ImGui::MenuItem("Disable all of type")) setEnabledAllOfType(false);
        if (ImGui::MenuItem("Isolate")) enableIsolate();
        ImGui::EndMenu();
      }

      buildStructureOptionsUI();

      // Do any structure-specific stuff here
      this->buildCustomOptionsUI();

      ImGui::EndPopup();
    }
  */

    // Do any structure-specific stuff here
    this->buildCustomUI();

    // Build quantities list, in the common case of a QuantityStructure
    this->buildQuantitiesUI();

    //ImGui::TreePop();
  //}
  ImGui::PopID();
}

// since hasExtents is false, the length scale and bbox value should never be used
bool FloatingQuantityStructure::hasExtents() { return false; }
double FloatingQuantityStructure::lengthScale() { return std::numeric_limits<double>::quiet_NaN(); }
std::tuple<glm::vec3, glm::vec3> FloatingQuantityStructure::boundingBox() {
  float nan = std::numeric_limits<float>::quiet_NaN();
  return std::make_tuple(glm::vec3{nan, nan, nan}, glm::vec3{nan, nan, nan});
}

std::string FloatingQuantityStructure::typeName() { return structureTypeName; }


FloatingScalarImageQuantity* FloatingQuantityStructure::addFloatingScalarImageImpl(std::string name, size_t dimX,
                                                                                   size_t dimY,
                                                                                   const std::vector<double>& values,
                                                                                   DataType type) {
  FloatingScalarImageQuantity* q = new FloatingScalarImageQuantity(*this, name, dimX, dimY, values, type);
  addQuantity(q);
  return q;
}
void removeFloatingScalarImage(std::string name) {
  if (!globalFloatingQuantityStructure) return;
  globalFloatingQuantityStructure->removeQuantity(name);
}


FloatingQuantityStructure* getGlobalFloatingQuantityStructure() {
  if (!globalFloatingQuantityStructure) {
    globalFloatingQuantityStructure = new FloatingQuantityStructure("global");
    bool success = registerStructure(globalFloatingQuantityStructure);
    if (!success) {
      safeDelete(globalFloatingQuantityStructure);
    }
  }
  return globalFloatingQuantityStructure;
}

void removeFloatingQuantityStructureIfEmpty() {
  if (globalFloatingQuantityStructure && globalFloatingQuantityStructure->quantities.empty()) {
    globalFloatingQuantityStructure->remove();
    globalFloatingQuantityStructure = nullptr;
  }
}


// Quantity default methods
FloatingQuantity::FloatingQuantity(std::string name_, FloatingQuantityStructure& parent_, bool dominates_)
    : Quantity<FloatingQuantityStructure>(name_, parent_, dominates_) {}

} // namespace polyscope
