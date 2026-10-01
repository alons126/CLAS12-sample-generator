# Architecture

The source tree follows the two user-facing workflows instead of one monolithic pipeline:

```text
src/
├── launcher/                       # run.csh support, build dispatch, checkout and presentation
└── workflows/
    ├── lund-creation/              # uniform creation and physical conversion
    ├── slurm-submission/           # ifarm validation and submission
    └── support/                    # C++ support shared by real workflow implementations
```

Future user-facing workflows belong beside the existing workflow directories. They should not be nested inside LUND creation or submission. Add shared infrastructure only after at least two workflows have the same concrete need.

## Entry points and control flow

`run.csh` is the operational front door. After early help/argument checks, it refreshes the disposable ifarm checkout and selects a workflow.

For LUND creation:

```text
run.csh
  -> src/launcher/workflow.py
  -> CMake configure/build when requested
  -> uniform-lund-creator or event-generator-to-lund-converter
  -> RunConfig
  -> source-specific event producer
  -> common Event/Particle model and LundWriter
  -> LUND files and completion manifest
```

For submission:

```text
run.csh
  -> setup_and_submit.csh
  -> submit.py
       -> resolve_inputs.py
       -> load and verify GEMC and COATJAVA modules; check inputs
       -> preview, or output replacement plus sbatch
  -> submit_GEMC_sample.sh in each Slurm task
       -> GEMC
       -> recon-util (COATJAVA)
```

The launcher never turns LUND creation into implicit submission. The submission coordinator never runs GEMC locally.

## LUND-creation layers

| Layer | Main responsibility |
| --- | --- |
| `apps/` | Thin CLI entry points and final error presentation |
| `core/config/` | Parse, merge, resolve, and validate settings; select target metadata |
| `core/geometry/` | Isolate the external target source and sample one vertex per event |
| `core/lund/` | Define `Event` and `Particle`; serialize, split, and publish completion |
| `core/presentation/` | Shared progress reporting for interactive and redirected output |
| `uniform-lund-creator/` | Generate random acceptance-test particles and uniform-only monitoring |
| `event-generator-to-lund-converter/` | Dispatch physical input to a format-specific adapter |
| `event-generator-to-lund-converter/genie-gst/` | Validate and translate GENIE GST records |

`LundCore` compiles the small common layers once. `UniformGeneration` depends on it and adds ROOT histogram/graphics components. `GenieGstConversion` depends on it and adds ROOT tree components. `PhysicalConversion` is the stable dispatcher in front of the format-specific adapter. The installed executables remain separate so a restricted build can omit an unused source and its ROOT components.

## Configuration boundary

Each executable constructs one `RunConfig` for its source. Resolution is deterministic:

```text
built-in common and source defaults
  -> optional sample profile
  -> command-line overrides
  -> automatic target/channel/beam values
  -> validation
  -> normalized input and final output paths
```

The object stores final values as strings for provenance and exposes checked typed readers to the event code. It does not generate events, advance random streams, modify output, or submit jobs. Source-specific keys are rejected in the wrong source instead of being ignored.

## Common event boundary

Source-specific code produces `Event` objects containing ordered `Particle` records. That boundary keeps scientific input logic away from text serialization:

- the uniform LUND creator decides which random particles exist;
- a physical adapter decides which input events and particles are supported;
- `TargetGeometry` supplies one vertex shared by every particle in the event; and
- `LundWriter` owns file names, splitting, exact formatting, output replacement, counts, and the final manifest.

This separation is the main extension rule. A new input adapter should translate its records into `Event`; it should not copy the writer, target sampling, or completion logic.

## Uniform path

`generateUniform()` converts resolved settings into a typed `UniformConfig`, creates separate kinematic and vertex RNGs, and generates events until the requested written count is reached. It writes each event before adding it to `UniformMonitoring`, so diagnostics never count an event that failed to reach LUND. Monitoring is saved before the manifest is published.

## Physical path

`convertPhysical()` dispatches the selected adapter. `convertGenieGST()` validates the ROOT tree and branch types, scans entries in order, selects supported interactions, copies supported truth particles, and calls the common writer. It owns scanned-versus-written accounting and the input-tail cutoff. It creates no monitoring histograms.

To add another format, create a sibling adapter and one explicit dispatcher branch. Keep the public executable and shared output contract. The [adapter guide](../development/adding-event-generator.md) lists the required scientific and software decisions.

## Submission boundary

`resolve_inputs.py` is a pure resolution and validation layer: CLI, optional config, manifest, and defaults become one checked settings record per sample. GEMC and COATJAVA versions are separate submission settings: each selects its software module and corresponding configuration directory. Explicit GCARD or YAML files override file lookup without changing software selection.

`submit.py` loads both modules in a private environment and verifies both installations and executable paths after the loads. It also owns reports, guarded directory actions, the `sbatch` call, and the submission record. The sourced shell bridge owns only shell integration and return status. The external worker inherits the checked environment and owns only commands executed by an array task; it does not select or load software releases.

This division prevents shell variables, path-name guesses, or worker-specific branches from becoming hidden configuration. It also keeps preview and execution on the same resolution path.

## Protected external boundaries

Two imported sources sit behind small interfaces:

- `src/workflows/lund-creation/external/targets.h` supplies target geometry and particle masses through `TargetGeometry`.
- `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` supplies the GEMC/COATJAVA worker command boundary.

Detector GCARD and YAML files under `config/detector/` are also external campaign resources. Replace these deliberately and validate the resulting production chain; do not edit them as routine project code. See [external inputs](external-inputs.md).

## Rules for future workflows

A new workflow needs one clear entry point, input contract, output contract, configuration path, and owning directory. Keep workflow-specific file formats and physics inside that directory. Do not create empty placeholders or a generic orchestration framework for planned work. Reuse current components only when their contract genuinely matches the new workflow.
