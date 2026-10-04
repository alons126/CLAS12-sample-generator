# Architecture

LUND creation and simulation submission have separate directories under [`src/workflows/`](../../src/workflows/) because users run them separately. The shared launcher lives under [`src/launcher/`](../../src/launcher/):

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

Users start an ifarm workflow with [`run.csh`](../../run.csh). It cleans and updates the repository copy, then starts the selected workflow. Bare launcher help and limited submission syntax checks can return before cleanup; full workflow checks run afterward. Read the [checkout model](../submit-simulation/ifarm-environment.md#disposable-checkout) for the exact deletion rules and exceptions.

For LUND creation, [`workflow.py`](../../src/launcher/workflow.py) coordinates the build and application launch:

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

For submission, [`setup_and_submit.csh`](../../src/workflows/slurm-submission/setup_and_submit.csh) bridges the sourced shell to the Python coordinator:

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

Creating LUND files never submits jobs automatically. The submission program asks Slurm to run GEMC on computing nodes; it does not run GEMC in the login shell or on the development workstation.

## LUND-creation layers

| Layer | Main responsibility |
| --- | --- |
| [`apps/`](../../src/workflows/lund-creation/apps) | Thin CLI entry points and final error presentation |
| [`core/config/`](../../src/workflows/lund-creation/core/config/) | Parse, merge, resolve, and validate settings; select target metadata |
| [`core/geometry/`](../../src/workflows/lund-creation/core/geometry/) | Isolate the external target source and sample one vertex per event |
| [`core/lund/`](../../src/workflows/lund-creation/core/lund/) | Define `Event` and `Particle`; serialize, split, and publish completion |
| [`core/presentation/`](../../src/workflows/lund-creation/core/presentation/) | Shared progress reporting for interactive and redirected output |
| [`uniform-lund-creator/`](../../src/workflows/lund-creation/uniform-lund-creator) | Generate random acceptance-test particles and uniform-only monitoring |
| [`event-generator-to-lund-converter/`](../../src/workflows/lund-creation/event-generator-to-lund-converter) | Dispatch physical input to a format-specific adapter |
| [`event-generator-to-lund-converter/genie-gst/`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/) | Validate and translate GENIE GST records |

`LundCore` compiles the small common layers once. `UniformGeneration` depends on it and adds ROOT histogram/graphics components. `GenieGstConversion` depends on it and adds ROOT tree components. `PhysicalConversion` is the stable dispatcher in front of the format-specific adapter. The installed executables remain separate so a restricted build can omit an unused source and its ROOT components.

## Configuration boundary

For example, `--events 100` replaces the profile's event count before creation starts. Each executable uses one `RunConfig` to combine settings in this order:

```text
built-in common and source defaults
  -> optional sample profile
  -> command-line overrides
  -> automatic target/channel/beam values
  -> validation
  -> normalized input and final output paths
```

`RunConfig` stores the final settings as strings so they can be copied into the log. Its accessors convert checked values to the types the event code needs. It does not generate events, draw random numbers, change output files, or submit jobs. An option belonging only to the other source causes an error rather than being ignored.

## Common event boundary

The uniform LUND creator and each physical-input adapter build `Event` objects containing particles in the order they should be written. `LundWriter` then converts those objects into LUND text. This keeps the decision about which particles exist separate from file formatting:

- the uniform LUND creator decides which random particles exist;
- a physical adapter decides which input events and particles are supported;
- `TargetGeometry` supplies one vertex shared by every particle in the event; and
- `LundWriter` owns file names, splitting, exact formatting, output replacement, counts, and the final manifest.

This separation is the main extension rule. A new input adapter should translate its records into `Event`; it should not copy the writer, target sampling, or completion logic.

## Uniform path

[`generateUniform()`](../../src/workflows/lund-creation/uniform-lund-creator/UniformGenerator.cpp#L153) copies checked settings into a typed `UniformConfig`. It creates separate random-number generators for momenta and angles and for vertex positions, then writes the requested number of events. It writes each event before adding it to `UniformMonitoring`, so histograms never count an event that failed to reach LUND. Monitoring files are saved before the final manifest is written.

## Physical path

[`convertPhysical()`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.cpp#L41) calls the selected input adapter. [`convertGenieGST()`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.cpp#L64) checks the ROOT tree and branch types, reads entries in order, selects supported interactions, copies supported particles, and calls the common writer. It separately counts entries examined and events written, and decides whether enough input remains to start another file. It creates no monitoring histograms.

To read another input format, add a directory beside [`genie-gst/`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/) and a branch in [`convertPhysical()`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.cpp#L41) that calls its reader. Keep the same command-line executable and use the common writer for output. The [adapter guide](../development/adding-event-generator.md) lists the decisions and checks needed.

## Submission boundary

[`resolve_inputs.py`](../../src/workflows/slurm-submission/resolve_inputs.py) combines command-line options, an optional config, the manifest, and defaults into one checked settings record per sample. It does not submit jobs or change output directories. The GEMC and COATJAVA version settings independently select the software and its default configuration directory. Supplying a GCARD or YAML changes the selected file, not the software version.

[`submit.py`](../../src/workflows/slurm-submission/submit.py) unloads and loads GEMC and switches COATJAVA with `module switch coatjava/<coatjava-version>` in a private environment. It verifies GEMC's installation, checks and prints the loaded COATJAVA release, and checks that the required programs are available. COATJAVA version validation does not depend on installation-directory names. It also owns reports, guarded directory actions, the `sbatch` call, and the submission record. The sourced shell bridge owns only shell integration and return status. The external worker inherits the checked environment and owns only commands executed by an array task; it does not select or load software releases.

Both preview and execution use the same checked settings. The worker receives those settings from the submission program rather than guessing them from directory names or choosing its own defaults.

## Protected external boundaries

Two imported sources sit behind small interfaces:

- [`src/workflows/lund-creation/external/targets.h`](../../src/workflows/lund-creation/external/targets.h) supplies target geometry and particle masses through `TargetGeometry`.
- [`src/workflows/slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) supplies the GEMC/COATJAVA worker command boundary.

Detector GCARD and YAML files under [`config/detector/`](../../config/detector) are also external campaign resources. Replace these deliberately and validate the resulting production chain; do not edit them during routine code changes. See [external inputs](external-inputs.md).

## Rules for future workflows

A new workflow needs its own directory, command users run, settings, accepted inputs, and documented outputs. Keep its file formats and physics code there. Do not create empty directories or a general-purpose controller for workflows that do not exist yet. Reuse a current component only when it already does what the new workflow needs.
