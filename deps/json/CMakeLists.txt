add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

target_include_directories(nlohmann_json INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(nlohmann_json INTERFACE cxx_std_11)

install(TARGETS nlohmann_json EXPORT ${EXPORTED_TARGETS_NAME} DESTINATION "${INSTALL_LIB_DIR}")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION "${INSTALL_INCLUDE_DIR}")
