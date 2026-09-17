# Building and testing

## Dependencies

- CMake 3.20 or later and a C++ compiler compatible with ROOT.
- ROOT with Core, Physics, RIO, Hist, Graf and Gpad; GENIE conversion additionally uses Tree and TreePlayer.
- Python 3.9+ for CTest integration checks and the simulation/submission scripts.
- At simulation runtime only: GEMC and `recon-util` in the configured environment.
- At submission runtime only: Slurm `sbatch` and access to the shared input/output paths.

HIPO/CLAS12ROOT/MPI/GENIE development libraries are not linked by these generators. GST conversion uses ROOT tree I/O.

## Build commands

The uniform legacy-parity reference is a pinned Git submodule. A fresh checkout should use `git clone --recurse-submodules`; for an existing checkout, run:

```bash
git submodule update --init --recursive
```

CMake configuration fails to provide the uniform reference tests if the submodule files are absent. The supported ifarm `run.csh` path initializes the pinned revision automatically after updating the disposable checkout.

```bash
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug
```

Executables are `build/debug/apps/clas12-uniform` and `build/debug/apps/clas12-generator-to-lund`. Both support `--help` and return nonzero on failure.

For production:

```bash
cmake --preset release
cmake --build --preset release --parallel 4
```

Select ROOT explicitly if discovery fails:

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH="$(root-config --prefix)"
```

The project adopts `ROOT_CXX_STANDARD`, so the compiled application uses the same language standard as the chosen ROOT. It does not force C++17 against a C++20 ROOT installation. When changing ROOT or compiler installations, configure a fresh build directory. Machine-specific settings can go in an untracked `CMakeUserPresets.json`.

## Individual workflows

```bash
cmake -S . -B build/uniform-only -DBUILD_GENIE=OFF
cmake --build build/uniform-only --parallel 4
ctest --test-dir build/uniform-only --output-on-failure

cmake -S . -B build/genie-only -DBUILD_UNIFORM=OFF
cmake --build build/genie-only --parallel 4
ctest --test-dir build/genie-only --output-on-failure
```

Use `-DBUILD_TESTING=OFF` to omit tests and their Python dependency. Build settings never initiate sample generation or submission.

## Installation

```bash
cmake --install build/release --prefix /path/to/install
```

The executables, `clas12-simulate` and `clas12-submit` are installed under `bin/`; example settings and detector resources are under `share/clas12-samples/config/`. ROOT must remain available at runtime. The Slurm runner path and inputs must be visible on worker nodes.

## What tests establish

CTest generates small temporary samples and checks LUND header/particle counts, mass-shell energies, target vertices, channel prescriptions, deterministic seeds, configuration rejection, overwrite protection, and GENIE process selection/file splitting. A synthetic GST fixture exercises all retained species and a skipped process. Simulation tests use executable stubs to verify argument handling, exact per-file counts, dry runs, and stopping after GEMC failure.

Independent adapters also execute external reference kernels/conversion. They compare LUND bytes and original histogram bins with explicitly matched settings and compare legacy job-command arguments. Distribution tests separately compare the maintained 1e and charged-hadron mixtures, uniform neutron momentum, and flat-theta angles to their analytic CDFs. See the [validation matrix](validation.md).

These are local software checks. They do not establish detector-card suitability or replace running GEMC/reconstruction and validating acceptance maps at JLab.

The supported checkout entry point is `source run.csh` in csh/tcsh; see [SSH execution](ssh-workflow.md). Geometry source, LUND format, gcard provenance and the required energy-dependent field settings are documented in [external inputs](external-inputs.md).

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
