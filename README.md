# CLAS12 sample generator

Two separate workflows:

1. **Create LUND files:** uniform acceptance samples or conversion of physical GENIE GST truth.
2. **Submit simulation on ifarm:** consume completed LUND files, configure GEMC/detector inputs, and submit GEMC followed by reconstruction as Slurm array jobs.

```text
run.csh --workflow create-lund -> Python build driver -> LUND application -> completed files
run.csh --workflow submit      -> setup_and_submit.csh -> submit.py -> sbatch array -> GEMC -> recon-util
```

On ifarm, use `source run.csh --workflow submit --lund-dir RUN/lundfiles`. The completed manifest supplies sample settings; optional CLI flags or `--config config/submission.conf` override simulation defaults. GEMC falls back to 5.14; submission checks the shared version directory, loads that module in its child environment, and verifies the exact `gemc` executable passed to Slurm. The default is preview; add `--execute` to submit and replace the selected simulation output directories while preserving LUND inputs. See the [setup and submission guide](docs/submit-simulation/guide.md).

This repository does not run the physical event generator or calculate final acceptance maps.

## First build and sample

Requirements: CMake 3.20+, a C++ compiler compatible with your ROOT installation, ROOT with Core/RIO/Hist/Physics/Tree/TreePlayer, and Python 3.9+ for tests and both workflow drivers, and csh/tcsh for the sourced ifarm launcher. Submission preview and execution require the ifarm module command, the requested shared GEMC version, and `recon-util`; only execution requires `sbatch`.

Clone with the pinned legacy reference submodule, or initialize it after an existing clone:

```bash
git clone --recurse-submodules REPOSITORY_URL
# Existing clone:
git submodule update --init --recursive
```

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure

build/debug/apps/uniform-lund-generator \
  --config config/samples/uniform-1e-5986MeV.conf \
  --events 100 \
  --output runs/first-electron
```

The resolved uniform run is written below `runs/first-electron/Uniform_sample_1e_5986MeV`. If that directory already exists, generation prints a warning, removes its previous contents and recreates it. Building and generation do not run `git clean` or submit jobs.

LUND output always preserves the established text conventions. Electron, proton, neutron, and charged-pion masses come from the external target source; photons are massless. Production momentum defaults are mixed p/1-p for the 1e electron and charged hadrons, and uniform p for neutrons. Sampled hadron momentum always extends to the beam energy; fixed 1 GeV/c momentum is a neutron-only option. Select electron–hadron samples with `--channel eh --hadron proton|neutron|pip|pim --hadron-region FD|CD`.

Open `runs/first-electron/Uniform_sample_1e_5986MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json` to see the resolved settings and output counts. LUND text is under `lundfiles/`; uniform diagnostics are stored once in `lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root`. Physical conversion does not create monitoring histograms.

## Documentation

Start at the [documentation home](docs/index.md). It presents the two workflows first and routes readers by task instead of exposing the complete reference at once.

| Subject | Use it for |
| --- | --- |
| [Getting started](docs/getting-started/index.md) | Install, build, run a small sample, and understand outputs |
| [Create LUND files](docs/create-lund/index.md) | Uniform generation, physical conversion, configuration, examples, and monitoring |
| [Submit simulation](docs/submit-simulation/index.md) | Preview and submit ifarm GEMC/reconstruction jobs |
| [Concepts and contracts](docs/concepts/index.md) | Architecture, sampling, LUND records, provenance, and scientific scope |
| [Development](docs/development/index.md) | Source reference, tests, documentation, wiki publishing, and [adding an event-generator adapter](docs/development/adding-event-generator.md) |
| [History and migration](docs/history/index.md) | Archived behavior, parity, and migration context |

Worked commands are grouped by workflow in the [LUND-creation examples](docs/create-lund/examples.md) and [submission examples](docs/submit-simulation/examples.md). The longer [checked-in command lists](tutorials/README.md) remain available for the established production matrix.

Maintained source is grouped first by workflow under `src/lund-generation/` and `src/slurm-submission/`; the shared dispatcher is under `src/launcher/`. The [architecture walkthrough](docs/concepts/architecture.md) maps these directories to build targets and runtime call chains.

The project keeps two narrow, RG-M-derived update boundaries. `src/lund-generation/external/targets.h` is an exact RG-M copy containing the latest target implementations available with GEMC 5.14 when adopted. `src/slurm-submission/external/submit_GEMC_sample.sh` is a modified RG-M-derived payload that keeps the source structure and usage pattern. Maintained adapters surround both files so later RG-M updates can be reviewed and incorporated without duplicating their geometry or detector commands; see [external inputs](docs/concepts/external-inputs.md).

The original source trees are retained in `legacy/` for comparison. Their public repository baseline is the [`legacy-v1.0.0` GitHub release tag](https://github.com/alons126/CLAS12-sample-generator/releases/tag/legacy-v1.0.0), which records the archived tree and the pinned Uniform submodule revision. `legacy/Uniform-sample-generator` remains a submodule of its independent upstream repository; its selected kernels are compiled only by parity tests. The legacy sources are retired from production use. Detector cards and reconstruction YAML are retained in `config/detector/`.

For local editing and server execution via `source run.csh`, read the [SSH workflow](docs/submit-simulation/ifarm-environment.md). When sourcing from outside the checkout, the user may set the optional `CLAS12_SAMPLES_DIR` environment variable to its absolute path; the project does not define it automatically. Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](docs/concepts/external-inputs.md).
