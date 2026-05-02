// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/structure.h"

#include "polyscope/floating_quantity.h"
#include "polyscope/polyscope.h"
#include "polyscope/quantity.h"

#include "polyscope/floating_quantities.h"

#include "imgui.h"

namespace polyscope {

Structure::Structure(std::string name_, std::string subtypeName_)
    : name(name_), subtypeName(subtypeName_), enabled(subtypeName + "#" + name + "#enabled", true),
      objectTransform(subtypeName + "#" + name + "#object_transform", glm::mat4(1.0)),
      transparency(subtypeName + "#" + name + "#transparency", 1.0),
      transformGizmo(subtypeName + "#" + name + "#transform_gizmo", &objectTransform.get(), &objectTransform),
      cullWholeElements(subtypeName + "#" + name + "#cullWholeElements", false),
      ignoredSlicePlaneNames(subtypeName + "#" + name + "#ignored_slice_planes", {}),
      objectSpaceBoundingBox(
          std::tuple<glm::vec3, glm::vec3>{glm::vec3{-777, -777, -777}, glm::vec3{-777, -777, -777}}),
      objectSpaceLengthScale(-777) {
  validateName(name);
}

Structure::~Structure() {};

Structure* Structure::setEnabled(bool newEnabled) {
  if (newEnabled == isEnabled()) return this;
  enabled = newEnabled;
  requestRedraw();
  return this;
};

bool Structure::isEnabled() { return enabled.get(); };

void Structure::enableIsolate() {
  for (auto& structure : polyscope::state::structures[this->typeName()]) {
    structure.second->setEnabled(false);
  }
  this->setEnabled(true);
}

void Structure::setEnabledAllOfType(bool newEnabled) {
  for (auto& structure : polyscope::state::structures[this->typeName()]) {
    structure.second->setEnabled(newEnabled);
  }
}

void Structure::addToGroup(std::string groupName) { addToGroup(*getGroup(groupName)); }

void Structure::addToGroup(Group& group) { group.addChildStructure(*this); }

void Structure::buildUI() {
  ImGui::PushID(name.c_str()); // ensure there are no conflicts with
                               // identically-named labels


  if (ImGui::TreeNode(name.c_str())) {

    bool currEnabled = isEnabled();
    ImGui::Checkbox("Enabled", &currEnabled);
    setEnabled(currEnabled);
    ImGui::SameLine();

    // Options popup
    if (ImGui::Button("Options")) {
      ImGui::OpenPopup("OptionsPopup");
    }
    if (ImGui::BeginPopup("OptionsPopup")) {

      // Transform
      if (ImGui::BeginMenu("Transform")) {
        if (ImGui::MenuItem("Center")) centerBoundingBox();
        if (ImGui::MenuItem("Unit Scale")) rescaleToUnit();
        if (ImGui::MenuItem("Reset")) resetTransform();
        transformGizmo.buildMenuItems();
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
            addSlicePlane();
          }
        } else {
          // otherwise, show toggles for each
          ImGui::TextUnformatted("Applies to this structure:");
          ImGui::Indent(20);
          for (std::unique_ptr<SlicePlane>& s : state::slicePlanes) {
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

    // Do any structure-specific stuff here
    this->buildCustomUI();

    // Build quantities list, in the common case of a Structure
    this->buildQuantitiesUI();

    ImGui::TreePop();
  }
  ImGui::PopID();
}


void Structure::buildQuantitiesUI() {
  // Build the quantities
  for (auto& x : quantities) {
    x.second->buildUI();
  }
  for (auto& x : floatingQuantities) {
    x.second->buildUI();
  }
}


void Structure::buildStructureOptionsUI() {
  if (ImGui::BeginMenu("Quantity Selection")) {
    if (ImGui::MenuItem("Enable all")) setAllQuantitiesEnabled(true);
    if (ImGui::MenuItem("Disable all")) setAllQuantitiesEnabled(false);
    ImGui::EndMenu();
  }
}

void Structure::buildSharedStructureUI() {}

void Structure::buildCustomOptionsUI() {}

void Structure::refresh() {

  for (auto& qp : quantities) {
    qp.second->refresh();
  }
  for (auto& qp : floatingQuantities) {
    qp.second->refresh();
  }

  updateObjectSpaceBounds();
  requestRedraw();
}

std::tuple<glm::vec3, glm::vec3> Structure::boundingBox() {
  const glm::mat4x4& T = objectTransform.get();
  glm::vec4 lh = T * glm::vec4(std::get<0>(objectSpaceBoundingBox), 1.);
  glm::vec3 l = glm::vec3(lh) / lh.w;
  glm::vec4 uh = T * glm::vec4(std::get<1>(objectSpaceBoundingBox), 1.);
  glm::vec3 u = glm::vec3(uh) / uh.w;
  return std::tuple<glm::vec3, glm::vec3>{l, u};
}

float Structure::lengthScale() {
  // compute the scaling caused by the object transform
  const glm::mat4x4& T = objectTransform.get();
  float transScale = std::cbrt(std::abs(glm::determinant(glm::mat3x3(T)))) / T[3][3];
  return transScale * objectSpaceLengthScale;
}

void Structure::setTransform(glm::mat4x4 transform) {
  objectTransform = transform;
  updateStructureExtents();
}

void Structure::setPosition(glm::vec3 vec) {
  objectTransform.get()[3][0] = vec.x;
  objectTransform.get()[3][1] = vec.y;
  objectTransform.get()[3][2] = vec.z;
  updateStructureExtents();
}

void Structure::translate(glm::vec3 vec) {
  objectTransform = glm::translate(objectTransform.get(), vec);
  updateStructureExtents();
}

glm::mat4x4 Structure::getTransform() { return objectTransform.get(); }

glm::vec3 Structure::getPosition() {
  return glm::vec3{objectTransform.get()[3][0], objectTransform.get()[3][1], objectTransform.get()[3][2]};
}

TransformationGizmo& Structure::getTransformGizmo() { return transformGizmo; }

void Structure::resetTransform() {
  objectTransform = glm::mat4(1.0);
  updateStructureExtents();
}

void Structure::centerBoundingBox() {
  std::tuple<glm::vec3, glm::vec3> bbox = boundingBox();
  glm::vec3 center = (std::get<1>(bbox) + std::get<0>(bbox)) / 2.0f;
  glm::mat4x4 newTrans = glm::translate(glm::mat4x4(1.0), -glm::vec3(center.x, center.y, center.z));
  objectTransform = newTrans * objectTransform.get();
  updateStructureExtents();
}

void Structure::rescaleToUnit() {
  double currScale = lengthScale();
  float s = static_cast<float>(1.0 / currScale);
  glm::mat4x4 newTrans = glm::scale(glm::mat4x4(1.0), glm::vec3{s, s, s});
  objectTransform = newTrans * objectTransform.get();
  updateStructureExtents();
}

bool Structure::hasExtents() { return true; }

glm::mat4 Structure::getModelView() { return view::getCameraViewMatrix() * objectTransform.get(); }

std::vector<std::string> Structure::addStructureRules(std::vector<std::string> initRules) {
  if (render::engine->slicePlanesEnabled()) {
    if (getCullWholeElements()) {
    } else {
      initRules.push_back("GENERATE_VIEW_POS");
      initRules.push_back("CULL_POS_FROM_VIEW");
    }
  }
  return initRules;
}

void Structure::setStructureUniforms(render::ShaderProgram& p) {
  if (p.hasUniform("u_modelView")) {
    glm::mat4 viewMat = getModelView();
    p.setUniform("u_modelView", glm::value_ptr(viewMat));
  }

  if (p.hasUniform("u_projMatrix")) {
    glm::mat4 projMat = view::getCameraPerspectiveMatrix();
    p.setUniform("u_projMatrix", glm::value_ptr(projMat));
  }

  if (render::engine->transparencyEnabled()) {
    if (p.hasUniform("u_transparency")) {
      p.setUniform("u_transparency", transparency.get());
    }

    if (p.hasUniform("u_viewportDim")) {
      glm::vec4 viewport = render::engine->getCurrentViewport();
      glm::vec2 viewportDim{viewport[2], viewport[3]};
      p.setUniform("u_viewportDim", viewportDim);
    }

    // Attach the min depth texture, if needed
    // (note that this design is somewhat lazy wrt to the name of the function: it sets a texture, not a uniform, and
    // only actually does anything once on initialization)
    if (render::engine->transparencyEnabled() && p.hasTexture("t_minDepth") && !p.textureIsSet("t_minDepth")) {
      p.setTextureFromBuffer("t_minDepth", render::engine->sceneDepthMin.get());
    }
  }

  // Respect any slice planes
  for (std::unique_ptr<SlicePlane>& s : state::slicePlanes) {
    bool ignoreThisPlane = getIgnoreSlicePlane(s->name);
    s->setSceneObjectUniforms(p, ignoreThisPlane);
  }

  // TODO this chain if "if"s is not great. Set up some system in the render engine to conditionally set these? Maybe
  // a list of lambdas? Ugh.
  if (p.hasUniform("u_viewport_viewPos")) {
    glm::vec4 viewport = render::engine->getCurrentViewport();
    p.setUniform("u_viewport_viewPos", viewport);
  }
  if (p.hasUniform("u_invProjMatrix_viewPos")) {
    glm::mat4 P = view::getCameraPerspectiveMatrix();
    glm::mat4 Pinv = glm::inverse(P);
    p.setUniform("u_invProjMatrix_viewPos", glm::value_ptr(Pinv));
  }
}

bool Structure::wantsCullPosition() { return render::engine->slicePlanesEnabled() && getCullWholeElements(); }

std::string Structure::uniquePrefix() { return typeName() + "#" + name + "#"; }

void Structure::remove() { removeStructure(typeName(), name); }


Structure* Structure::setTransparency(float newVal) {
  transparency = newVal;

  if (newVal < 1. && options::transparencyMode == TransparencyMode::None) {
    options::transparencyMode = TransparencyMode::Pretty;
  }
  requestRedraw();

  return this;
}
float Structure::getTransparency() { return transparency.get(); }

Structure* Structure::setCullWholeElements(bool newVal) {
  cullWholeElements = newVal;
  refresh();
  requestRedraw();
  return this;
}
bool Structure::getCullWholeElements() { return cullWholeElements.get(); }


Structure* Structure::setTransformGizmoEnabled(bool newVal) {
  transformGizmo.setEnabled(newVal);
  return this;
}
bool Structure::getTransformGizmoEnabled() { return transformGizmo.getEnabled(); }

Structure* Structure::setIgnoreSlicePlane(std::string name, bool newValue) {

  if (getIgnoreSlicePlane(name) == newValue) {
    // no change
    ignoredSlicePlaneNames.manuallyChanged();
    refresh();
    requestRedraw();
    return this;
  }

  std::vector<std::string>& names = ignoredSlicePlaneNames.get();
  if (newValue) {
    // new value is true; add it to the list
    names.push_back(name);
  } else {
    // new value is false; remove it from the list
    names.erase(std::remove(names.begin(), names.end(), name), names.end());
  }
  ignoredSlicePlaneNames.manuallyChanged();
  refresh();
  requestRedraw();
  return this;
}

bool Structure::getIgnoreSlicePlane(std::string name) {
  std::vector<std::string>& names = ignoredSlicePlaneNames.get();
  bool ignoreThisPlane = (std::find(names.begin(), names.end(), name) != names.end());
  return ignoreThisPlane;
}

void Structure::addQuantity(Quantity* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity and take ownership
  quantities[q->name] = std::unique_ptr<Quantity>(q);
}


void Structure::addQuantity(FloatingQuantity* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity
  floatingQuantities[q->name] = std::unique_ptr<FloatingQuantity>(q);
}


Quantity* Structure::getQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return nullptr;
  }
  return quantities[name].get();
}


FloatingQuantity* Structure::getFloatingQuantity(std::string name) {
  if (floatingQuantities.find(name) == floatingQuantities.end()) {
    return nullptr;
  }
  return floatingQuantities[name].get();
}


void Structure::removeQuantity(std::string name, bool errorIfAbsent) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  if (errorIfAbsent && !(quantityExists || floatingQuantityExists)) {
    exception("No quantity named " + name + " added to structure " + name);
    return;
  }

