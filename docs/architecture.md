# Architecture and code walkthrough

## Build targets

| Target | Source | Responsibility |
| --- | --- | --- |
| `SampleCommon` | `src/common/` | Configuration, event representation, vertices, LUND output, monitoring |
| `UniformGeneration` | `src/uniform/` | Uniform sampling prescriptions |
| `GenieConversion` | `src/genie/` | GENIE GST input adapter |
| `PhysicalConversion` | `src/physical/` | Select the configured physical event-generator adapter |
| `clas12-uniform` | `apps/uniform_main.cpp` | Parse CLI, call generator, report errors |
| `clas12-generator-to-lund` | `apps/genie_to_lund_main.cpp` | Parse physical input settings and dispatch an adapter |

The root CMake file discovers ROOT and adds subdirectories. `src/CMakeLists.txt` declares reusable libraries and target-scoped dependencies. The test targets alone compile archived reference code; production libraries do not include archived implementations. `apps/CMakeLists.txt` links entry points. Every implementation is compiled once; implementation files are never included from another implementation. ROOT macros and archived analysis helpers are excluded from production targets.

## Workflow dispatcher

`run.csh` performs the checked disposable-server refresh and environment setup, then calls `scripts/workflow.py` with the original argument boundaries preserved. The Python dispatcher owns build/test staging and selects one child; it does not interpret sample physics or detector settings.

```text
scripts/workflow.py
    ├── --workflow create-lund --source uniform
    │       └── BUILD/apps/clas12-uniform
    ├── --workflow create-lund --source physical
    │       └── BUILD/apps/clas12-generator-to-lund
    └── --workflow submit
            └── scripts/slurm/submit.py
```

The dispatcher calls `parse_known_args()`: its own options become launcher settings, while unknown tokens become the selected child's argument vector. One optional bare `--` separator is removed. The remaining tokens are appended unchanged and executed as an argv list from the repository root, without shell evaluation. Thus `--config`, `--input`, and `--output` reach a LUND executable, while `--manifest`, `--gcard`, `--reconstruction`, and `--site` reach the submitter.

Launcher settings have three precedence levels: built-in fallbacks, the strict JSON selected by `--run-settings` (default `config/run.json`), and explicit launcher options. The JSON may contain only `build`, `run`, `test`, `build_dir`, `build_type`, and `jobs`. Workflow and source are required command selections. There is no automatic `config/run.local.json`; an alternative profile must be named explicitly because ifarm synchronization normally removes untracked files.

When enabled, the stages run in dependency order: configure/build both LUND applications, run CTest, then dispatch the selected child. A failed checked stage prevents every later stage. `--run false` gives a build/test-only invocation. LUND creation never submits jobs automatically.

The configuration files remain separate because they have different owners and lifetimes:

| Input | Consumer | Responsibility |
| --- | --- | --- |
| `config/run.json` or `--run-settings FILE` | `workflow.py` | Stable build, test, and stage defaults |
| `config/samples/*.conf` | Selected C++ application | Sample physics, target, event count, naming, and generator provenance |
| Completed `manifest.json` | Submission and simulation coordinators | Exact completed LUND files, counts, resolved configuration, and provenance |
| `config/sites/*.json` | Submission and simulation coordinators | Worker-visible programs and Slurm resources |
| Explicit GCARD and YAML | GEMC and reconstruction payload | Detector and reconstruction configuration |

This separation keeps the selected action visible in the command and prevents scheduler or build settings from changing the scientific definition of a sample.

## Following a uniform run

1. The application calls `RunConfig::parse`. Built-in defaults are merged with a `key = value` file and then command-line overrides. Unknown, repeated and invalid settings fail before opening an output directory.
2. `generateUniform` resolves the channel and owns separate kinematic and vertex random streams. `TargetGeometry` samples a common interaction vertex for all particles in the event.
3. `Event` holds metadata and `Particle` values. Generation logic operates on these values, not on text formatting or shell commands.
4. `LundWriter` creates a new run directory, splits events into numbered files, and serializes all channels in the same format.
5. `Monitoring` owns detached ROOT histograms. It fills per-PDG momentum, angles, vertices and angular/momentum correlations.
6. `LegacyMonitoring` writes the original channel histogram names and binning to a separate ROOT file, with optional rendered plots.
7. After output and monitoring finish successfully, `LundWriter::finish` atomically renames the completed manifest into place.

## Following a physical run

`convertPhysical` selects the `event-generator` adapter; GENIE is the implemented default. `convertGenie` loads a `TChain("gst")` and validates required branches. Typed `TTreeReaderValue` and dynamically sized `TTreeReaderArray` objects avoid the imported fixed arrays of 250 particles. The adapter assigns the legacy process code, filters supported PDG codes, creates an `Event`, and calls the same writer and monitoring code.

The converter stops at the configured output capacity or end of input. The final partial file is retained. No empty rollover file is opened. Errors reading later chain entries prevent publication of a completed manifest.

## Simulation boundary

`scripts/simulation/run.py` consumes `manifest.json` and explicit detector/site settings. It delegates detector execution to the protected Bash payload `src/common/external/submit_GEMC_sample.sh`, adapted from the two legacy job scripts. Python validates the manifest and owns locks/provenance; the payload owns sample monitoring and the GEMC/reconstruction sequence. Dry runs print the commands without creating simulation directories. Execution checks return codes and output files and writes one record per completed file.

`scripts/slurm/submit.py` uses the same planning validation and submits an array with one task per manifest file. Each task invokes the runner with its one-based index. CMake never submits jobs.

## Adding functionality

- Add a sampling prescription in `src/uniform/` with validated settings and an output-level test of its distribution or invariants.
- Add another physical adapter behind `convertPhysical`; keep the public executable and manifest contract unchanged.
- Replace or extend `src/common/external/targets.h`, the external geometry source, and test its vertex bounds; see [external inputs](external-inputs.md). Geometry and nuclear A/Z are separate choices.
- Add detector cards under `config/detector/` and select them explicitly at execution time.
- Keep machine paths, scheduler resources and binary names in site configuration.

Do not infer physics configuration from filenames or output paths. Keep the external header's global RNG isolated inside the geometry adapter; do not add application-global RNGs or duplicate LUND formatting in individual workflows.

The complete [source/API inventory](code-reference.md) also covers tests, examples, error paths, and archived supporting utilities.
