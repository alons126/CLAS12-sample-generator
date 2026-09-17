# Source and API reference

This chapter inventories the supported code and the archived support code so a future technical note can distinguish the implemented methods from historical dependencies.

## 1. Build and application layer

| File | Responsibilities and interfaces |
| --- | --- |
| `CMakeLists.txt` | Defines project/version and BUILD_UNIFORM/BUILD_GENIE; discovers ROOT; matches ROOT's C++ standard; configures revision header; adds libraries/apps/tests and installation |
| `CMakePresets.json` | Debug and Release configure/build presets; Debug CTest preset |
| `src/CMakeLists.txt` | Static `LundCore`, `UniformGeneration`, `GenieConversion`, and `PhysicalConversion` targets |
| `apps/CMakeLists.txt` | Defines and installs the two application targets |
| `apps/uniform_main.cpp` | Handles `--help`, parses uniform settings, invokes generation; returns 1 on caught exceptions |
| `apps/genie_to_lund_main.cpp` | Generator-independent physical entry point and error reporting |
| `.vscode/c_cpp_properties.json` | Uses Debug compile_commands.json for editor compiler/include settings |

Production sources compile once into conventional targets. References to archived implementation files appear only in test adapters. Test executables are not installed.

## 2. Shared maintained layers

The shared pipeline is organized by responsibility instead of a catch-all `common` directory. These directories compile into the single `LundCore` target because they are small and always used together. `src/common/external/` retains only protected imported files and is outside the maintained layers described below.

### Configuration (`src/config/`)

[RunConfig.h](../src/config/RunConfig.h) declares the shared, read-only handoff from each C++ command-line entry point to its generator/converter and writer. [RunConfig.cpp](../src/config/RunConfig.cpp) implements the accepted common and source-specific key sets, strict `--key value` and `key = value` parsing, precedence, RG-M target lookup, automatic sampling/provenance settings, output naming, path normalization, and validation. Typed readers expose checked values and `values` supplies the exact resolved strings for manifest provenance.

Configuration is parsed once; `UniformConfig` converts frequently used settings to typed values before the event loop. `RunConfig` does not inspect event data, own RNGs, mutate output directories, serialize LUND, monitor events, or submit simulation. Unknown/duplicate keys and invalid ranges fail before those operations can begin. Config syntax and defaults are in [configuration](configuration.md).

### LUND records and serialization (`src/lund/`)

[Event.h](../src/lund/Event.h) declares `Particle`, `Event`, and `particleMass(pid,legacy)`. [Particle.cpp](../src/lund/Particle.cpp) reads the centralized support tables and rejects unsupported species.

[LundWriter.h](../src/lund/LundWriter.h) / [LundWriter.cpp](../src/lund/LundWriter.cpp): constructor validates and recreates the resolved run directory; `full` checks event capacity; `write` serializes an event and rotates files at the resolved `events-per-file` threshold; `finish(scanned)` publishes the manifest. Uniform construction also prepares the archived downstream/output directories. Output-stream exceptions propagate; failed runs may leave partial output without a manifest.

### Geometry (`src/geometry/`)

[RgmTarget.h](../src/config/RgmTarget.h) / [RgmTarget.cpp](../src/config/RgmTarget.cpp) map RG-M material/assembly identifiers to A/Z, protected geometry keys, and GEMC variations. [TargetGeometry.h](../src/geometry/TargetGeometry.h) / [TargetGeometry.cpp](../src/geometry/TargetGeometry.cpp) validate and sample the external geometry under an isolated RNG lock. Fixed tester coordinates bypass target sampling. The adapter includes the protected `src/common/external/targets.h`; that imported header remains in place so replacing it does not mix external ownership with maintained geometry code.

### Monitoring (`src/monitoring/`)

[Monitoring.h](../src/monitoring/Monitoring.h) / [Monitoring.cpp](../src/monitoring/Monitoring.cpp): an owned implementation allocates per-PDG histograms on first use. `fill` records all written particles and `save` writes `monitoring.root`. Histograms are detached from the ROOT directory during generation to avoid global ownership conflicts.

[LegacyMonitoring.h](../src/monitoring/LegacyMonitoring.h) / [LegacyMonitoring.cpp](../src/monitoring/LegacyMonitoring.cpp): owns the original uniform histogram definitions as run-local objects. Constructor selects `1e`, `ep`, `en`, or `Tester_e`. Each entry maps histogram x/y quantities to particle values. `fill` includes original inter-particle correlations; `save` writes numerical histograms and optionally renders a caller-selected legacy PDF/numbered-PNG layout. The definitions are migrated source, not runtime imports from `legacy/`.

