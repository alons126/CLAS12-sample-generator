# Dependencies and execution environments

## Required software

LUND creation requires:

- CMake 3.20 or later;
- a C++ compiler compatible with the selected ROOT installation;
- [ROOT](https://github.com/root-project/root) with Core, Physics, and RIO;
- ROOT Hist, Graf, and Gpad for the uniform LUND creator;
- ROOT Tree and TreePlayer for the physical LUND converter; and
- Python 3.9 or later for the launcher and submission tools.

The executables do not link to GENIE. The physical LUND converter reads existing GENIE GST files through ROOT.

Simulation submission additionally requires an ifarm account, Slurm, GEMC, COATJAVA, and storage visible to the Slurm workers. The [ifarm environment guide](../submit-simulation/ifarm-environment.md) defines the expected login setup and versions.

Clone the collaboration's fork by replacing the placeholder with its HTTPS or SSH address:

```bash
git clone REPOSITORY_URL
cd CLAS12-sample-generator
```

## Choose the right entry point

Use [`run.csh`](../../run.csh) for normal ifarm operation. It is a csh/tcsh entry point that refreshes a disposable checkout, configures and builds LUND applications when requested, and starts the selected workflow. The checked-in build defaults are Release, `build/release`, and four build workers.

Use CMake and the compiled executables directly for local development. This avoids the disposable-checkout refresh and makes build failures easier to inspect:

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
