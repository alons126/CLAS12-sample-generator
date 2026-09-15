# Source and API reference

This chapter inventories the supported code and the archived support code so a future technical note can distinguish the implemented methods from historical dependencies.

## 1. Build and application layer

| File | Responsibilities and interfaces |
| --- | --- |
| `CMakeLists.txt` | Defines project/version and BUILD_UNIFORM/BUILD_GENIE; discovers ROOT; matches ROOT's C++ standard; configures revision header; adds libraries/apps/tests and installation |
| `CMakePresets.json` | Debug and Release configure/build presets; Debug CTest preset |
| `src/CMakeLists.txt` | Static `SampleCommon`, `UniformGeneration`, `GenieConversion` targets and their include/link dependencies |
| `apps/CMakeLists.txt` | Defines and installs the two application targets |
| `apps/uniform_main.cpp` | Handles `--help`, parses uniform settings, invokes generation; returns 1 on caught exceptions |
| `apps/genie_to_lund_main.cpp` | Corresponding GENIE entry point and error reporting |
| `.vscode/c_cpp_properties.json` | Uses Debug compile_commands.json for editor compiler/include settings |

Production sources compile once into conventional targets. References to archived implementation files appear only in test adapters. Test executables are not installed.

## 2. Common code

### RunConfig

[RunConfig.h](../src/common/RunConfig.h) / [RunConfig.cpp](../src/common/RunConfig.cpp): `parse(argc,argv,genie)` merges defaults, one optional config file, and CLI overrides; resolves automatic sampling/angle options and paths; then validates. `get`, `number`, `integer` return settings with strict conversion, while `values` exposes resolved settings for provenance. `validate` enforces counts, finite values, supported modes, geometry and bounds. `help` generates CLI usage; `jsonString` escapes manifest strings including control characters.

Configuration is parsed once; `UniformConfig` converts frequently used settings to typed values before the event loop. Unknown/duplicate keys and invalid ranges fail before output creation. Config syntax and defaults are in [configuration](configuration.md).

### Event and particle mass

[Event.h](../src/common/Event.h) declares `Particle`, `Event` and `particleMass(pid,legacy)`. Momentum and vertex values are owned, not shared mutable pointers. `particleMass` is implemented in [Particle.cpp](../src/common/Particle.cpp) and rejects unsupported species. Legacy/standard pion constants are listed in the [data contract](data-contracts.md).

### TargetGeometry

[TargetGeometry.h](../src/common/TargetGeometry.h) / [TargetGeometry.cpp](../src/common/TargetGeometry.cpp): the constructor validates the geometry name. `sample(TRandom3&)` consumes the caller's vertex stream and returns one vertex. `point` consumes no random draws. Continuous targets draw x/y Gaussian then z uniform; foils draw x/y Gaussian then an equal-probability foil index. The authoritative map and sampler live in the replaceable [targets.h](../src/common/external/targets.h). The adapter isolates its globals and transfers the caller RNG state under a mutex; see [external inputs](external-inputs.md).

### LundWriter

[LundWriter.h](../src/common/LundWriter.h) / [LundWriter.cpp](../src/common/LundWriter.cpp): constructor claims a new output directory; `full` checks the requested event capacity; `write` serializes an event and automatically rotates files at 10,000 events; `count` returns accepted output count; `finish(scanned)` closes files and publishes the manifest. Capacity and the file-splitting limit are cached outside the hot loop. Output-stream exceptions propagate to the application; no cleanup removes partial output.

`Version.h.in` embeds project version, target-header SHA-256 and the configure-time Git revision into the manifest. This is build provenance, not a runtime Git dependency.

### Monitoring

[Monitoring.h](../src/common/Monitoring.h) / [Monitoring.cpp](../src/common/Monitoring.cpp): an owned implementation allocates per-PDG histograms on first use. `fill` records all written particles and `save` writes `monitoring.root`. Histograms are detached from the ROOT directory during generation to avoid global ownership conflicts.

### LegacyMonitoring

[LegacyMonitoring.h](../src/common/LegacyMonitoring.h) / [LegacyMonitoring.cpp](../src/common/LegacyMonitoring.cpp): owns the original uniform histogram definitions as run-local objects. Constructor selects `1e`, `ep`, `en`, or `Tester_e`. Each entry maps histogram x/y quantities to particle values. `fill` includes original inter-particle correlations; `save(path,render)` writes numerical histograms and optionally renders PNG/PDF products. The definitions are migrated source, not runtime imports from `legacy/`.

## 3. Uniform code

[UniformConfig.h](../src/uniform/UniformConfig.h) defines the `UniformChannel` enum and the typed configuration used by the hot loop. It includes angular/momentum bounds, resolved mode booleans, trigger parameters and A/Z.