### Support (`src/support/`)

[constants.h](../src/support/constants.h) is the single maintained catalog of supported PDG identifiers, current PDG 2026 masses, and explicitly separated archived compatibility masses consumed by the LUND layer and generators. [environment.h](../src/support/environment.h) is the only maintained C++ source of ANSI color definitions. It exposes immutable semantic colors for errors, completion, system messages, information, warnings, and reset. Application entry points, workflow summaries, replacement warnings, and completion messages reference those names instead of defining escape sequences locally. Shell and Python launchers retain their separate environment-variable palette because they cannot include a C++ header.

[Version.h.in](../src/support/Version.h.in) embeds project version, target-header SHA-256, and the configure-time Git revision into the generated `Version.h` used by the manifest. This is build provenance, not a runtime Git dependency.

## 3. Uniform code

[UniformConfig.h](../src/uniform/UniformConfig.h) defines the `UniformChannel` and `HadronSpecies` enums and the typed configuration used by the hot loop. It includes angular/momentum bounds, resolved mode booleans, trigger parameters and A/Z.

[UniformGenerator.h](../src/uniform/UniformGenerator.h) / [UniformGenerator.cpp](../src/uniform/UniformGenerator.cpp) expose `generateUniform(const RunConfig&)`. The function owns RNGs, geometry, both monitoring sets and a writer. Internal `momentum` constructs Cartesian vectors; `triggerPhi` retains the archived sector/tie convention. Each loop iteration samples a vertex and the configured particles, writes the event, then fills diagnostics. After completion it saves both ROOT products, optional plots, and the manifest.

The 1e electron and charged-hadron branches alternate uniform-p and uniform-1/p components using the run-global index. Neutrons use uniform momentum unless their optional fixed mode is selected. Hadron species and FD/CD region resolve the documented angular and threshold defaults. Mathematical definitions are in [sampling models](sampling-models.md).

## 4. Physical input code

[PhysicalConverter.h](../src/physical/PhysicalConverter.h) / [PhysicalConverter.cpp](../src/physical/PhysicalConverter.cpp) provide the stable physical-source dispatch. `event-generator=genie` selects the current adapter; future adapters join here without changing the public executable.

[GenieConverter.h](../src/genie/GenieConverter.h) / [GenieConverter.cpp](../src/genie/GenieConverter.cpp) expose `convertGenie(const RunConfig&)`. A `TChain("gst")` feeds typed `TTreeReaderValue`/`TTreeReaderArray` objects. The function checks branches/types/array lengths, fills the original pre-selection electron diagnostic, selects process/species, and writes an `Event`. It also fills the common written-particle diagnostics.

The reader arrays replace the archived fixed 250-element buffers. Input errors, unsupported-only input and output failures do not publish a manifest. Complete splitting retains partial files; the capacity limit counts accepted events, not scanned entries. Schema and process conventions are in the [GENIE guide](genie-to-lund-conversion.md).

## 5. Execution scripts

| File/function | Contract |
| --- | --- |
| `scripts/workflow.py: parser` | Defines launcher-owned workflow/source and build/test options; child options remain unknown for forwarding |
| `workflow.py: settings` | Merges built-ins, one explicit/default strict run JSON, and CLI overrides; validates the required workflow/source and build controls |
| `workflow.py: execute` | Prints a safely quoted representation, then runs the original argv list from the repository root with checked failure propagation |
| `workflow.py: main` | Optionally configures/builds both LUND applications, optionally runs CTest, and dispatches uniform, physical, or submission child commands |
| `scripts/simulation/run.py: parser` | CLI for manifest, detector files, site, field scales, file index, naming and execution |
| `run.py: load_plan` | Validates manifest schema/counts/paths, finite field scales, existing configuration files, output conflicts, site executable names; prepares a preview of the fixed legacy commands and validates compatible inputs |
| `run.py: main` | Prints a dry run, or acquires a per-file exclusive lock, invokes submit_GEMC_sample.sh and records command/config/payload hashes |
| `scripts/slurm/submit.py: main` | Reuses runner validation, validates site scheduler fields, constructs a one-task-per-file Slurm array and quotes the worker invocation; submits only with --execute |

`workflow.py` maps `create-lund/uniform` to `clas12-uniform`, `create-lund/physical` to `clas12-generator-to-lund`, and `submit` to `scripts/slurm/submit.py`. It forwards child options unchanged after removing one optional bare `--`; it does not choose a sample profile, input, output, site, GCARD, or YAML. Its run JSON contains only build/test stage controls, and no `run.local.json` is loaded implicitly.

