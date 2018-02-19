# Basic Project

This is a basic project using Polyscope showing how to set up the build system and use git submodules.

```
// Download the files in this directory and unpack
curl -LJO https://github.com/nmwsharp/polyscope/examples/basic_project/these_files.tgz
tar -xzvf these_files.tgz
rm these_files.tgz

// Make some directories
mkdir deps && mkdir build

// Initialize a git repo (skip this if you're already in a repo)
git init .

// Check out polyscope as a git submodule
cd deps
git submodule add -b master https://github.com/nmwsharp/polyscope
git submodule update --remote
cd ..
git submodule update --init --recursive

// Build
cd build && cmake .. && make -j3

```
