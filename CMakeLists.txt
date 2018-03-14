cmake_minimum_required(VERSION 3.1)

project(polyscope)

### Policy settings
cmake_policy(SET CMP0054 NEW)   # don't implicitly dereference inside if()

### Check if running as subproject or standalone
get_directory_property(HAVE_PARENT PARENT_DIRECTORY)

### Configure output locations
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

### Compiler options
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 ) # Emit a compile flags file to support completion engines 

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using Clang (linux or apple) or GCC
  message("Using clang/gcc compiler flags")
  SET(BASE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -g3")
  SET(DISABLED_WARNINGS " -Wno-unused-parameter -Wno-unused-variable -Wno-unused-private-field -Wno-unused-function -Wno-deprecated-declarations")
  SET(TRACE_INCLUDES " -H -Wno-error=unused-command-line-argument")

  # clang-specific things
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(BASE_CXX_FLAGS "${BASE_CXX_FLAGS} -ferror-limit=5 -fcolor-diagnostics")
  endif()


  SET(CMAKE_CXX_FLAGS "${BASE_CXX_FLAGS} ${DISABLED_WARNINGS}")
  #SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TRACE_INCLUDES}") # uncomment if you need to track down where something is getting included from
  #SET(CMAKE_CXX_FLAGS_DEBUG          "-g3")
  SET(CMAKE_CXX_FLAGS_DEBUG          "-g3 -fsanitize=address")
  SET(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELEASE        "-march=native -O3 -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  message("Using Visual Studio compiler flags")
  set(BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  set(BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # parallel build
  SET(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4267\"")  # ignore conversion to smaller type (fires more aggressively than the gcc version, which is annoying)
  SET(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4244\"")  # ignore conversion to smaller type (fires more aggressively than the gcc version, which is annoying)
  SET(DISABLED_WARNINGS "${DISABLED_WARNINGS} /wd\"4305\"")  # ignore truncation on initialization
  SET(CMAKE_CXX_FLAGS "${BASE_CXX_FLAGS} ${DISABLED_WARNINGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

  add_definitions(/D "_CRT_SECURE_NO_WARNINGS")
  add_definitions (-DNOMINMAX)
else()
  # unrecognized
  message( FATAL_ERROR "Unrecognized compiler [${CMAKE_CXX_COMPILER_ID}]" )
endif()

### Do anything needed for dependencies and bring their stuff in to scope
add_subdirectory(deps)

### Add things for this project
add_subdirectory(src)

### Build the demo viewer application
option(BUILD_POLYSCOPE_VIEWER "Build the viewer app." ON)
if(BUILD_POLYSCOPE_VIEWER)
    add_subdirectory(src/viewer-app)
endif()

option(BUILD_POLYSCOPE_DEMO "Build the demo app." ON)
if(BUILD_POLYSCOPE_DEMO)
    add_subdirectory(src/demo-app)
endif()

### Pass relevant variables to parent projects
if(HAVE_PARENT AND HAVE_SUITESPARSE)
    SET(HAVE_SUITESPARSE TRUE PARENT_SCOPE)
endif()


