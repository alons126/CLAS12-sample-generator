# Developer build reference

Ordinary ifarm operation uses `source run.csh`, which configures and builds LUND creation automatically. Use the commands on this page for local source development, build troubleshooting, or a deliberately restricted build. Run them from the repository root.

## Standard local build

Configure a fresh Debug build and compile with four workers:

```bash
cmake \
    -S . \
    -B build/debug \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug
cmake \
    --build build/debug \
    --parallel 4
```

`-S .` selects the checkout as the source tree. `-B` selects a separate build directory, and both commands address that same directory. `CMAKE_BUILD_TYPE` accepts `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel`. Use a different build directory for each compiler, ROOT installation, or build type instead of reusing an incompatible CMake cache.

The project adopts the selected ROOT installation's `ROOT_CXX_STANDARD`. It rejects ROOT installations that publish a C++ standard older than C++17 rather than forcing a standard that may be incompatible with ROOT.

## Select ROOT explicitly

CMake normally discovers ROOT from the active environment. If discovery fails, configure a fresh directory with ROOT's installation prefix:

```bash
cmake \
    -S . \
    -B build/debug-with-root \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$(root-config --prefix)"
cmake \
    --build build/debug-with-root \
    --parallel 4
```

Confirm that `root-config` describes the ROOT installation intended for the build before configuring.

## Build one LUND application

Both LUND applications are enabled by default. Disable the application and ROOT components that are not needed for a focused development build:

```bash
cmake \
    -S . \
    -B build/uniform-only \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_GENIE=OFF
cmake \
    --build build/uniform-only \
    --parallel 4

cmake \
    -S . \
    -B build/physical-only \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_UNIFORM=OFF
cmake \
    --build build/physical-only \
    --parallel 4
```

`BUILD_GENIE=OFF` omits the physical LUND converter. `BUILD_UNIFORM=OFF` omits the uniform LUND creator and its monitoring components. Build settings never create LUND files or submit simulation jobs.

## Validate a development build

Run the affected executable with `--help`, exercise the changed path with a small event count, and run any validation targets provided by the checkout:

```bash
ctest \
    --test-dir build/debug \
    --output-on-failure
```

A successful build and local test establish software behavior only. They do not establish detector-level acceptance equivalence; see the [scientific validation boundaries](validation.md).
