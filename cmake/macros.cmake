option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compile_commands.json" ON)

set(CONFIG_BASENAME "${PROJECT_NAME}Config")
set(CONFIG_FILENAME "${CONFIG_BASENAME}.cmake")
set(VERSION_FILENAME "${CONFIG_BASENAME}Version.cmake")
set(EXPORTED_TARGETS_NAME "${PROJECT_NAME}Targets")
set(EXPORTED_TARGETS_FILE "${EXPORTED_TARGETS_NAME}.cmake")

include(GNUInstallDirs)
set(INSTALL_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")
set(INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}")
set(INSTALL_PKGCONFIG_DIR "${CMAKE_INSTALL_DATADIR}/pkgconfig")

macro (package)
  include(CMakePackageConfigHelpers)

  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${VERSION_FILENAME}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/cmake_config.cmake.in"
    "${PROJECT_BINARY_DIR}/${CONFIG_FILENAME}"
    INSTALL_DESTINATION ${INSTALL_CMAKE_DIR})

  install(
    EXPORT ${EXPORTED_TARGETS_NAME}
    FILE ${EXPORTED_TARGETS_FILE}
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${INSTALL_CMAKE_DIR})

  install(FILES "${PROJECT_BINARY_DIR}/${CONFIG_FILENAME}"
                "${PROJECT_BINARY_DIR}/${VERSION_FILENAME}"
          DESTINATION ${INSTALL_CMAKE_DIR})

  configure_file("${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                 "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake" @ONLY IMMEDIATE)

  if (NOT TARGET uninstall)
    add_custom_target(uninstall "${CMAKE_COMMAND}" -P
                                "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake")
  else ()
    add_custom_target(
      ${PROJECT_NAME}_uninstall "${CMAKE_COMMAND}" -P
                                "${PROJECT_BINARY_DIR}/cmake_uninstall.cmake")
    add_dependencies(uninstall ${PROJECT_NAME}_uninstall)
  endif ()
endmacro ()
