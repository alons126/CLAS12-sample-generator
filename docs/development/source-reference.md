# Source and API reference

This chapter inventories the supported code and the archived support code so a future technical note can distinguish the implemented methods from historical dependencies.

## 1. Build and application layer

| File | Responsibilities and interfaces |
| --- | --- |
| `CMakeLists.txt` | Defines project/version and BUILD_UNIFORM/BUILD_GENIE; discovers ROOT; matches ROOT's C++ standard; configures revision header; adds production libraries/apps and installation |
| `src/CMakeLists.txt` | Adds the LUND-generation, Slurm-submission, and shared-launcher source trees |
| `src/lund-generation/CMakeLists.txt` | Defines `LundCore`, `UniformGeneration`, `GenieGstConversion`, and `PhysicalConversion` |
| `src/slurm-submission/CMakeLists.txt` | Installs the external submission worker |
| `src/launcher/CMakeLists.txt` | Keeps the launcher source boundary separate from installed targets |
| `src/lund-generation/apps/CMakeLists.txt` | Defines and installs the two application targets |
| `src/lund-generation/apps/uniform_lund_generator_main.cpp` | Uniform generator entry point and error reporting |
| `src/lund-generation/apps/event_generator_to_lund_converter_main.cpp` | Generator-independent physical entry point and error reporting |
| `.vscode/c_cpp_properties.json` | Uses Debug compile_commands.json for editor compiler/include settings |

Production sources compile once into conventional targets. Archived implementation files are not linked into production targets.

## 2. Shared maintained layers

The shared LUND pipeline is organized by responsibility inside `src/lund-generation/`. These directories compile into the single `LundCore` target because they are small and always used together. External target geometry is isolated under `src/lund-generation/external/`; the external simulation payload belongs separately to `src/slurm-submission/external/`.

### Configuration (`src/lund-generation/core/config/`)

[RunConfig.h](../../src/lund-generation/core/config/RunConfig.h) declares the shared, read-only handoff from each C++ command-line entry point to its generator/converter and writer. [RunConfig.cpp](../../src/lund-generation/core/config/RunConfig.cpp) implements the accepted common and source-specific key sets, strict `--key value` and `key = value` parsing, precedence, RG-M target lookup, automatic sampling/provenance settings, output naming, path normalization, and validation. Typed readers expose checked values and `RunConfig::values()` supplies the exact resolved strings for manifest provenance.

Configuration is parsed once; `UniformConfig` converts frequently used settings to typed values before the event loop. `RunConfig` does not inspect event data, own RNGs, mutate output directories, serialize LUND, monitor events, or submit simulation. Unknown/duplicate keys and invalid ranges fail before those operations can begin. Config syntax and defaults are in [configuration](../create-lund/configuration.md).

### LUND records and serialization (`src/lund-generation/core/lund/`)

[Event.h](../../src/lund-generation/core/lund/Event.h) declares supported PDG identifiers, `Particle`, `Event`, and `particleMass(pid)`. [Particle.cpp](../../src/lund-generation/core/lund/Particle.cpp) delegates mass lookup to the target-source adapter. [TargetGeometry.cpp](../../src/lund-generation/core/geometry/TargetGeometry.cpp) is the only maintained translation unit that includes external [`targets.h`](../../src/lund-generation/external/targets.h); it returns that source's electron, proton, neutron, and charged-pion masses, returns zero for photons, and rejects unsupported species.

[LundWriter.h](../../src/lund-generation/core/lund/LundWriter.h) / [LundWriter.cpp](../../src/lund-generation/core/lund/LundWriter.cpp): constructor validates and recreates the resolved run directory; `LundWriter::full()` checks event capacity; `LundWriter::write()` serializes an event and rotates files at the resolved `events-per-file` threshold; `LundWriter::finish(scanned)` publishes the manifest. Setup reporting groups run limits, beam/target values, active source settings, and real output paths with one resolved value per line; completion reporting contains only counters and status. Fixed serialization constants and inactive channel settings are omitted. Uniform construction also prepares its downstream/output directories. Output-stream exceptions propagate; failed runs may leave partial output without a manifest.

### Geometry (`src/lund-generation/core/geometry/`)

[RgmTarget.h](../../src/lund-generation/core/config/RgmTarget.h) / [RgmTarget.cpp](../../src/lund-generation/core/config/RgmTarget.cpp) map RG-M material/assembly identifiers to A/Z, external geometry keys, and GEMC variations. [TargetGeometry.h](../../src/lund-generation/core/geometry/TargetGeometry.h) / [TargetGeometry.cpp](../../src/lund-generation/core/geometry/TargetGeometry.cpp) validate and sample the external geometry under an isolated RNG lock and expose the same external source's particle masses through a read-only lookup. Every maintained mode, including the electron tester, samples its selected geometry. The adapter is the only maintained translation unit that includes `src/lund-generation/external/targets.h`; that imported header remains in place so replacing it does not mix external ownership with maintained code.

