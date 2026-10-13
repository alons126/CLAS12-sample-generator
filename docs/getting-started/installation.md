# Dependencies and execution environments

## Required software

LUND creation requires:

- CMake 3.20 or later.
- a C++ compiler compatible with the selected ROOT installation; ROOT must report C++17 or later.
- Git for cloning and launcher updates; tcsh for the ifarm launcher. Direct local CMake/application commands do not require tcsh.
- [ROOT](https://github.com/root-project/root):
  - with Core, Physics, and RIO.
  - Hist, Graf, and Gpad for the uniform LUND creator.
  - Tree and TreePlayer for the physical LUND converter.
- Python 3.9 or later for the launcher and submission tools.

The executables do not link to [GENIE](https://github.com/GENIE-MC/Generator). The physical LUND converter reads existing GENIE GST files through ROOT.

Simulation submission additionally requires an ifarm account, Slurm, [GEMC](https://github.com/gemc/clas12Tags/tree/main), [COATJAVA](https://github.com/JeffersonLab/coatjava), and storage visible to the Slurm workers. The [ifarm environment guide](../submit-simulation/ifarm-environment.md) defines the expected login setup and versions.

For production, clone `main`. When reading this file in the repository, replace `REPOSITORY_URL` with the HTTPS clone address from GitHub’s **Code** menu. Wiki generation inserts the publishing repository’s address, including when publishing from a fork:

```bash
git \
    clone \
    --branch main \
    REPOSITORY_URL \
    CLAS12-sample-generator
cd CLAS12-sample-generator
```

## Prepare a local build environment

The [publication workflow](../../.github/workflows/prepare-release.yml) defines an Ubuntu 24.04 build environment using conda-forge ROOT 6.34, a C++ compiler, CMake, and Ninja. The following Bash recipe follows those dependency choices; it is not a claim that every operating system has completed detector validation. Install [Miniforge](https://github.com/conda-forge/miniforge#install) first and open a shell where `conda` is available. ROOT also documents [Conda installation](https://root.cern/install/#conda).

```bash
conda \
    create \
    --name clas12-samples \
    --override-channels \
    --channel conda-forge \
    --strict-channel-priority \
    'root_base=6.34' \
    cxx-compiler \
    'cmake>=3.20' \
    ninja \
    'python>=3.9'
conda activate clas12-samples
```

Activation selects this environment's programs and compiler. Activate it again in each new terminal before building or running the local applications. Keep a site-provided ROOT installation and a Conda installation in separate shell sessions. For another installation method, follow the [ROOT installation instructions](https://root.cern/install/) and use its compatible compiler.

## Verify prerequisites

Before building, print the selected tool versions and ROOT location:

```bash
cmake --version
python3 --version
root-config --version
root-config --prefix
root-config --cxx
root-config --cflags
```

Require CMake 3.20+, Python 3.9+, and a ROOT/compiler combination using at least C++17. During configuration, CMake reports the compiler it selected and checks ROOT's published standard and required libraries. Confirm the compiler belongs to the environment you intended. If you switch ROOT or compiler installations, use a fresh build directory as explained in the [build reference](../development/building.md#fresh-debug-build).

For ifarm, complete the [login setup and command checks](../submit-simulation/ifarm-environment.md#login-environment) instead of installing a local Conda environment into the production shell. See [troubleshooting](troubleshooting.md) for missing commands or dependency failures.

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
