// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include <array>
#include <tuple>

// Some useful hash functions that should really be in the standard library

// See https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
// and elsewhere

// Use with something like:
// std::unordered_set<std::pair<size_t, size_t>, polyscope::hash_combine::hash<std::pair<size_t, size_t>>> seenEdges;

namespace polyscope {
namespace hash_combine {

// Copy of the usual hash capabilities from std, but in this namespace so it gets picked up for non-combining types
template <typename TT>
struct hash {
  size_t operator()(TT const& tt) const { return std::hash<TT>()(tt); }
};

// Combinie hash values in a not-completely-evil way
// (I think this is borrowed from boost)
namespace {
template <class T>
inline void hash_combine(std::size_t& seed, T const& v) {
  seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl {
  static void apply(size_t& seed, Tuple const& tuple) {
    HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
    hash_combine(seed, std::get<Index>(tuple));
  }
};

template <class Tuple>
struct HashValueImpl<Tuple, 0> {
  static void apply(size_t& seed, Tuple const& tuple) { hash_combine(seed, std::get<0>(tuple)); }
};
} // namespace

// Hash for tuples
template <typename... TT>
struct hash<std::tuple<TT...>> {
  size_t operator()(std::tuple<TT...> const& tt) const {
    size_t seed = 0;
    HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
    return seed;
  }
};


// Hash for pairs
template <typename T, typename U>
struct hash<std::pair<T, U>> {
  std::size_t operator()(const std::pair<T, U>& x) const {
    size_t hVal = std::hash<T>()(x.first);
    hash_combine<U>(hVal, x.second);
    return hVal;
  }
};

}; // namespace hash_combine
}; // namespace polyscope
