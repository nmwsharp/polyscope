#!/bin/sh
# run from project root
find src/ -path "./render/bindata/" -prune -name '*.cpp' -print0 | xargs -0 clang-format -i
find include/ -name '*.h' -print0 | xargs -0 clang-format -i
find include/ -name '*.ipp' -print0 | xargs -0 clang-format -i
find test/src/ -name '*.cpp' -print0 | xargs -0 clang-format -i
find test/include/ -name '*.h' -print0 | xargs -0 clang-format -i
find examples/ -path "./demo-app/build/" -prune -name '*.cpp' -print0 | xargs -0 clang-format -i
find examples/ -path "./demo-app/build/" -prune -name '*.h' -print0 | xargs -0 clang-format -i
