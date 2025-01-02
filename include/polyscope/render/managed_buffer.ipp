// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {
namespace render {

template <typename T>
ManagedBuffer<T>& ManagedBufferRegistry::getManagedBuffer(std::string name) {
  return ManagedBufferMap<T>::getManagedBufferMapRef(this).getManagedBuffer(name);
}

template <typename T>
bool ManagedBufferRegistry::hasManagedBuffer(std::string name) {
  return ManagedBufferMap<T>::getManagedBufferMapRef(this).hasManagedBuffer(name);
}

template <typename T>
void ManagedBufferRegistry::addManagedBuffer(ManagedBuffer<T>* buffer) {
  ManagedBufferMap<T>::getManagedBufferMapRef(this).addManagedBuffer(buffer);
}

template <typename T>
void ManagedBufferMap<T>::addManagedBuffer(ManagedBuffer<T>* buffer) {
  for (ManagedBuffer<T>* buff : allBuffers) {
    if (buff->name == buffer->name) {
      exception("managed buffer map already contains buffer of name " + buff->name);
    }
  }
  allBuffers.push_back(buffer);
}


template <typename T>
ManagedBuffer<T>& ManagedBufferMap<T>::getManagedBuffer(std::string name) {

  // WARNING: this logic is duplicated between has- and get- functions

  auto endsWith = [](std::string const& str, std::string const& query) -> bool {
    if (query.size() > str.size()) return false;
    return std::equal(query.rbegin(), query.rend(), str.rbegin());
  };


  for (ManagedBuffer<T>* buff : allBuffers) {
    if (endsWith(buff->name, "#" + name)) {
      return *buff;
    }
  }

  exception("managed buffer map does not contain buffer of name " + name);
  return *allBuffers[0]; // invalid, never executed
}

template <typename T>
bool ManagedBufferMap<T>::hasManagedBuffer(std::string name) {

  // WARNING: this logic is duplicated between has- and get- functions

  auto endsWith = [](std::string const& str, std::string const& query) -> bool {
    if (query.size() > str.size()) return false;
    return std::equal(query.rbegin(), query.rend(), str.rbegin());
  };


  for (ManagedBuffer<T>* buff : allBuffers) {
    if (endsWith(buff->name, "#" + name)) {
      return true;
    }
  }

  return false;
}

} // namespace render
} // namespace polyscope
