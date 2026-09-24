# CLAS12 sample generator

Two separate workflows:

1. **Create LUND files:** uniform acceptance samples or conversion of physical GENIE GST truth.
2. **Submit simulation on ifarm:** consume completed LUND files, configure GEMC/detector inputs, and submit GEMC followed by reconstruction as Slurm array jobs.

```text
run.csh --workflow create-lund -> Python build driver -> LUND application -> completed files
run.csh --workflow submit      -> setup_and_submit.csh -> submit.py -> sbatch array -> GEMC -> recon-util
```

On ifarm, use `source run.csh --workflow submit --lund-dir RUN/lundfiles`. The completed manifest supplies sample settings; optional CLI flags or `--config config/submission.conf` override simulation defaults. GEMC falls back to 5.14; submission checks the shared version directory, loads that module in its child environment, and verifies the exact `gemc` executable passed to Slurm. The default is preview; add `--execute` to submit and replace the selected simulation output directories while preserving LUND inputs. See the [setup and submission guide](docs/gemc-reconstruction-batch-submission.md).

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
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug

build/debug/apps/clas12-uniform \
  --config config/samples/uniform-1e-5986MeV.conf \
  --events 100 \
  --output runs/first-electron
```

The resolved uniform run is written below `runs/first-electron/Uniform_sample_1e_5986MeV`. If that directory already exists, generation prints a warning, removes its previous contents and recreates it. Building and generation do not run `git clean` or submit jobs.

LUND output always preserves the established text conventions. Electron, proton, neutron, and charged-pion masses come from the protected target source; photons are massless. Production momentum defaults are mixed p/1-p for the 1e electron and charged hadrons, and uniform p for neutrons. Sampled hadron momentum always extends to the beam energy; fixed 1 GeV/c momentum is a neutron-only option. Select electron–hadron samples with `--channel eh --hadron proton|neutron|pip|pim --hadron-region FD|CD`.

Open `runs/first-electron/Uniform_sample_1e_5986MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json` to see the resolved settings and output counts. LUND text is under `lundfiles/`; uniform diagnostics are stored once in `lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root`. Physical conversion does not create monitoring histograms.

## Where to start

Read the [newcomer guide](docs/index.md), then [build instructions](docs/building.md) and the [architecture walkthrough](docs/architecture.md).

Maintained source is grouped first by workflow under `src/lund-generation/` and `src/slurm-submission/`; the shared dispatcher is under `src/launcher/`. The LUND workflow then separates configuration, geometry, serialization, uniform generation, and physical adapters by responsibility. Protected imported files live with the workflow that consumes them. The architecture walkthrough maps these directories to the build targets and runtime call chain.

- [Uniform generation](docs/uniform-samples.md)
- [Physical event-generator conversion](docs/genie-to-lund-conversion.md)
- [GEMC, reconstruction and Slurm](docs/gemc-reconstruction-batch-submission.md)
- [Workflow command examples](tutorials/README.md)
- [Configuration reference](docs/configuration.md)
- [Technical note and complete reference](docs/technical-note.md)
- [Legacy parity and validation](docs/validation.md)
- [Migration from the imported repositories](docs/migration.md)
- [Public GitHub Wiki publishing](docs/wiki.md)

The original source trees are retained in `legacy/` for comparison. `legacy/Uniform-sample-generator` is pinned as a submodule to its independent upstream repository; its selected kernels are compiled only by parity tests. The legacy sources are retired from production use. Detector cards and reconstruction YAML are retained in `config/detector/`.

For local editing and server execution via `source run.csh`, read the [SSH workflow](docs/ssh-workflow.md). When sourcing from outside the checkout, the user may set the optional `CLAS12_SAMPLES_DIR` environment variable to its absolute path; the project does not define it automatically. Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](docs/external-inputs.md).
