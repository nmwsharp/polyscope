// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <cstddef>
#include <vector>

namespace polyscope {

class DisjointSets {
public:
  // Constructor
  DisjointSets(size_t n_);

  // Find parent of element x
  size_t find(size_t x);

  // Union by rank
  void merge(size_t x, size_t y);

private:
  // Member variables
  size_t n;
  std::vector<size_t> parent;
  std::vector<size_t> rank;
};

// Slight generalization of a disjoint set, which can track "marked" sets.
class MarkedDisjointSets {
public:
  // Constructor
  MarkedDisjointSets(size_t n_);

  // Find parent of element x
  size_t find(size_t x);

  // Union by rank
  // If either set in the union is marked, the result is marked
  void merge(size_t x, size_t y);

  // Mark/unmark a set
  void mark(size_t x);
  void unmark(size_t x);

  // Check if a set is marked
  bool isMarked(size_t x);

private:
  // Member variables
  size_t n;
  std::vector<size_t> parent;
  std::vector<size_t> rank;
  std::vector<bool> marked;
};

} // namespace polyscope
