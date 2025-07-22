add_library(MarchingCube INTERFACE)
add_library(MarchingCube::MarchingCube ALIAS MarchingCube)

target_include_directories(MarchingCube INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

install(TARGETS MarchingCube EXPORT ${EXPORTED_TARGETS_NAME} DESTINATION "${INSTALL_LIB_DIR}")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION "${INSTALL_INCLUDE_DIR}")