  // delete standard quantities
  if (quantityExists) {
    // If this is the active quantity, clear it
    Quantity& q = *quantities[name];
    if (dominantQuantity == &q) {
      clearDominantQuantity();
    }

    // Delete the quantity
    quantities.erase(name);
  }

  // delete floating quantities
  if (floatingQuantityExists) {
    floatingQuantities.erase(name);
  }
}


void Structure::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
  while (floatingQuantities.size() > 0) {
    removeQuantity(floatingQuantities.begin()->first);
  }
}


void Structure::setDominantQuantity(Quantity* q) {
  if (!q->dominates) {
    exception("tried to set dominant quantity with quantity that has dominates=false");
    return;
  }

  // Dominant quantity must be enabled
  q->setEnabled(true);

  // All other dominating quantities will be disabled
  for (auto& qp : quantities) {
    Quantity* qOther = qp.second.get();
    if (qOther->dominates && qOther->isEnabled() && qOther != q) {
      qOther->setEnabled(false);
    }
  }

  dominantQuantity = q;
}


void Structure::clearDominantQuantity() { dominantQuantity = nullptr; }


void Structure::setAllQuantitiesEnabled(bool newEnabled) {
  for (auto& x : quantities) {
    x.second->setEnabled(newEnabled);
  }
  for (auto& x : floatingQuantities) {
    x.second->setEnabled(newEnabled);
  }
}

