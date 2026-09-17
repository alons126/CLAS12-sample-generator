# CLAS12 sample generator

One compiled project for two sources of CLAS12 simulation input:

- **Uniform samples** for acceptance-map studies: $(e,e')$, electron–hadron samples with proton, neutron, pip, or pim in FD or CD.
- **Physical samples**: conversion of existing GENIE `gst` ROOT trees to LUND.

Both write LUND files and a run manifest. The submission workflow sends completed files through GEMC and reconstruction on ifarm Slurm. This repository does not run the physical event generator itself or calculate final acceptance maps.

## First build and sample

Requirements: CMake 3.20+, a C++ compiler compatible with your ROOT installation, ROOT with Core/RIO/Hist/Physics/Tree/TreePlayer, and Python 3.9+ for tests and simulation scripts. GEMC and `recon-util` are needed only when executing simulation.

Clone with the pinned legacy reference submodule, or initialize it after an existing clone:

```bash
git clone --recurse-submodules REPOSITORY_URL
# Existing clone:
git submodule update --init --recursive
```

```bash
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug

build/debug/apps/clas12-uniform \
  --config config/samples/uniform-1e-5986.conf \
  --events 100 \
  --output runs/first-electron
```

The resolved uniform run is written below `runs/first-electron/Uniform_sample_1e_5986MeV`. If that directory already exists, generation prints a warning, removes its previous contents and recreates it. Building and generation do not run `git clean` or submit jobs.

The default LUND format preserves legacy text conventions; use `--lund-format precise` for higher precision. Production momentum defaults are mixed p/1-p for the 1e electron and charged hadrons, and uniform p for neutrons. Fixed 1 GeV/c momentum is a neutron-only option. Select electron–hadron samples with `--channel eh --hadron proton|neutron|pip|pim --hadron-region FD|CD`.

Open `runs/first-electron/Uniform_sample_1e_5986MeV/manifest.json` to see the resolved settings and output counts. LUND text is under `lundfiles/`; diagnostic histograms are in `monitoring.root`.

## Where to start

Read the [newcomer guide](docs/index.md), then [build instructions](docs/building.md) and the [architecture walkthrough](docs/architecture.md).

Maintained shared C++ is grouped by responsibility under `src/config/`, `src/lund/`, `src/geometry/`, `src/monitoring/`, and `src/support/`. Source-specific implementations match the executable names in `src/clas12-uniform/` and `src/clas12-generator-to-lund/`; GENIE is nested under the latter as one physical-input adapter. Protected imported files remain under `src/common/external/`. The architecture walkthrough maps these directories to the build targets and runtime call chain.

- [Uniform generation](docs/uniform-samples.md)
- [Physical event-generator conversion](docs/genie-to-lund-conversion.md)
- [GEMC, reconstruction and Slurm](docs/gemc-reconstruction-batch-submission.md)
- [Configuration reference](docs/configuration.md)
- [Technical note and complete reference](docs/technical-note.md)
- [Legacy parity and validation](docs/validation.md)
- [Migration from the imported repositories](docs/migration.md)

The original source trees are retained in `legacy/` for comparison. `legacy/Uniform-sample-generator` is pinned as a submodule to its independent upstream repository; its selected kernels are compiled only by parity tests. The legacy sources are retired from production use. Detector cards and reconstruction YAML are retained in `config/detector/`.

For local editing and server execution via `source run.csh`, read the [SSH workflow](docs/ssh-workflow.md). When sourcing from outside the checkout, the user may set the optional `CLAS12_SAMPLES_DIR` environment variable to its absolute path; the project does not define it automatically. Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](docs/external-inputs.md).
