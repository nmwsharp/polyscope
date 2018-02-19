# Basic Project

This is a basic project using Polyscope, showing how to set up the build system and use git submodules. The resulting application visualizes a mesh with a few data members, generates a point cloud, and demonstrates a user-callback GUI.

We recommend that you use utilize [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to download polyscope and its dependencies. With submodules, it is easy to stay up to date with Polyscope updates, as well as pushing your own contributions upstream.

## Download and build: short version
"I'm in a hurry and I like running code from strangers on the internet."


**(Option 1)** This downloads and builds the example, leaving you in a new git repository:
```
curl -LJO https://github.com/nmwsharp/polyscope/raw/master/examples/basic_project/these_files.tgz && tar -xzvf these_files.tgz && rm these_files.tgz && git init . && cd deps && git submodule add -b master https://github.com/nmwsharp/polyscope && git submodule update --remote && cd .. && git submodule update --init --recursive && mkdir build && cd build && cmake .. && make -j3 && cd ..
```
run `rm -rf .git` to avoid avoid leaving a git repo behind, but DO NOT do so in an existing repo, it can destroy local updates

**(Option 2)** This downloads and builds the example in an existing git repository (ie, if you just created and cloned a new repo on github):
```
curl -LJO https://github.com/nmwsharp/polyscope/raw/master/examples/basic_project/these_files.tgz && tar -xzvf these_files.tgz && rm these_files.tgz && git init . && cd deps && git submodule add -b master https://github.com/nmwsharp/polyscope && git submodule update --remote && cd .. && git submodule update --init --recursive && mkdir build && cd build && cmake .. && make -j3 && cd ..
```

##  Download and build: long version
Download the files to the current directory and unpack
```
curl -LJO https://github.com/nmwsharp/polyscope/raw/master/examples/basic_project/these_files.tgz
tar -xzvf these_files.tgz
rm these_files.tgz
```

Initialize a git repo (skip this if you're already in a repo)
```
git init .
```

Check out polyscope and its dependencies via a git submodules
```
cd deps
git submodule add -b master https://github.com/nmwsharp/polyscope
git submodule update --remote
cd ..
git submodule update --init --recursive
```

Compile the code
```
mkdir build && cd build && cmake .. && make -j3 && cd ..
```

Run the example
```
./build/bin/polyscope_basic_demo /path/to/your/mesh.obj
```

Note: run `rm -rf .git` to avoid avoid leaving a git repository behind, but DO NOT do so in an existing repository, it can destroy local updates.
