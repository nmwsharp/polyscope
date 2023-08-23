// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/weak_handle.h"


namespace polyscope {

WeakReferrable::WeakReferrable() : weakReferrableDummyRef(new WeakHandleDummyType()) {}


} // namespace polyscope
