#!/bin/sh
# run from project root
clang-format -i src/*.cpp src/render/*.cpp src/render/opengl/*.cpp src/render/opengl/shaders/*.cpp src/render/mock_opengl/*.cpp
find include/ -name '*.h' -print0 | xargs -0 clang-format -i
find include/ -name '*.ipp' -print0 | xargs -0 clang-format -i
find test/src/ -name '*.cpp' -print0 | xargs -0 clang-format -i
find test/include/ -name '*.h' -print0 | xargs -0 clang-format -i
find examples/ -path "./demo-app/build/" -prune -name '*.cpp' -print0 | xargs -0 clang-format -i
find examples/ -path "./demo-app/build/" -prune -name '*.h' -print0 | xargs -0 clang-format -i
