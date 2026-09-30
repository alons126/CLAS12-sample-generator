# Dependencies and build entry points

## Dependencies

- CMake 3.20 or later and a C++ compiler compatible with ROOT.
- ROOT with Core, Physics, RIO, Hist, Graf and Gpad; GENIE conversion additionally uses Tree and TreePlayer.
- Python 3.9+ for the simulation and submission scripts.
- **For ifarm simulation submission and Slurm execution:**
  - GEMC (version 5.14 or above) and COATJAVA (version 10.0.7) in the configured environment.
  - Slurm `sbatch` and access to the shared input/output paths.

The LUND-creation executables depend on ROOT but do not link against GENIE. The physical LUND converter reads existing GENIE GST files through ROOT tree I/O.

## Build through `run.csh`

On ifarm, use `source run.csh` from a csh/tcsh login shell. For `--workflow create-lund`, the launcher configures and builds both LUND applications before running the selected one. The checked-in defaults use `build/release`, a Release build, and four parallel build workers; [launcher settings](../../config/run.json.md) documents the available overrides. Users following this path do not need to invoke CMake directly.

The ifarm checkout is intentionally disposable and is refreshed from Git before the workflow begins. Keep valuable edits in the development checkout and read the [ifarm environment guide](../submit-simulation/ifarm-environment.md) before running there. The submission workflow does not compile the LUND applications; it checks the configured GEMC and COATJAVA environment and submits the existing LUND files.

## Build locally

Local development uses CMake directly so it can compile and run the executables without the ifarm launcher. Follow the [quickstart build](quickstart.md#build) for the single standard Debug build. It produces:

- `build/debug/apps/uniform-lund-creator`
- `build/debug/apps/event-generator-to-lund-converter`

Both executables support `--help` and return a nonzero status on failure. Contributors who need another build type, an explicit ROOT location, a partial build, or compiler troubleshooting should use the [developer build reference](../development/building.md). These build operations compile software only; they do not create samples or submit simulation jobs.
