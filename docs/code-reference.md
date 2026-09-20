# Source and API reference

This chapter inventories the supported code and the archived support code so a future technical note can distinguish the implemented methods from historical dependencies.

## 1. Build and application layer

| File | Responsibilities and interfaces |
| --- | --- |
| `CMakeLists.txt` | Defines project/version and BUILD_UNIFORM/BUILD_GENIE; discovers ROOT; matches ROOT's C++ standard; configures revision header; adds libraries/apps/tests and installation |
| `CMakePresets.json` | Debug and Release configure/build presets; Debug CTest preset |
| `src/CMakeLists.txt` | Adds the LUND-generation, Slurm-submission, and shared-launcher source trees |
| `src/lund-generation/CMakeLists.txt` | Defines `LundCore`, `UniformGeneration`, `GenieConversion`, and `PhysicalConversion` |
| `src/slurm-submission/CMakeLists.txt` | Installs submission programs and registers submission tests |
| `src/launcher/CMakeLists.txt` | Registers the sourced-launcher integration test |
| `src/lund-generation/apps/CMakeLists.txt` | Defines and installs the two application targets |
| `src/lund-generation/apps/uniform_main.cpp` | Handles `--help`, parses uniform settings, invokes generation; returns 1 on caught exceptions |
| `src/lund-generation/apps/genie_to_lund_main.cpp` | Generator-independent physical entry point and error reporting |
| `.vscode/c_cpp_properties.json` | Uses Debug compile_commands.json for editor compiler/include settings |

Production sources compile once into conventional targets. References to archived implementation files appear only in test adapters. Test executables are not installed.

## 2. Shared maintained layers

The shared LUND pipeline is organized by responsibility inside `src/lund-generation/`. These directories compile into the single `LundCore` target because they are small and always used together. Protected target geometry is isolated under `src/lund-generation/external/`; the protected simulation payload belongs separately to `src/slurm-submission/external/`.

### Configuration (`src/lund-generation/core/config/`)

[RunConfig.h](../src/lund-generation/core/config/RunConfig.h) declares the shared, read-only handoff from each C++ command-line entry point to its generator/converter and writer. [RunConfig.cpp](../src/lund-generation/core/config/RunConfig.cpp) implements the accepted common and source-specific key sets, strict `--key value` and `key = value` parsing, precedence, RG-M target lookup, automatic sampling/provenance settings, output naming, path normalization, and validation. Typed readers expose checked values and `values` supplies the exact resolved strings for manifest provenance.

Configuration is parsed once; `UniformConfig` converts frequently used settings to typed values before the event loop. `RunConfig` does not inspect event data, own RNGs, mutate output directories, serialize LUND, monitor events, or submit simulation. Unknown/duplicate keys and invalid ranges fail before those operations can begin. Config syntax and defaults are in [configuration](configuration.md).

### LUND records and serialization (`src/lund-generation/core/lund/`)

[Event.h](../src/lund-generation/core/lund/Event.h) declares `Particle`, `Event`, and `particleMass(pid)`. [Particle.cpp](../src/lund-generation/core/lund/Particle.cpp) reads the centralized rounded mass table and rejects unsupported species.

[LundWriter.h](../src/lund-generation/core/lund/LundWriter.h) / [LundWriter.cpp](../src/lund-generation/core/lund/LundWriter.cpp): constructor validates and recreates the resolved run directory; `full` checks event capacity; `write` serializes an event and rotates files at the resolved `events-per-file` threshold; `finish(scanned)` publishes the manifest. Uniform construction also prepares the archived downstream/output directories. Output-stream exceptions propagate; failed runs may leave partial output without a manifest.

### Geometry (`src/lund-generation/core/geometry/`)

[RgmTarget.h](../src/lund-generation/core/config/RgmTarget.h) / [RgmTarget.cpp](../src/lund-generation/core/config/RgmTarget.cpp) map RG-M material/assembly identifiers to A/Z, protected geometry keys, and GEMC variations. [TargetGeometry.h](../src/lund-generation/core/geometry/TargetGeometry.h) / [TargetGeometry.cpp](../src/lund-generation/core/geometry/TargetGeometry.cpp) validate and sample the external geometry under an isolated RNG lock. Fixed tester coordinates bypass target sampling. The adapter includes the protected `src/lund-generation/external/targets.h`; that imported header remains in place so replacing it does not mix external ownership with maintained geometry code.

### Support (`src/lund-generation/core/support/`)

