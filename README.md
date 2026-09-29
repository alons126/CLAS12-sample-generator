# CLAS12 sample generator

Current user-facing workflows:

1. **Create LUND files:** uniform acceptance samples or conversion of physical GENIE GST truth.
2. **Submit simulation on ifarm:** consume completed LUND files, configure GEMC/detector inputs, and submit GEMC followed by reconstruction as Slurm array jobs.

```text
run.csh --workflow create-lund -> Python build driver -> LUND application -> completed files
run.csh --workflow submit      -> setup_and_submit.csh -> submit.py -> sbatch array -> GEMC -> recon-util
```

On ifarm, `~/.cshrc` must source `~/environment.csh` to load the CLAS12 environment and select COATJAVA 10.0.7; see the [ifarm environment guide](docs/submit-simulation/ifarm-environment.md) for the exact file. Then use `source run.csh --workflow submit --lund-dir RUN/lundfiles`. The completed manifest supplies sample settings; optional CLI flags or `--config config/submission.conf` override simulation defaults. GEMC falls back to 5.14 because it includes the new RG-M Ar target and corrected one-foil C12 target implementations[^sportes-2026-rgm]. Submission checks the shared version directory, loads that module in its child environment, and verifies the exact `gemc` executable passed to Slurm. GEMC 6.x with COATJAVA 11 requires further analysis validation before it can replace the current defaults. The default is preview; add `--execute` to submit and replace the selected simulation output directories while preserving LUND inputs. See the [setup and submission guide](docs/submit-simulation/guide.md).

This repository does not run the physical event generator or calculate final acceptance maps.

## First build and sample

Requirements: CMake 3.20+, a C++ compiler compatible with your ROOT installation, ROOT with Core/RIO/Hist/Physics/Tree/TreePlayer, Python 3.9+ for the workflow drivers, and csh/tcsh for the sourced ifarm launcher. Submission preview and execution require the ifarm module command, the requested shared GEMC version, and the COATJAVA 10.0.7 `recon-util` supplied by the documented login environment; only execution requires `sbatch`.

Clone the repository:

```bash
git clone REPOSITORY_URL
```

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 4

build/debug/apps/uniform-lund-creator \
  --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
  --events 100 \
  --output runs/first-electron
```

The resolved uniform run is written below `runs/first-electron/Uniform__1e__5986MeV`. If that directory already exists, generation prints a warning, removes its previous contents and recreates it. Building and generation do not run `git clean` or submit jobs.

LUND output uses one space between fields and writes particle momentum, energy, mass, and vertex values with five digits after the decimal point. Electron, proton, neutron, and charged-pion masses come from the external target source; photons are massless. Production momentum defaults are mixed p/1-p for the 1e electron and charged hadrons, and uniform p for neutrons. Sampled hadron momentum always extends to the beam energy; fixed 1 GeV/c momentum is a neutron-only option. Select electron–hadron samples with `--channel eh --hadron proton|neutron|pip|pim --hadron-region FD|CD`.

Open `runs/first-electron/Uniform__1e__5986MeV/lundfiles/lund-creation-monitoring/lund-creation-log.json` to see the resolved settings, output counts, and full configure-time Git information. LUND text is under `lundfiles/`; uniform diagnostics are stored once in `lundfiles/lund-creation-monitoring/<prefix>__monitoring_plots.root`. Physical conversion does not create monitoring histograms.

## Documentation

Start at the [documentation home](docs/index.md). It presents the currently implemented workflows first and routes readers by task instead of exposing the complete reference at once.

| Subject | Use it for |
| --- | --- |
| [Getting started](docs/getting-started/index.md) | Install, build, run a small sample, and understand outputs |
| [Create LUND files](docs/create-lund/index.md) | Uniform generation, physical conversion, configuration, examples, and monitoring |
| [Submit simulation](docs/submit-simulation/index.md) | Preview and submit ifarm GEMC/reconstruction jobs |
| [Concepts and contracts](docs/concepts/index.md) | Architecture, sampling, LUND records, provenance, and scientific scope |
| [Development](docs/development/index.md) | Source reference, documentation, wiki publishing, and [adding an event-generator adapter](docs/development/adding-event-generator.md) |

Worked commands are grouped by workflow in the [LUND-creation examples](docs/create-lund/examples.md) and [submission examples](docs/submit-simulation/examples.md). The longer [checked-in command lists](tutorials/README.md) remain available for the established production matrix.

Workflow implementations are peers under `src/workflows/`; shared C++ workflow support is under `src/workflows/support/`, and the shared dispatcher is under `src/launcher/`. The [architecture walkthrough](docs/concepts/architecture.md) maps these directories to build targets and runtime call chains.

The project has two narrow update boundaries for code obtained from RG-M. `src/workflows/lund-creation/external/targets.h` is an exact RG-M copy containing the target implementations available with GEMC 5.14 when it was added[^sportes-2026-rgm]. `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` adapts the RG-M job payload to the project’s generator-independent settings. Small adapters around these files allow later RG-M updates without copying target geometry or detector commands; see [external inputs](docs/concepts/external-inputs.md).

For local editing and server execution via `source run.csh`, read the [SSH workflow](docs/submit-simulation/ifarm-environment.md). When sourcing from outside the checkout, the user may set the optional `CLAS12_SAMPLES_DIR` environment variable to its absolute path; the project does not define it automatically. Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](docs/concepts/external-inputs.md).

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
