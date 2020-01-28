// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#include "polyscope/polyscope.h"
#include "polyscope/structure.h"

namespace polyscope {

// === Derived structure can manage quantities

template <typename S>
QuantityStructure<S>::QuantityStructure(std::string name_, std::string subtypeName) : Structure(name_, subtypeName) {}

template <typename S>
QuantityStructure<S>::~QuantityStructure(){};

template <typename S>
void QuantityStructure<S>::addQuantity(QuantityType* q, bool allowReplacement) {

  // Look for an existing quantity with this name
  bool existingQuantityWasEnabled = false;
  if (quantities.find(q->name) != quantities.end()) {

    if (allowReplacement) {
      // delete it
      existingQuantityWasEnabled = quantities.find(q->name)->second->isEnabled();
      removeQuantity(q->name);
    } else {
      // throw an error
      error("Tried to add quantity with name: [" + q->name +
            "], but a quantity with that name already exists on the structure [" + name +
            "]. Use the allowReplacement option like addQuantity(..., true) to replace.");
      return;
    }
  }

  // Add the new quantity
  quantities[q->name] = std::unique_ptr<QuantityType>(q);

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
void QuantityStructure<S>::removeQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return;
  }

  // If this is the active quantity, clear it
  QuantityType& q = *quantities[name];
  if (dominantQuantity == &q) {
    clearDominantQuantity();
  }

  // Delete the quantity
  quantities.erase(name);
}

template <typename S>
void QuantityStructure<S>::removeAllQuantities() {
  while (quantities.size() > 0) {
    removeQuantity(quantities.begin()->first);
  }
}

template <typename S>
void QuantityStructure<S>::setDominantQuantity(Quantity<S>* q) {
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
void QuantityStructure<S>::buildQuantitiesUI() {
  // Build the quantities
  for (auto& x : quantities) {
    x.second->buildUI();
  }
}


} // namespace polyscope
