// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/floating_quantity.h"
#include "polyscope/polyscope.h"
#include "polyscope/quantity.h"
#include "polyscope/structure.h"

namespace polyscope {

// === Derived structure can manage quantities

template <typename S>
QuantityStructure<S>::QuantityStructure(std::string name_, std::string subtypeName) : Structure(name_, subtypeName) {}

template <typename S>
QuantityStructure<S>::~QuantityStructure(){};

template <typename S>
bool QuantityStructure<S>::checkForQuantityWithNameAndDeleteOrError(std::string name, bool allowReplacement) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  // if it already exists and we cannot replace, throw an error
  if (!allowReplacement && (quantityExists || floatingQuantityExists)) {
    error("Tried to add quantity with name: [" + name +
          "], but a quantity with that name already exists on the structure [" + name +
          "]. Use the allowReplacement option like addQuantity(..., true) to replace.");
    return false;
  }

  // Track if the previous quantity was enabled
  // TODO why is isn't this handled by the persistence cache like everything else?
  bool existingQuantityWasEnabled = false;
  if (quantityExists) {
    existingQuantityWasEnabled = quantities.find(name)->second->isEnabled();
  }
  if (floatingQuantityExists) {
    existingQuantityWasEnabled = floatingQuantities.find(name)->second->isEnabled();
  }

  // Remove the old quantity
  if (quantityExists || floatingQuantityExists) {
    removeQuantity(name);
  }

  return existingQuantityWasEnabled;
}

template <typename S>
void QuantityStructure<S>::addQuantity(QuantityType* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  bool existingQuantityWasEnabled = checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity
  quantities[q->name] = std::unique_ptr<QuantityType>(q);

  // Re-enable the quantity if we're replacing an enabled quantity
  if (existingQuantityWasEnabled) {
    q->setEnabled(true);
  }
}

template <typename S>
void QuantityStructure<S>::addQuantity(FloatingQuantity* q, bool allowReplacement) {

  // Check if a quantity with this name exists, remove it or throw and error if so
  bool existingQuantityWasEnabled = checkForQuantityWithNameAndDeleteOrError(q->name, allowReplacement);

  // Add the new quantity
  floatingQuantities[q->name] = std::unique_ptr<FloatingQuantity>(q);

  // Re-enable the quantity if we're replacing an enabled quantity
  if (existingQuantityWasEnabled) {
    q->setEnabled(true);
  }
}

template <typename S>
typename QuantityStructure<S>::QuantityType* QuantityStructure<S>::getQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return nullptr;
  }
  return quantities[name].get();
}

template <typename S>
FloatingQuantity* QuantityStructure<S>::getFloatingQuantity(std::string name) {
  if (floatingQuantities.find(name) == floatingQuantities.end()) {
    return nullptr;
  }
  return floatingQuantities[name].get();
}

template <typename S>
void QuantityStructure<S>::refresh() {
  for (auto& qp : quantities) {
    qp.second->refresh();
  }
  for (auto& qp : floatingQuantities) {
    qp.second->refresh();
  }
  requestRedraw();
}

template <typename S>
void QuantityStructure<S>::removeQuantity(std::string name, bool errorIfAbsent) {

  // Look for an existing quantity with this name
  bool quantityExists = quantities.find(name) != quantities.end();
  bool floatingQuantityExists = floatingQuantities.find(name) != floatingQuantities.end();

  if (errorIfAbsent && !(quantityExists || floatingQuantityExists)) {
    error("No quantity named " + name + " added to structure " + name);
    return;
  }

  // delete standard quantities
  if (quantityExists) {
    // If this is the active quantity, clear it
    QuantityType& q = *quantities[name];
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

template <typename S>
void QuantityStructure<S>::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
  while (floatingQuantities.size() > 0) {
    removeQuantity(floatingQuantities.begin()->first);
  }
}

template <typename S>
void QuantityStructure<S>::setDominantQuantity(QuantityS<S>* q) {
  if (!q->dominates) {
    error("tried to set dominant quantity with quantity that has dominates=false");
    return;
  }

  // Dominant quantity must be enabled
  q->setEnabled(true);

  // All other dominating quantities will be disabled
  for (auto& qp : quantities) {
    QuantityType* qOther = qp.second.get();
    if (qOther->dominates && qOther->isEnabled() && qOther != q) {
      qOther->setEnabled(false);
    }
  }

  dominantQuantity = q;
}

template <typename S>
void QuantityStructure<S>::clearDominantQuantity() {
  dominantQuantity = nullptr;
}

template <typename S>
void QuantityStructure<S>::setAllQuantitiesEnabled(bool newEnabled) {
  for (auto& x : quantities) {
    x.second->setEnabled(newEnabled);
  }
  for (auto& x : floatingQuantities) {
    x.second->setEnabled(newEnabled);
  }
}


template <typename S>
void QuantityStructure<S>::buildQuantitiesUI() {
  // Build the quantities
  for (auto& x : quantities) {
    x.second->buildUI();
  }
  for (auto& x : floatingQuantities) {
    x.second->buildUI();
  }
}

template <typename S>
void QuantityStructure<S>::buildStructureOptionsUI() {
  if (ImGui::BeginMenu("Quantity Selection")) {
    if (ImGui::MenuItem("Enable all")) setAllQuantitiesEnabled(true);
    if (ImGui::MenuItem("Disable all")) setAllQuantitiesEnabled(false);
    ImGui::EndMenu();
  }
}

} // namespace polyscope