The imported header is kept as an exact RG-M copy, while `src/slurm-submission/external/submit_GEMC_sample.sh` is a modified RG-M-derived script that retains its source structure and usage pattern. The maintained adapters around both files provide narrow update points for later RG-M releases; detailed provenance and replacement guidance are in [external inputs](../concepts/external-inputs.md) and the [worker reference](../submit-simulation/worker-reference.md).

### Project support (`src/support/`)

[set_colors.csh](../../src/support/environment/set_colors.csh) owns the shared ANSI palette and exports it through `*_COLOR` environment variables. The other scripts in [`src/support/environment/`](../../src/support/environment/) prepare shared banners and the checkout/host environment used across workflow boundaries. [environment.h](../../src/support/environment.h) decodes the inherited palette once for C++ and exposes immutable semantic colors for errors, completion, system messages, information, warnings, and reset. Application entry points, workflow summaries, replacement warnings, and completion messages reference those names instead of defining escape sequences locally. A C++ executable launched without the shared environment uses empty color strings, so its output remains readable without introducing a second fallback palette.

The scripts in [`src/support/printers/`](../../src/support/printers/) render the optional startup and final-status artwork used by the top-level dispatcher. [Version.h.in](../../src/support/Version.h.in) embeds project version, target-header SHA-256, and full configure-time Git repository/commit/status/tracking metadata into the generated `Version.h` used by the manifest. This is build provenance, not a runtime Git dependency.

## 3. Uniform-to-LUND implementation

[UniformConfig.h](../../src/lund-generation/uniform-lund-generator/UniformConfig.h) defines the `UniformChannel` and `HadronSpecies` enums and the typed configuration used by the hot loop. It includes angular/momentum bounds, resolved mode booleans, trigger parameters and A/Z.

[UniformGenerator.h](../../src/lund-generation/uniform-lund-generator/UniformGenerator.h) / [UniformGenerator.cpp](../../src/lund-generation/uniform-lund-generator/UniformGenerator.cpp) expose `generateUniform(const RunConfig&)`. The function owns RNGs, geometry, one `UniformMonitoring` object and a writer. Internal `momentum()` constructs Cartesian vectors; `triggerPhi()` retains the archived sector/tie convention. Each loop iteration samples a vertex and the configured particles, writes the event, then fills diagnostics. After completion it saves one ROOT product, the required rendered plots, and the generation log.

[UniformMonitoring.h](../../src/lund-generation/uniform-lund-generator/UniformMonitoring.h) / [UniformMonitoring.cpp](../../src/lund-generation/uniform-lund-generator/UniformMonitoring.cpp) own the complete uniform-only monitoring contract. The implementation preserves the legacy organization, titles, correlations, axis text settings and canvas layout, sets every vertex-z axis to −8–5 cm to cover the target positions of all RG-M targets[^sportes-2026-rgm][^rgm-analysis-note], and generalizes hadron tokens to `pFD`, `pCD`, `nFD`, `nCD`, `pipFD`, `pipCD`, `pimFD`, and `pimCD`. `UniformMonitoring::save()` writes all histograms once to `<prefix>_monitoring_plots.root` and always renders those same objects into `MonitoringPlotsPath`.

The 1e electron and charged-hadron branches alternate uniform-p and uniform-1/p components using the run-global index. Neutrons use uniform momentum unless their optional fixed mode is selected. Hadron species and FD/CD region resolve the documented angular and threshold defaults. Mathematical definitions are in [sampling models](../concepts/sampling-models.md).

## 4. Event-generator-to-LUND implementation

[PhysicalConverter.h](../../src/lund-generation/event-generator-to-lund-converter/PhysicalConverter.h) / [PhysicalConverter.cpp](../../src/lund-generation/event-generator-to-lund-converter/PhysicalConverter.cpp) provide the stable physical-source dispatch. `event-generator=genie-gst` selects the current format-specific adapter; future adapters join here without changing the public executable.

[GenieConverterGST.h](../../src/lund-generation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.h) / [GenieConverterGST.cpp](../../src/lund-generation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.cpp) are nested below the physical dispatcher because GENIE GST is one adapter of the `event-generator-to-lund-converter` executable. They expose `convertGenieGST(const RunConfig&)`. A `TChain("gst")` feeds typed `TTreeReaderValue`/`TTreeReaderArray` objects. The function checks branches/types/array lengths, selects process/species, and writes an `Event`. Physical conversion creates no monitoring histograms.

