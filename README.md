![Polyscope](https://github.com/nmwsharp/polyscope/blob/master/misc/teaser_image/teaser.jpg)
---
Polyscope is a C++ viewer and user interface for the rapid prototyping and debugging of geometric algorithms in 3D geometry processing, scientific computing, and computer graphics/vision. The lofty objective of Polyscope is to offer a useful visual interface to your data via a single line of code.

Polyscope uses a paradigm of *structures* and *quantities*. A **structure** is a geometric object in the scene, such as a surface mesh or point cloud. A **quantity** is data associated with a structure, such as a scalar function or a vector field.

The following structures and quantities are supported (or *planned*):

- Surface meshes
  - Scalar functions (on vertices, faces, edges, halfedges, or corners)
  - Vector fields (extrinsic or embedded in the surface, symmetric, etc)
  - Distance functions
  - Subsets of mesh elements
  - *Parameterizations*
- Point clouds
  - *Scalar functions*
  - *Vector functions*
- Camera views
  - RGB images
  - Scalar images
- Ray sets
- *Volume meshes*
- *Octrees*
 
When any of these structures and quantities are registered, Polyscope displays them in an interactive 3D scene, handling boilerplate concerns such as toggling the display of various data, colormapping data and editing maps, providing "picking" support to click in the scene and display numerical quantities, and generating histograms of scalar values.

A simple workflow for visualizing data in Polyscope looks like:
```C++
#include "polyscope.h"

// Initialize polyscope
polyscope::init();

// Register a surface mesh structure
polyscope::registerSurfaceMesh("my mesh", meshGeometry);

// Add a scalar and a vector function to the mesh
polyscope::getSurfaceMesh("my mesh")->addQuantity("my_scalar", scalarQuantity);
polyscope::getSurfaceMesh("my mesh")->addQuantity("my_vector", vectorQuantity);

// Show the gui
polyscope::show();

```

The last line creates a UI window allowing you to inspect and visualize the data! For details on the API, see the documentation.

---

Development of this software was funded in part by NSF Award 1717320, an NSF graduate research fellowship, and gifts from Adobe Research and Autodesk, Inc.