[UniformGenerator.h](../src/uniform/UniformGenerator.h) / [UniformGenerator.cpp](../src/uniform/UniformGenerator.cpp) expose `generateUniform(const RunConfig&)`. The function owns RNGs, geometry, both monitoring sets and a writer. Internal `momentum` constructs Cartesian vectors; `triggerPhi` retains the archived sector/tie convention. Each loop iteration samples a vertex and the configured particles, writes the event, then fills diagnostics. After completion it saves both ROOT products, optional plots, and the manifest.

The sampled ep branch alternates momentum components using the run-global index, independent of output-format numbering. Mathematical definitions are in [sampling models](sampling-models.md).

## 4. GENIE code

[GenieConverter.h](../src/genie/GenieConverter.h) / [GenieConverter.cpp](../src/genie/GenieConverter.cpp) expose `convertGenie(const RunConfig&)`. A `TChain("gst")` feeds typed `TTreeReaderValue`/`TTreeReaderArray` objects. The function checks branches/types/array lengths, fills the original pre-selection electron diagnostic, selects process/species, and writes an `Event`. It also fills the common written-particle diagnostics.

The reader arrays replace the archived fixed 250-element buffers. Input errors, unsupported-only input and output failures do not publish a manifest. Complete splitting retains partial files; the capacity limit counts accepted events, not scanned entries. Schema and process conventions are in the [GENIE guide](genie-to-lund-conversion.md).

## 5. Execution scripts

| File/function | Contract |
| --- | --- |
| `scripts/simulation/run.py: parser` | CLI for manifest, detector files, site, field scales, file index, naming and execution |
| `run.py: load_plan` | Validates manifest schema/counts/paths, finite field scales, existing configuration files, output conflicts, site executable names; prepares a preview of the fixed legacy commands and validates compatible inputs |
| `run.py: main` | Prints a dry run, or acquires a per-file exclusive lock, invokes submit_GEMC_sample.sh and records command/config/payload hashes |
| `scripts/slurm/submit.py: main` | Reuses runner validation, validates site scheduler fields, constructs a one-task-per-file Slurm array and quotes the worker invocation; submits only with --execute |

The runner calls the external `src/common/external/submit_GEMC_sample.sh` Bash payload with resolved environment values. The payload retains the original GEMC/reconstruction command lines. Python prepares a preview and invokes the script for actual execution. Slurm's `--wrap` needs a shell command; fixed arguments are shell-quoted and only the task-index environment variable is expanded by the worker. Scripts do not send SSH commands, clean repositories or source environment modules. On failure, lock/partial outputs remain for inspection; successful file records are stored by manifest index.

## 6. Configuration and resources

- `config/samples/uniform-{electron,proton,neutron}.conf`: small default/fixed examples with explicit Ar metadata.
- `uniform-{proton,neutron}-sampled.conf`: requested non-fixed modes with legacy angular windows.
- `electron-tester.conf`: fixed beam momentum, point vertex.
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

`legacy/Uniform-sample-generator/` retains the former configuration/path helpers, text printing, particle formatter, angle calculation, target globals, histogram globals, main/ROOT launchers and tester. These are historical comparison material; only the event/diagnostic kernels are imported into test references.

`legacy/GEMC-samples/` retains the converter, geometry helper, shell setup/submission chains and resource snapshots. Its `framework/classes/AMaps` implements historical acceptance-map lookup, `hPlots` implements plotting containers, and `DSCuts` stores cut parameters. `framework/namespaces/general_utilities` holds environment/text/ROOT helpers and the restored converter mass constants. The acceptance-map/fiducial application is commented out in the archived converter; these classes are not active new generation dependencies. They are not a supported replacement for downstream acceptance analysis.

The archived root `genie_job_submission_script.csh` is another historical submission copy. There is one supported new Slurm runner. Do not infer which historical copy was last used from its location alone.

## 9. SSH checkout orchestration

[SSH workflow](ssh-workflow.md) documents every shell wrapper, the banner helpers and `config/run.json`. `scripts/workflow.py` validates settings, merges CLI overrides, optionally performs a clean-checkout fast-forward pull, configures/builds/tests, and dispatches the selected generator, converter, simulation runner or submitter. Subprocess arguments are passed as lists. Shell wrappers preserve quoted arguments and return failures without exiting a sourced session.

`tests/launcher.py` exercises sourced/direct invocation, paths with spaces, failures, configuration/build calls and Git update safety using an isolated local repository. `tests/prepare_replacement_geometry.py` creates a changed target header; `tests/replacement_geometry.cpp` checks the actual adapter against that replacement, including new target discovery and RNG independence.

See [source documentation conventions](source-documentation.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded and protected from edits.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
