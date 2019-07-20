# Polyscope's documentation is hosted at [polyscope.run](http://polyscope.run)

![Polyscope](http://polyscope.run/media/teaser.svg)

Polyscope is a C++ viewer and user interface for the rapid prototyping and debugging of geometric algorithms in 3D geometry processing, scientific computing, and computer graphics/vision. The lofty objective of Polyscope is to offer a useful visual interface to your data via a single line of code.

Polyscope uses a paradigm of *structures* and *quantities*. A **structure** is a geometric object in the scene, such as a surface mesh or point cloud. A **quantity** is data associated with a structure, such as a scalar function or a vector field.

When any of these structures and quantities are registered, Polyscope displays them in an interactive 3D scene, handling boilerplate concerns such as toggling the display of various data, colormapping data and editing maps, providing "picking" support to click in the scene and display numerical quantities, and generating histograms of scalar values.

A simple workflow for visualizing data in Polyscope looks like:
``` C++
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

// Initialize polyscope
polyscope::init();

// Register a surface mesh structure
polyscope::registerSurfaceMesh("my mesh", mesh.vertices, mesh.faces);

// Add a scalar and a vector function to the mesh
polyscope::getSurfaceMesh("my mesh")->addVertexScalarQuantity("my_scalar", scalarQuantity);
polyscope::getSurfaceMesh("my mesh")->addFaceVectorQuantity("my_vector", vectorQuantity);

// Show the gui
polyscope::show();
```

Polyscope is designed to be easy to integrate with existing programs. It is simple to build, and fewer than 10 lines of code should be sufficient to start visualizing. Thanks to some [template magic](../data_adaptors), Polyscope can probably directly read from the data types you're already using!

---
Author: [Nicholas Sharp](http://www.nmwsharp.com)

If Polyscope contributes to an academic publication, cite it as:
```bib
@misc{polyscope,
  title = {Polyscope},
  author = {Nicholas Sharp and the Polyscope contributors},
  note = {www.polyscope.run},
  year = {2019}
}
```

Development of this software was funded in part by NSF Award 1717320, an NSF graduate research fellowship, and gifts from Adobe Research and Autodesk, Inc.