[constants.h](../src/lund-generation/core/support/constants.h) is the single maintained catalog of supported PDG identifiers, current PDG 2026 masses, and explicitly separated archived compatibility masses consumed by the LUND layer and generators. [environment.h](../src/lund-generation/core/support/environment.h) is the only maintained C++ source of ANSI color definitions. It exposes immutable semantic colors for errors, completion, system messages, information, warnings, and reset. Application entry points, workflow summaries, replacement warnings, and completion messages reference those names instead of defining escape sequences locally. Shell and Python launchers retain their separate environment-variable palette because they cannot include a C++ header.

[Version.h.in](../src/lund-generation/core/support/Version.h.in) embeds project version, target-header SHA-256, and the configure-time Git revision into the generated `Version.h` used by the manifest. This is build provenance, not a runtime Git dependency.

## 3. `clas12-uniform` implementation

[UniformConfig.h](../src/lund-generation/clas12-uniform/UniformConfig.h) defines the `UniformChannel` and `HadronSpecies` enums and the typed configuration used by the hot loop. It includes angular/momentum bounds, resolved mode booleans, trigger parameters and A/Z.

[UniformGenerator.h](../src/lund-generation/clas12-uniform/UniformGenerator.h) / [UniformGenerator.cpp](../src/lund-generation/clas12-uniform/UniformGenerator.cpp) expose `generateUniform(const RunConfig&)`. The function owns RNGs, geometry, one `UniformMonitoring` object and a writer. Internal `momentum` constructs Cartesian vectors; `triggerPhi` retains the archived sector/tie convention. Each loop iteration samples a vertex and the configured particles, writes the event, then fills diagnostics. After completion it saves one ROOT product, the required rendered plots, and the generation log.

[UniformMonitoring.h](../src/lund-generation/clas12-uniform/UniformMonitoring.h) / [UniformMonitoring.cpp](../src/lund-generation/clas12-uniform/UniformMonitoring.cpp) own the complete uniform-only monitoring contract. The implementation preserves legacy bins, axes, titles, correlations, axis text settings and canvas layout, then generalizes hadron tokens to `pFD`, `pCD`, `nFD`, `nCD`, `pipFD`, `pipCD`, `pimFD`, and `pimCD`. `save` writes all histograms once to `<prefix>_monitoring_plots.root` and always renders those same objects into `MonitoringPlotsPath`.

The 1e electron and charged-hadron branches alternate uniform-p and uniform-1/p components using the run-global index. Neutrons use uniform momentum unless their optional fixed mode is selected. Hadron species and FD/CD region resolve the documented angular and threshold defaults. Mathematical definitions are in [sampling models](sampling-models.md).

## 4. `clas12-generator-to-lund` implementation

[PhysicalConverter.h](../src/lund-generation/clas12-generator-to-lund/PhysicalConverter.h) / [PhysicalConverter.cpp](../src/lund-generation/clas12-generator-to-lund/PhysicalConverter.cpp) provide the stable physical-source dispatch. `event-generator=genie` selects the current adapter; future adapters join here without changing the public executable.

[GenieConverter.h](../src/lund-generation/clas12-generator-to-lund/genie/GenieConverter.h) / [GenieConverter.cpp](../src/lund-generation/clas12-generator-to-lund/genie/GenieConverter.cpp) are nested below the physical dispatcher because GENIE is one adapter of the `clas12-generator-to-lund` executable. They expose `convertGenie(const RunConfig&)`. A `TChain("gst")` feeds typed `TTreeReaderValue`/`TTreeReaderArray` objects. The function checks branches/types/array lengths, selects process/species, and writes an `Event`. Physical conversion creates no monitoring histograms.

The reader arrays replace the archived fixed 250-element buffers. Input errors, unsupported-only input and output failures do not publish a manifest. Complete splitting retains partial files; the capacity limit counts accepted events, not scanned entries. Schema and process conventions are in the [GENIE guide](genie-to-lund-conversion.md).

## 5. Execution scripts

| File | Contract |
| --- | --- |
| `run.csh` | Guarded ifarm refresh; source submission directly or dispatch LUND creation |
| `src/launcher/workflow.py` | LUND configuration, build/test stages and application dispatch |
| `src/slurm-submission/resolve_inputs.py` | Manifest/config/CLI precedence, truth validation, portable file inventory and safe shell assignments |
| `src/slurm-submission/setup_and_submit.csh` | Resolved input handoff, GEMC module, legacy report/checks, output reset and one array per sample |
| `src/slurm-submission/external/submit_GEMC_sample.sh` | Protected Slurm task payload; GEMC followed by reconstruction |

