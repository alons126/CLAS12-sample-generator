# Developer build reference

Use direct CMake builds for source development. Normal ifarm operation uses `run.csh`, which reads `config/run.json` and builds LUND applications automatically.

## Fresh Debug build

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

`-S` selects the source checkout. `-B` selects a separate binary tree. Keep separate build directories for different compilers, ROOT installations, and build types rather than reusing an incompatible cache.

The project adopts the C++ standard published by ROOT and requires at least C++17. It does not force a different standard that might be incompatible with the selected ROOT build.

## Select ROOT explicitly

If CMake cannot find the intended ROOT installation, configure a fresh tree with its prefix:

```bash
cmake \
    -S . \
    -B build/debug-with-root \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="$(root-config --prefix)"
cmake \
    --build build/debug-with-root \
    --parallel 4
```

Confirm that `root-config` names the intended installation before configuring.

## Build one LUND source

Both applications are enabled by default. A focused build can omit the unused application and its ROOT components:

```bash
cmake \
    -S . \
    -B build/uniform-only \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_GENIE=OFF
cmake \
    --build build/uniform-only \
    --parallel 4
```

```bash
cmake \
    -S . \
    -B build/physical-only \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_UNIFORM=OFF
cmake \
    --build build/physical-only \
    --parallel 4
```

`BUILD_GENIE=OFF` omits the physical LUND converter. `BUILD_UNIFORM=OFF` omits the uniform LUND creator and monitoring code.

## Check the built interface

```bash
build/debug/apps/uniform-lund-creator --help
build/debug/apps/event-generator-to-lund-converter --help
```

Then run a small deterministic sample through the path you changed and inspect its files, counts, manifest, and diagnostics. Build commands alone never create LUND files or submit simulation.