The runner calls the external `src/common/external/submit_GEMC_sample.sh` Bash payload with resolved environment values. The payload retains the original GEMC/reconstruction command lines. Python prepares a preview and invokes the script for actual execution. Slurm's `--wrap` needs a shell command; fixed arguments are shell-quoted and only the task-index environment variable is expanded by the worker. Scripts do not send SSH commands, clean repositories or source environment modules. On failure, lock/partial outputs remain for inspection; successful file records are stored by manifest index.

## 6. Configuration and resources

- `config/samples/uniform-{electron,proton,neutron}.conf`: production sampling examples with explicit Ar metadata.
- `uniform-{proton,neutron}-sampled.conf`: explicit aliases for the non-fixed production modes with legacy angular windows.
- `electron-tester.conf`: fixed beam momentum and fixed `(0,0,-3 cm)` vertex.
- `genie.conf`: an explicit Ar conversion example.
- `legacy-coderun.conf`, `legacy-genie-wrapper.conf`: active archived launch settings; override their production-sized counts for smoke tests.
- `config/sites/local.json`: executable names for local processing.
- `config/sites/jlab.json`: executable names and the historical scheduler/log conventions; the caller must load the correct software environment.
- `config/detector/Generation_files_*`: unchanged 2/4/6 GeV cards and reconstruction YAML for the archived versions. They are resources, not generated models. Matching detector/data dependencies are external.

## 7. Test code

| File | Role |
| --- | --- |
| `tests/CMakeLists.txt` | Registers integration, parity, distribution and command tests |
| `integration.py` | LUND invariants, channel behavior, config validation, deterministic output, conversion splitting/schema errors |
| `make_gst_fixture.cpp` | Generates normal, long parity, short, missing/wrong-type, empty, unsupported and >250-particle GST fixtures |
| `simulation.py` | Dry runs, file selection, exact counts, installed-style execution stubs and failure behavior |
| `legacy_uniform_driver.cpp` | Calls archived uniform/tester kernels and archived histogram initialization with controlled seeds and temporary files |
| `prepare_legacy_genie.py` | Builds a redirected reference converter without changing its event loop |
| `legacy_parity.py` | Compares reference/current LUND bytes and invokes histogram comparison; demonstrates the short-input correction |
| `compare_histograms.cpp` | Compares ROOT histogram names/counts, axes, entries, contents/errors including flow bins |
| `distributions.py` | Compares new neutron/proton draws to analytic CDFs |
| `submission_parity.py` | Runs archived payloads with fake binaries and compares GEMC/reconstruction argv |

Exact test scope and acceptance criteria are in [validation](validation.md).

## 8. Archived supporting code

`legacy/Uniform-sample-generator/` is a Git submodule pinned to the independent `alons126/Uniform-sample-generator` repository. It retains the configuration/path helpers, text printing, particle formatter, angle calculation, target globals, histogram globals, main/ROOT launchers, tester and upstream historical material. Only selected event/diagnostic kernels are compiled into maintained test references; production targets do not link the submodule.

`legacy/GEMC-samples/` retains the converter, geometry helper, shell setup/submission chains and resource snapshots. Its `framework/classes/AMaps` implements historical acceptance-map lookup, `hPlots` implements plotting containers, and `DSCuts` stores cut parameters. `framework/namespaces/general_utilities` holds environment/text/ROOT helpers and the restored converter mass constants. The acceptance-map/fiducial application is commented out in the archived converter; these classes are not active new generation dependencies. They are not a supported replacement for downstream acceptance analysis.

The archived root `genie_job_submission_script.csh` is another historical submission copy. There is one supported new Slurm runner. Do not infer which historical copy was last used from its location alone.

## 9. SSH checkout orchestration

[SSH workflow](ssh-workflow.md) documents the disposable ifarm checkout refresh and `config/run.json`. `scripts/workflow.py` reads build/test defaults, requires an explicit workflow and LUND source, builds/tests, and dispatches either LUND creation or Slurm submission. Sample profiles and child arguments are explicit; the simulation runner is an internal array worker. Subprocess arguments are passed as lists and sourced wrappers preserve failure status.

`tests/launcher.py` exercises sourced/direct invocation, paths with spaces, failures, configuration/build calls and Git update safety using an isolated local repository. `tests/prepare_replacement_geometry.py` creates a changed target header; `tests/replacement_geometry.cpp` checks the actual adapter against that replacement, including new target discovery and RNG independence.

See [source documentation conventions](source-documentation.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded and protected from edits.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