The resolver obtains the prefix and task count from the completed manifest or explicit input, then the setup script consumes the validated LUND files. It exports a shared event limit for the array, defaulting to the largest selected manifest file count. The payload retains its original scheduler defaults. See the [submission guide](gemc-reconstruction-batch-submission.md).

## 6. Configuration and resources

- `config/samples/uniform-<label>-{2070,4029,5986}MeV.conf`: complete Ar40 profiles for every supported 1e/FD/CD label at each established beam energy; pion and CD files are explicitly marked unvalidated for production.
- `electron-tester-{2070,4029,5986}MeV.conf`: beam-specific tester profiles with fixed beam momentum and target-sampled vertices.
- `genie.conf`: an explicit Ar conversion example.
- `legacy-coderun.conf`, `legacy-genie-wrapper.conf`: active archived launch settings; override their production-sized counts for smoke tests.
- `config/detector/Generation_files_*`: unchanged 2/4/6 GeV cards and reconstruction YAML for the archived versions. They are resources, not generated models. Matching detector/data dependencies are external.

## 7. Test code

| File | Role |
| --- | --- |
| `src/lund-generation/tests/CMakeLists.txt` | Registers LUND integration, parity, distribution, and geometry tests |
| `src/slurm-submission/tests/` | Legacy setup transcript, environment and failure tests |
| `src/launcher/tests/launcher.py` | Shared sourced/direct launcher, argument, build, and update-safety checks |
| `integration.py` | LUND invariants, channel behavior, config validation, deterministic output, conversion splitting/schema errors |
| `check_monitoring.cpp` | Checks representative FD/CD proton, neutron, pip and pim ROOT names, titles, axis labels, styles and histogram counts |
| `make_gst_fixture.cpp` | Generates normal, long parity, short, missing/wrong-type, empty, unsupported and >250-particle GST fixtures |
| `legacy_uniform_driver.cpp` | Calls archived uniform/tester kernels and archived histogram initialization with controlled seeds and temporary files |
| `prepare_legacy_genie.py` | Builds a redirected reference converter without changing its event loop |
| `legacy_parity.py` | Compares reference/current LUND bytes and invokes histogram comparison; demonstrates the short-input correction |
| `compare_histograms.cpp` | Compares ROOT histogram names/counts, axes, entries, contents/errors including flow bins |
| `distributions.py` | Compares new neutron/proton draws to analytic CDFs |
| `src/slurm-submission/tests/submission_parity.py` | Compares full archived setup stdout and Slurm environment using temporary fixtures |

Exact test scope and acceptance criteria are in [validation](validation.md).

## 8. Archived supporting code

`legacy/Uniform-sample-generator/` is a Git submodule pinned to the independent `alons126/Uniform-sample-generator` repository. It retains the configuration/path helpers, text printing, particle formatter, angle calculation, target globals, histogram globals, main/ROOT launchers, tester and upstream historical material. Only selected event/diagnostic kernels are compiled into maintained test references; production targets do not link the submodule.

`legacy/GEMC-samples/` retains the converter, geometry helper, shell setup/submission chains and resource snapshots. Its `framework/classes/AMaps` implements historical acceptance-map lookup, `hPlots` implements plotting containers, and `DSCuts` stores cut parameters. `framework/namespaces/general_utilities` holds environment/text/ROOT helpers and the restored converter mass constants. The acceptance-map/fiducial application is commented out in the archived converter; these classes are not active new generation dependencies. They are not a supported replacement for downstream acceptance analysis.

The archived root `genie_job_submission_script.csh` is another historical submission copy. There is one supported new Slurm runner. Do not infer which historical copy was last used from its location alone.

## 9. SSH checkout orchestration

[SSH workflow](ssh-workflow.md) documents the disposable ifarm checkout refresh and `config/run.json`. `src/launcher/workflow.py` reads build/test defaults, requires an explicit workflow and LUND source, builds/tests, and dispatches LUND creation. Submission is sourced directly by `run.csh`; the protected Bash payload is the array worker. Subprocess arguments are passed as lists and sourced wrappers preserve failure status.

`src/launcher/tests/launcher.py` exercises sourced/direct invocation, paths with spaces, failures, configuration/build calls and Git update safety using an isolated local repository. `src/lund-generation/tests/prepare_replacement_geometry.py` creates a changed target header; `src/lund-generation/tests/replacement_geometry.cpp` checks the actual adapter against that replacement, including new target discovery and RNG independence.

See [source documentation conventions](source-documentation.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded and protected from edits.

The [unified external GEMC payload](gemc-payload.md) documents `src/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with sourced-shell setup.
