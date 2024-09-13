# Create a library for the viewer code
add_library(
    stb
    stb_impl.cpp
)

target_include_directories(stb PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

set_target_properties(stb PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

install(TARGETS stb EXPORT ${EXPORTED_TARGETS_NAME} DESTINATION "${INSTALL_LIB_DIR}")
