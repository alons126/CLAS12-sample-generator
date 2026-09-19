# Architecture and code walkthrough

## Source layout

Maintained code is grouped first by the two user-facing workflows. `src/lund-generation/` owns LUND creation, while `src/slurm-submission/` owns ifarm job submission. `src/launcher/` contains the shared entry-point machinery that selects either workflow. Inside LUND generation, responsibility folders remain ownership boundaries while `LundCore` compiles the small shared layers together.

| Directory | Responsibility |
| --- | --- |
| `src/lund-generation/` | Both uniform and physical LUND creation, their entry points, external geometry, and tests |
| `src/slurm-submission/` | Simulation runner, Slurm array submitter, protected GEMC payload, and submission tests |
| `src/launcher/` | Shared Python dispatcher and sourced-shell support used by `run.csh` |
| `src/lund-generation/core/config/` | Parse and validate run settings; resolve RG-M target identity and metadata |
| `src/lund-generation/core/lund/` | Represent events and particles; split files, serialize LUND, and publish the manifest |
| `src/lund-generation/core/geometry/` | Adapt the protected target definitions to one sampled interaction vertex per event |
| `src/lund-generation/core/support/` | Central PDG constants, terminal presentation, and the generated-version template |
| `src/lund-generation/clas12-uniform/` | Produce deliberately unphysical acceptance-map events and their monitoring |
| `src/lund-generation/clas12-generator-to-lund/` | Dispatch a physical input source to its event-generator adapter |
| `src/lund-generation/clas12-generator-to-lund/genie/` | Read GENIE GST as the currently implemented physical adapter |
| `src/lund-generation/external/` | Protected imported target geometry |
| `src/slurm-submission/external/` | Protected GEMC/reconstruction worker payload |

The two source-specific directories intentionally match the installed executable names. The GENIE reader is nested under `clas12-generator-to-lund` because it implements one physical-input adapter rather than an independent workflow. A future adapter belongs beside it, such as `src/lund-generation/clas12-generator-to-lund/gibuu/`. Cross-layer includes state dependencies directly, for example `core/config/RunConfig.h`, `core/lund/Event.h`, and `core/support/constants.h`.

## Build targets

| Target | Source | Responsibility |
| --- | --- | --- |
| `LundCore` | `src/lund-generation/core/config/`, `src/lund-generation/core/lund/`, `src/lund-generation/core/geometry/`, `src/lund-generation/core/support/` | Shared configuration-to-manifest LUND pipeline |
| `UniformGeneration` | `src/lund-generation/clas12-uniform/` | Uniform sampling prescriptions and uniform-only monitoring |
| `GenieConversion` | `src/lund-generation/clas12-generator-to-lund/genie/` | GENIE GST input adapter |
| `PhysicalConversion` | `src/lund-generation/clas12-generator-to-lund/` | Select the configured physical event-generator adapter |
| `clas12-uniform` | `src/lund-generation/apps/uniform_main.cpp` | Parse CLI, call generator, report errors |
| `clas12-generator-to-lund` | `src/lund-generation/apps/genie_to_lund_main.cpp` | Parse physical input settings and dispatch an adapter |

The root CMake file discovers ROOT and adds subdirectories. `src/CMakeLists.txt` declares reusable libraries and target-scoped dependencies. The test targets alone compile archived reference code; production libraries do not include archived implementations. `src/lund-generation/apps/CMakeLists.txt` links entry points. Every implementation is compiled once; implementation files are never included from another implementation. ROOT macros and archived analysis helpers are excluded from production targets.

## Workflow dispatcher

`run.csh` performs the checked disposable-server refresh and environment setup, then calls `src/launcher/workflow.py` with the original argument boundaries preserved. The Python dispatcher owns build/test staging and selects one child; it does not interpret sample physics or detector settings.

```text
src/launcher/workflow.py
    ├── --workflow create-lund --source uniform
    │       └── BUILD/apps/clas12-uniform
    ├── --workflow create-lund --source physical
    │       └── BUILD/apps/clas12-generator-to-lund
    └── --workflow submit
            └── src/slurm-submission/submit.py
```

The dispatcher calls `parse_known_args()`: its own options become launcher settings, while unknown tokens become the selected child's argument vector. One optional bare `--` separator is removed. The remaining tokens are appended unchanged and executed as an argv list from the repository root, without shell evaluation. Thus `--config`, `--input`, and `--output` reach a LUND executable, while `--manifest`, `--gcard`, `--reconstruction`, and `--site` reach the submitter.

Launcher settings have three precedence levels: built-in fallbacks, the strict JSON selected by `--run-settings` (default `config/run.json`), and explicit launcher options. The JSON may contain only `build`, `run`, `test`, `build_dir`, `build_type`, and `jobs`. Workflow and source are required command selections. There is no automatic `config/run.local.json`; an alternative profile must be named explicitly because ifarm synchronization normally removes untracked files.

When enabled, the stages run in dependency order: configure/build both LUND applications, run CTest, then dispatch the selected child. A failed checked stage prevents every later stage. `--run false` gives a build/test-only invocation. LUND creation never submits jobs automatically.

The configuration files remain separate because they have different owners and lifetimes:

