// Copyright 2017-2023, Nicholas Sharp and the Polyscope contributors. https://polyscope.run

#pragma once

namespace polyscope {
namespace render {

template <typename T>
ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string bufferName) {
  return getManagedBufferRegistryRef<T>().getManagedBuffer(structureName, bufferName);
}

// Get a reference to any buffer that currently exists in Polyscope, by name
// (this one fetches buffers associated with quantities)
template <typename T>
ManagedBuffer<T>& getManagedBuffer(std::string structureName, std::string quantityName, std::string bufferName) {
  return getManagedBufferRegistryRef<T>().getManagedBuffer(structureName, quantityName, bufferName);
}

// clang-format off

template<> inline ManagedBufferRegistry<float>&                    getManagedBufferRegistryRef<float>()                    { return detail::managedBufferRegistry_float; }
template<> inline ManagedBufferRegistry<double>&                   getManagedBufferRegistryRef<double>()                   { return detail::managedBufferRegistry_double; }
template<> inline ManagedBufferRegistry<glm::vec2>&                getManagedBufferRegistryRef<glm::vec2>()                { return detail::managedBufferRegistry_vec2; }
template<> inline ManagedBufferRegistry<glm::vec3>&                getManagedBufferRegistryRef<glm::vec3>()                { return detail::managedBufferRegistry_vec3; }
template<> inline ManagedBufferRegistry<glm::vec4>&                getManagedBufferRegistryRef<glm::vec4>()                { return detail::managedBufferRegistry_vec4; }
template<> inline ManagedBufferRegistry<std::array<glm::vec3,2>>&  getManagedBufferRegistryRef<std::array<glm::vec3,2>>()  { return detail::managedBufferRegistry_arr2vec3; }
template<> inline ManagedBufferRegistry<std::array<glm::vec3,3>>&  getManagedBufferRegistryRef<std::array<glm::vec3,3>>()  { return detail::managedBufferRegistry_arr3vec3; }
template<> inline ManagedBufferRegistry<std::array<glm::vec3,4>>&  getManagedBufferRegistryRef<std::array<glm::vec3,4>>()  { return detail::managedBufferRegistry_arr4vec3; }
template<> inline ManagedBufferRegistry<uint32_t>&                 getManagedBufferRegistryRef<uint32_t>()                 { return detail::managedBufferRegistry_uint32; }
template<> inline ManagedBufferRegistry<int32_t>&                  getManagedBufferRegistryRef<int32_t>()                  { return detail::managedBufferRegistry_int32; }
template<> inline ManagedBufferRegistry<glm::uvec2>&               getManagedBufferRegistryRef<glm::uvec2>()               { return detail::managedBufferRegistry_uvec2; }
template<> inline ManagedBufferRegistry<glm::uvec3>&               getManagedBufferRegistryRef<glm::uvec3>()               { return detail::managedBufferRegistry_uvec3; }
template<> inline ManagedBufferRegistry<glm::uvec4>&               getManagedBufferRegistryRef<glm::uvec4>()               { return detail::managedBufferRegistry_uvec4; }

// clang-format on

} // namespace render
} // namespace polyscope