void Structure::checkForQuantityWithNameAndDeleteOrError(std::string name, bool allowReplacement) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  // if it already exists and we cannot replace, throw an error
  if (!allowReplacement && (quantityExists || floatingQuantityExists)) {
    exception("Tried to add quantity with name: [" + name +
              "], but a quantity with that name already exists on the structure [" + name +
              "]. Use the allowReplacement option like addQuantity(..., true) to replace.");
  }

  // Remove the old quantity
  if (quantityExists || floatingQuantityExists) {
    removeQuantity(name);
  }
}

// === Floating Quantity Impls ===

// Forward declare helper functions, which wrap the constructors for the floating quantities below.
// Otherwise, we would have to include their respective headers here, and create some really gnarly header dependency
// chains.
ScalarImageQuantity* createScalarImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                               const std::vector<float>& data, ImageOrigin imageOrigin,
                                               DataType dataType);
ColorImageQuantity* createColorImageQuantity(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                             const std::vector<glm::vec4>& data, ImageOrigin imageOrigin);
DepthRenderImageQuantity* createDepthRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData, ImageOrigin imageOrigin);

ColorRenderImageQuantity* createColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                 const std::vector<float>& depthData,
                                                 const std::vector<glm::vec3>& normalData,
                                                 const std::vector<glm::vec3>& colorData, ImageOrigin imageOrigin);


