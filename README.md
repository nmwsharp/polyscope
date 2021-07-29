# Polyscope's documentation is hosted at [polyscope.run](http://polyscope.run)

![Polyscope](http://polyscope.run/media/teaser.svg)

[![actions status linux](https://github.com/nmwsharp/polyscope/workflows/linux/badge.svg)](https://github.com/nmwsharp/polyscope/actions)
[![actions status macOS](https://github.com/nmwsharp/polyscope/workflows/macOS/badge.svg)](https://github.com/nmwsharp/polyscope/actions)
[![actions status windows](https://github.com/nmwsharp/polyscope/workflows/windows/badge.svg)](https://github.com/nmwsharp/polyscope/actions)
![PyPI](https://img.shields.io/pypi/v/polyscope?style=flat-square)

Polyscope is a C++/Python viewer and user interface for 3D data such as meshes and point clouds. It allows you to register your data and quickly generate informative and beautiful visualizations, either programmatically or via a dynamic GUI. Polyscope is designed to be lightweight---it does not "take ownership" over your entire program, and it is easy to integrate with existing codebases and popular libraries. The lofty objective of Polyscope is to offer a useful visual interface to your data via a single line of code.

Polyscope uses a paradigm of *structures* and *quantities*. A **structure** is a geometric object in the scene, such as a surface mesh or point cloud. A **quantity** is data associated with a structure, such as a scalar function or a vector field.

When any of these structures and quantities are registered, Polyscope displays them in an interactive 3D scene, handling boilerplate concerns such as toggling visibility, color-mapping data and adjusting maps, "picking" to click in the scene and query numerical quantities, etc.

C++:

``` C++
#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

// Initialize polyscope
polyscope::init();

// Register a point cloud
// `points` is a Nx3 array-like container of points
polyscope::registerPointCloud("my points", points)

// Register a surface mesh structure
// `meshVerts` is a Vx3 array-like container of vertex positions
// `meshFaces` is a Fx3 array-like container of face indices  
polyscope::registerSurfaceMesh("my mesh", meshVerts, meshFaces);

// Add a scalar and a vector function defined on the mesh
// `scalarQuantity` is a length V array-like container of values
// `vectorQuantity` is an Fx3 array-like container of vectors per face
polyscope::getSurfaceMesh("my mesh")->addVertexScalarQuantity("my_scalar", scalarQuantity);
polyscope::getSurfaceMesh("my mesh")->addFaceVectorQuantity("my_vector", vectorQuantity);

// View the point cloud and mesh we just registered in the 3D UI
polyscope::show();
```

Python:
``` python
import polyscope as ps

# Initialize polyscope
ps.init()

### Register a point cloud
# `my_points` is a Nx3 numpy array
ps.register_point_cloud("my points", my_points)

### Register a mesh
# `verts` is a Nx3 numpy array of vertex positions
# `faces` is a Fx3 array of indices, or a nested list
ps.register_surface_mesh("my mesh", verts, faces, smooth_shade=True)

# Add a scalar function and a vector function defined on the mesh
# vertex_scalar is a length V numpy array of values
# face_vectors is an Fx3 array of vectors per face
ps.get_surface_mesh("my mesh").add_scalar_quantity("my_scalar", 
        vertex_scalar, defined_on='vertices', cmap='blues')
ps.get_surface_mesh("my mesh").add_vector_quantity("my_vector", 
        face_vectors, defined_on='faces', color=(0.2, 0.5, 0.5))

# View the point cloud and mesh we just registered in the 3D UI
ps.show()
```

Polyscope is designed to make your life easier. It is simple to build, and fewer than 10 lines of code should be sufficient to start visualizing. In C++, some [template magic](https://polyscope.run/data_adaptors/) means Polyscope can probably accept the data types you're already using!

---
Author: [Nicholas Sharp](http://www.nmwsharp.com)

If Polyscope contributes to an academic publication, cite it as:
```bib
@misc{polyscope,
  title = {Polyscope},
  author = {Nicholas Sharp and others},
  note = {www.polyscope.run},
  year = {2019}
}
```

Development of this software was funded in part by NSF Award 1717320, an NSF graduate research fellowship, and gifts from Adobe Research and Autodesk, Inc.
