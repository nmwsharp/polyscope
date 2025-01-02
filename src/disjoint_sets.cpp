// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#include "polyscope/disjoint_sets.h"

using std::vector;

namespace polyscope {

// Constructor
DisjointSets::DisjointSets(size_t n_) : n(n_), parent(n + 1), rank(n + 1) {
  // Initialize all elements to be in different sets and to have rank 0
  for (size_t i = 0; i <= n; i++) {
    rank[i] = 0;
    parent[i] = i;
  }
}

// Find parent of element x
size_t DisjointSets::find(size_t x) {
  if (x != parent[x]) parent[x] = find(parent[x]);
  return parent[x];
}

// Union by rank
void DisjointSets::merge(size_t x, size_t y) {
  x = find(x);
  y = find(y);

  // Smaller tree becomes a subtree of the larger tree
  if (rank[x] > rank[y])
    parent[y] = x;
  else
    parent[x] = y;

  if (rank[x] == rank[y]) rank[y]++;
}

// Constructor
MarkedDisjointSets::MarkedDisjointSets(size_t n_) : n(n_), parent(n + 1), rank(n + 1), marked(n + 1) {
  // Initialize all elements to be in different sets and to have rank 0
  for (size_t i = 0; i <= n; i++) {
    rank[i] = 0;
    parent[i] = i;
    marked[i] = false;
  }
}

void MarkedDisjointSets::mark(size_t x) {
  size_t p = find(x);
  marked[p] = true;
}

void MarkedDisjointSets::unmark(size_t x) {
  size_t p = find(x);
  marked[p] = false;
}

bool MarkedDisjointSets::isMarked(size_t x) {
  size_t p = find(x);
  return marked[p];
}

// Find parent of element x
size_t MarkedDisjointSets::find(size_t x) {
  if (x != parent[x]) parent[x] = find(parent[x]);
  return parent[x];
}

// Union by rank
void MarkedDisjointSets::merge(size_t x, size_t y) {
  x = find(x);
  y = find(y);

  // Smaller tree becomes a subtree of the larger tree
  if (rank[x] > rank[y])
    parent[y] = x;
  else
    parent[x] = y;

  if (rank[x] == rank[y]) rank[y]++;

  // If either was marked, both are marked
  if (marked[x] || marked[y]) {
    marked[x] = true;
    marked[y] = true;
  }
}

} // namespace polyscope