RawColorRenderImageQuantity* createRawColorRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                       const std::vector<float>& depthData,
                                                       const std::vector<glm::vec3>& colorData,
                                                       ImageOrigin imageOrigin);

RawColorAlphaRenderImageQuantity* createRawColorAlphaRenderImage(Structure& parent, std::string name, size_t dimX,
                                                                 size_t dimY, const std::vector<float>& depthData,
                                                                 const std::vector<glm::vec4>& colorData,
                                                                 ImageOrigin imageOrigin);


ScalarRenderImageQuantity* createScalarRenderImage(Structure& parent, std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec3>& normalData,
                                                   const std::vector<float>& scalarData, ImageOrigin imageOrigin,
                                                   DataType type);


ScalarImageQuantity* Structure::addScalarImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                           const std::vector<float>& values, ImageOrigin imageOrigin,
                                                           DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ScalarImageQuantity* q = createScalarImageQuantity(*this, name, dimX, dimY, values, imageOrigin, type);
  addQuantity(q);
  return q;
}


ColorImageQuantity* Structure::addColorImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                         const std::vector<glm::vec4>& values,
                                                         ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ColorImageQuantity* q = createColorImageQuantity(*this, name, dimX, dimY, values, imageOrigin);
  addQuantity(q);
  return q;
}


DepthRenderImageQuantity* Structure::addDepthRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                     const std::vector<float>& depthData,
                                                                     const std::vector<glm::vec3>& normalData,
                                                                     ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  DepthRenderImageQuantity* q = createDepthRenderImage(*this, name, dimX, dimY, depthData, normalData, imageOrigin);
  addQuantity(q);
  return q;
}


ColorRenderImageQuantity* Structure::addColorRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                     const std::vector<float>& depthData,
                                                                     const std::vector<glm::vec3>& normalData,
                                                                     const std::vector<glm::vec3>& colorData,
                                                                     ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ColorRenderImageQuantity* q =
      createColorRenderImage(*this, name, dimX, dimY, depthData, normalData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}


ScalarRenderImageQuantity* Structure::addScalarRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                       const std::vector<float>& depthData,
                                                                       const std::vector<glm::vec3>& normalData,
                                                                       const std::vector<float>& scalarData,
                                                                       ImageOrigin imageOrigin, DataType type) {
  checkForQuantityWithNameAndDeleteOrError(name);
  ScalarRenderImageQuantity* q =
      createScalarRenderImage(*this, name, dimX, dimY, depthData, normalData, scalarData, imageOrigin, type);
  addQuantity(q);
  return q;
}


RawColorRenderImageQuantity* Structure::addRawColorRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                                           const std::vector<float>& depthData,
                                                                           const std::vector<glm::vec3>& colorData,
                                                                           ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  RawColorRenderImageQuantity* q =
      createRawColorRenderImage(*this, name, dimX, dimY, depthData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}


RawColorAlphaRenderImageQuantity*
Structure::addRawColorAlphaRenderImageQuantityImpl(std::string name, size_t dimX, size_t dimY,
                                                   const std::vector<float>& depthData,
                                                   const std::vector<glm::vec4>& colorData, ImageOrigin imageOrigin) {
  checkForQuantityWithNameAndDeleteOrError(name);
  RawColorAlphaRenderImageQuantity* q =
      createRawColorAlphaRenderImage(*this, name, dimX, dimY, depthData, colorData, imageOrigin);
  addQuantity(q);
  return q;
}

} // namespace polyscope
