# Dependencies and execution environments

## Required software

LUND creation requires:

- CMake 3.20 or later.
- a C++ compiler compatible with the selected ROOT installation.
- [ROOT](https://github.com/root-project/root):
  - with Core, Physics, and RIO.
  - Hist, Graf, and Gpad for the uniform LUND creator.
  - Tree and TreePlayer for the physical LUND converter.
- Python 3.9 or later for the launcher and submission tools.

The executables do not link to [GENIE](https://github.com/GENIE-MC/Generator). The physical LUND converter reads existing GENIE GST files through ROOT.

Simulation submission additionally requires an ifarm account, Slurm, [GEMC](https://github.com/gemc/clas12Tags/tree/main), [COATJAVA](https://github.com/JeffersonLab/coatjava), and storage visible to the Slurm workers. The [ifarm environment guide](../submit-simulation/ifarm-environment.md) defines the expected login setup and versions.

Clone the repository by replacing the placeholder with its address:

```bash
git clone REPOSITORY_URL
cd CLAS12-sample-generator
```

## Choose the right entry point

Use [`run.csh`](../../run.csh) to run workflows on ifarm from a csh/tcsh shell. Before starting, it discards uncommitted changes and deletes untracked and ignored files, except the `build/` directory. It then updates the code from Git and builds the LUND applications when requested. Keep your development edits in a separate local copy and your generated samples outside the ifarm repository directory. The default build uses Release mode, stores compiled files in `build/release`, and runs four build tasks in parallel.

For local development, use CMake to build, then run the compiled applications. These commands do not delete files or update the checkout:

```bash
cmake \
    -S . \
    -B build/debug \
    -DCMAKE_BUILD_TYPE=Debug
cmake \
    --build build/debug \
    --parallel 4
```

The build produces:

- `build/debug/apps/uniform-lund-creator`
- `build/debug/apps/event-generator-to-lund-converter`

Both programs support `--help`. Developers who need a partial build, another ROOT installation, or another build type should use the [developer build reference](../development/building.md). Launcher build defaults and overrides are documented in [`config/run.json.md`](../../config/run.json.md).

Building does not create a sample or submit a job. Those actions require an explicit workflow command.
