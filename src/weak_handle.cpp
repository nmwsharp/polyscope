// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/weak_handle.h"
#include "polyscope/internal.h"

namespace polyscope {

WeakReferrable::WeakReferrable()
    : weakReferrableDummyRef(new WeakHandleDummyType()), weakReferableUniqueID(internal::getNextUniqueID()) {}

bool GenericWeakHandle::isValid() const { return !sentinel.expired(); };

void GenericWeakHandle::reset() { sentinel.reset(); };

uint64_t GenericWeakHandle::getUniqueID() const { return targetUniqueID; };


} // namespace polyscope