| Input | Consumer | Responsibility |
| --- | --- | --- |
| `config/run.json` or `--run-settings FILE` | `workflow.py` | Stable build, test, and stage defaults |
| `config/samples/*.conf` | Selected C++ application | Sample physics, target, event count, naming, and generator provenance |
| Completed `lundfiles/lund-gen-monitoring/lund-gen-log.json` | Submission and simulation coordinators | Exact completed LUND files, counts, resolved configuration, and provenance |
| `config/sites/*.json` | Submission and simulation coordinators | Worker-visible programs and Slurm resources |
| Explicit GCARD and YAML | GEMC and reconstruction payload | Detector and reconstruction configuration |

This separation keeps the selected action visible in the command and prevents scheduler or build settings from changing the scientific definition of a sample.

## Sample configuration boundary

`RunConfig.h` declares the read-only configuration object shared by uniform generation, physical conversion, and `LundWriter`; `RunConfig.cpp` implements its construction and validation. Each LUND executable calls `RunConfig::parse(argc, argv, uniform)` exactly once. The boolean selects the accepted source-specific vocabulary: uniform sampling keys when true, or physical-input provenance keys when false.

Resolution follows one fixed sequence:

```text
shared + source-specific built-in defaults
    -> optional key = value sample profile
    -> command-line overrides
    -> target/channel/beam-dependent auto values
    -> shared and source-specific validation
    -> normalized input path and final absolute run-directory path
```

The object retains values as strings so the spelling actually used by the run can be written to `lundfiles/lund-gen-monitoring/lund-gen-log.json`. Consumers use `get`, `number`, and `integer` for checked access; the writer uses `values` to serialize the full resolved configuration. Target identity may supply automatic geometry, A/Z, and GEMC variation values, but explicit overrides remain independent. Source-specific options are rejected in the wrong mode rather than accepted and ignored.

This boundary is intentionally side-effect-free with respect to run products: parsing may read the selected profile, but it does not inspect GST event contents, sample kinematics or vertices, create or replace the run directory, write LUND or monitoring files, or submit simulation. Those responsibilities begin only after parsing succeeds and remain with the uniform generator, physical adapter, writer, and submission workflow respectively.

## Following a uniform run

1. The application calls `RunConfig::parse`. Built-in defaults are merged with a `key = value` file and then command-line overrides. Unknown, repeated and invalid settings fail before opening an output directory.
2. `generateUniform` receives the resolved `1e` or `eh` channel, selected hadron, and FD/CD region, then owns separate kinematic and vertex random streams. `TargetGeometry` samples a common interaction vertex for all particles in the event.
3. `Event` holds metadata and `Particle` values. Generation logic operates on these values, not on text formatting or shell commands.
4. `LundWriter` creates a new run directory, splits events into numbered files, and serializes all channels in the same format.
5. `UniformMonitoring` owns one ordered set of detached ROOT histograms. It preserves the archived definitions and rendering style and generalizes hadron labels to proton, neutron, pip, and pim in FD or CD.
6. It writes every histogram once to `<prefix>_monitoring_plots.root` and renders the same objects to PDF and PNG for every uniform channel.
7. After output and monitoring finish successfully, `LundWriter::finish` atomically renames the completed manifest into place.

## Following a physical run

`convertPhysical` selects the `event-generator` adapter; GENIE is the implemented default. `convertGenie` loads a `TChain("gst")` and validates required branches. Typed `TTreeReaderValue` and dynamically sized `TTreeReaderArray` objects avoid the imported fixed arrays of 250 particles. The adapter assigns the legacy process code, filters supported PDG codes, creates an `Event`, and calls the shared writer. Physical conversion creates no ROOT monitoring file or rendered monitoring plots.

The converter stops at the configured output capacity or end of input. The final partial file is retained. No empty rollover file is opened. Errors reading later chain entries prevent publication of a completed manifest.

## Simulation boundary

`src/slurm-submission/run.py` consumes `lundfiles/lund-gen-monitoring/lund-gen-log.json` and explicit detector/site settings. It delegates detector execution to the protected Bash payload `src/slurm-submission/external/submit_GEMC_sample.sh`, adapted from the two legacy job scripts. Python validates the manifest and owns locks/provenance; the payload owns sample monitoring and the GEMC/reconstruction sequence. Dry runs print the commands without creating simulation directories. Execution checks return codes and output files and writes one record per completed file.

`src/slurm-submission/submit.py` uses the same planning validation and submits an array with one task per manifest file. Each task invokes the runner with its one-based index. CMake never submits jobs.

## Adding functionality

- Add a sampling prescription in `src/lund-generation/clas12-uniform/` with validated settings and an output-level test of its distribution or invariants.
- Add another physical adapter under `src/lund-generation/clas12-generator-to-lund/<generator>/` and register it behind `convertPhysical`; keep the public executable and manifest contract unchanged.
- Replace or extend `src/lund-generation/external/targets.h`, the external geometry source, and test its vertex bounds; see [external inputs](external-inputs.md). Geometry and nuclear A/Z are separate choices.
- Add detector cards under `config/detector/` and select them explicitly at execution time.
- Keep machine paths, scheduler resources and binary names in site configuration.

Do not infer physics configuration from filenames or output paths. Keep the external header's global RNG isolated inside the geometry adapter; do not add application-global RNGs or duplicate LUND formatting in individual workflows.

The complete [source/API inventory](code-reference.md) also covers tests, examples, error paths, and archived supporting utilities.
