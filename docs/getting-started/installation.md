# Building and installation

## Dependencies

- CMake 3.20 or later and a C++ compiler compatible with ROOT.
- ROOT with Core, Physics, RIO, Hist, Graf and Gpad; GENIE conversion additionally uses Tree and TreePlayer.
- Python 3.9+ for the simulation and submission scripts.
- At simulation runtime only: GEMC and `recon-util` in the configured environment.
- At submission runtime only: Slurm `sbatch` and access to the shared input/output paths.

HIPO/CLAS12ROOT/MPI/GENIE development libraries are not linked by these generators. GST conversion uses ROOT tree I/O.

## Build commands

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 4
```

Executables are `build/debug/apps/uniform-lund-creator` and `build/debug/apps/event-generator-to-lund-converter`. Both support `--help` and return nonzero on failure.

For production:

```bash
cmake -S . -B build/release -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel 4
```

Select ROOT explicitly if discovery fails:

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(root-config --prefix)"
```

`-S .` selects the checkout as the source tree, `-B` selects an out-of-source build directory, `-G "Unix Makefiles"` keeps the supported build-tool choice explicit, and `CMAKE_BUILD_TYPE` selects Debug or Release compiler settings. The configure and build commands address the same selected directory directly.

The project adopts `ROOT_CXX_STANDARD`, so the compiled application uses the same language standard as the chosen ROOT. It does not force C++17 against a C++20 ROOT installation. When changing ROOT or compiler installations, configure a fresh build directory. Add machine-specific CMake cache settings as additional `-DNAME=VALUE` arguments or use a separate build directory.

## Individual workflows

```bash
cmake -S . -B build/uniform-only -DBUILD_GENIE=OFF
cmake --build build/uniform-only --parallel 4

cmake -S . -B build/genie-only -DBUILD_UNIFORM=OFF
cmake --build build/genie-only --parallel 4
```

Build settings never initiate sample generation or submission.

## Installation

```bash
cmake --install build/release --prefix /path/to/install
```

The LUND executables and external `submit_GEMC_sample.sh` payload are installed under `bin/`; example settings and detector resources are under `share/clas12-samples/config/`. ROOT must remain available for LUND creation. Submission uses the checkout’s sourced setup script; its payload and inputs must be visible on worker nodes.

The supported checkout entry point is `source run.csh` in csh/tcsh; see [SSH execution](../submit-simulation/ifarm-environment.md). Geometry source, LUND format, gcard provenance and the required energy-dependent field settings are documented in [external inputs](../concepts/external-inputs.md).

The [unified external GEMC payload](../submit-simulation/worker-reference.md) documents `src/workflows/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python setup and its sourced shell bridge.
