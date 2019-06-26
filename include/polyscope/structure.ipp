#include "polyscope/polyscope.h"
#include "polyscope/structure.h"

namespace polyscope {

// === Derived structure can manage quantities

template <typename S>
QuantityStructure<S>::QuantityStructure(std::string name_) : Structure(name_) {}

template <typename S>
QuantityStructure<S>::~QuantityStructure(){};

template <typename S>
void QuantityStructure<S>::addQuantity(Quantity<S>* q, bool allowReplacement) {

  // Look for an existing quantity with this name
  bool existingQuantityWasEnabled = false;
  if (quantities.find(q->name) != quantities.end()) {

    if (allowReplacement) {
      // delete it
      existingQuantityWasEnabled = quantities.find(q->name)->second->isEnabled();
      removeQuantity(q->name);
    } else {
      // throw an error
      error("existing quantity with name: " + q->name + ". true addQuantity(.., allowReplacement=true) to replace");
      return;
    }
  }

  // Add the new quantity
  quantities[q->name] = std::unique_ptr<Quantity<S>>(q);

  // Re-enable the quantity if we're replacing an enabled quantity
  if (existingQuantityWasEnabled) {
    q->setEnabled(true);
  }
}


template <typename S>
Quantity<S>* QuantityStructure<S>::getQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return nullptr;
  }
}

template <typename S>
void QuantityStructure<S>::removeQuantity(std::string name) {
  if (quantities.find(name) == quantities.end()) {
    return;
  }

  // If this is the active quantity, clear it
  Quantity<S>& q = *quantities[name];
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
  // Dominant quantity must be enabled
  q->setEnabled(true);
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