The reader arrays have no maintained fixed-size particle buffer. ROOT reports each current-entry length through `TTreeReaderArray::GetSize()` from the branch's leaf-count metadata. Conversion requires every reported length to equal nonnegative `nf` before accessing index zero, and a 300-supported-particle fixture verifies traversal through the final element. Protons, neutrons, charged pions and photons are copied in input order. Residual neutral pions are skipped because their two-photon decay must be generated upstream. Only QE, MEC, RES, and DIS reactions are supported; adding another reaction requires updating the adapter. Input errors, unsupported-only input and output failures do not publish a manifest. Before a follow-up file starts, the physical-input cutoff requires at least `events-per-file` inclusive input entries; it never interrupts a file already in progress. Capacity still counts accepted events. Schema and process conventions are in the [GENIE guide](../create-lund/physical.md).

## 5. Execution scripts

| File | Contract |
| --- | --- |
| `run.csh` | Guarded ifarm refresh; source submission directly or dispatch LUND creation |
| `src/launcher/workflow.py` | LUND configuration, build stages and application dispatch |
| `src/slurm-submission/resolve_inputs.py` | Manifest/config/CLI precedence, truth validation, portable file inventory and in-memory resolved settings |
| `src/slurm-submission/setup_and_submit.csh` | Small sourced bridge: shared palette, quoted arguments and Python exit status |
| `src/slurm-submission/submit.py` | Preloaded environment, established report/checks, guarded output reset and one array per sample |
| `src/slurm-submission/external/submit_GEMC_sample.sh` | External Slurm task payload; GEMC followed by reconstruction |

The resolver obtains the prefix and task count from the completed manifest or explicit input, then the setup script consumes the validated LUND files. It exports a shared event limit for the array, defaulting to the largest selected manifest file count. Before handoff, the Python coordinator writes `reconhipo/slurm-submission-log.json` with resolved settings, runtime Git identity, the exact `sbatch` command, and detector-input/payload hashes. The payload retains its original scheduler defaults. See the [submission guide](../submit-simulation/guide.md).

## 6. Configuration and resources

- `config/samples/uniform-<label>-{2070,4029,5986}MeV.conf`: complete Ar40 profiles for every supported 1e/FD/CD label at each established beam energy; pion and CD files are explicitly marked unvalidated for production.
- `electron-tester-{2070,4029,5986}MeV.conf`: beam-specific tester profiles with fixed beam momentum and target-sampled vertices.
- `genie-gst.conf`: an explicit Ar conversion example.
- `legacy-coderun.conf`, `legacy-genie-wrapper.conf`: active archived launch settings; override their production-sized counts for smoke tests.
- `config/detector/Generation_files_*`: unchanged 2/4/6 GeV cards and reconstruction YAML for the archived versions. They are resources, not generated models. Matching detector/data dependencies are external.

## 7. Archived supporting code

The public repository baseline for the archived sources is the [`legacy-v1.0.0` GitHub release tag](https://github.com/alons126/CLAS12-sample-generator/releases/tag/legacy-v1.0.0). Use that tag when comparing maintained code with the historical tree described below.

`legacy/Uniform-sample-generator/` is a Git submodule pinned to the independent `alons126/Uniform-sample-generator` repository. It retains the configuration/path helpers, text printing, particle formatter, angle calculation, target globals, histogram globals, main/ROOT launchers, tester and upstream historical material. Production targets do not link the submodule.

`legacy/GEMC-samples/` retains the converter, geometry helper, shell setup/submission chains and resource snapshots. Its `framework/classes/AMaps` implements historical acceptance-map lookup, `hPlots` implements plotting containers, and `DSCuts` stores cut parameters. `framework/namespaces/general_utilities` holds environment/text/ROOT helpers and the restored converter mass constants. The acceptance-map/fiducial application is commented out in the archived converter; these classes are not active new generation dependencies. They are not a supported replacement for downstream acceptance analysis.

The archived root [`genie_job_submission_script.csh`](../../legacy/genie_job_submission_script.csh) is another historical submission copy. There is one supported new Slurm runner. Do not infer which historical copy was last used from its location alone.

## 8. SSH checkout orchestration

[SSH workflow](../submit-simulation/ifarm-environment.md) documents the disposable ifarm checkout refresh and `config/run.json`. `src/launcher/workflow.py` reads build defaults, requires an explicit workflow and LUND source, builds when requested, and dispatches LUND creation. Submission is sourced directly by `run.csh`; the external Bash payload is the array worker. Subprocess arguments are passed as lists and sourced wrappers preserve failure status.

See [source documentation conventions](documentation-style.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded from edits.

The [unified external GEMC payload](../submit-simulation/worker-reference.md) documents `src/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python setup and its sourced shell bridge.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
