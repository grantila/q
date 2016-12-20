# Building q and q-io

When building `q`, you won't need any external libraries - just a C++11 compatible compiler.

However, building `q-io` requires `libuv`.


# Prerequisites

## General

`q` is built using your favorite compiler, and the build is orchestrated by CMake. You'll need CMake and a C++ compiler toolchain.

### Mac OS (using homebrew)

Ensure you have XCode installed, including the _Command line tools_.

```sh
brew install cmake
```

### Debian, Ubuntu and other dpkg-based distributions

```sh
sudo apt install cmake
```

## q-io

`q-io` requires `libuv` (and development files) to be installed.

### Mac OS (using homebrew)

```sh
brew install libuv
```

### Debian, Ubuntu and other dpkg-based distributions

```sh
sudo apt install libuv-dev
```


## Building

First, ensure the prerequisites are met - that you have the necessary libraries and tools installed. Install the required dependencies for `q` (and `q-io` if you whish), then prepare the build for your platform.

Either just run:

```sh
./build.sh
```

and then open the projects files located under the `build/` directory.

Or run CMake manually with the generator of your choice, e.g.

```sh
cmake -G [your_target_system_generator] -Bbuild -H.
```

and build using your target system (make, XCode, Visual Studio, etc).


## Build using custom dependencies

To build `q-io` with a custom built libuv, unpack the `libuv` in the `3rdparty/` directory (as `3rdparty/libuv-x.y.z`) and run `./build.sh deps` to build libuv and prepare CMake to use _that_ instead of the system libuv. You can also use any other already-built `libuv` by specifying a `LIBUV_PATH` environment variable before running `./build.sh`, i.e. `LIBUV_PATH=[the path] ./build.sh`. This path should contain a `lib/` directory and a `include/` directory.
