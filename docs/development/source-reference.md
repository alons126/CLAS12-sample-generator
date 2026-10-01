# Source and API map

This page maps implementation files to responsibilities. The [architecture guide](../concepts/architecture.md) explains why those boundaries exist.

## Launcher and build

| Path | Responsibility |
| --- | --- |
| `run.csh` | Operational entry point; early help/argument checks, disposable-checkout refresh, and workflow selection |
| `src/launcher/workflow.py` | `create-lund` build settings, CMake stages, executable selection, and exact child-argument forwarding |
| `src/launcher/checkout/` | Checked cleanup, reset, pull, and submodule synchronization |
| `src/launcher/environment/` | Checkout and host preparation |
| `src/launcher/presentation/` | Shared colors, startup output, and success/stop artwork |
| `CMakeLists.txt` | Dependencies, workflow switches, C++ standard, compiled provenance, and top-level install rules |
| `cmake/Version.h.in` | Generated project, Git, and target-header provenance |

`config/run.json` controls only launcher build/run behavior. Sample definitions belong under `config/samples/`; detector resources belong under `config/detector/`.

## Shared LUND core

| Path or type | Responsibility |
| --- | --- |
| `core/config/RunConfig` | Merge defaults/profile/CLI, resolve automatic values, validate, and expose final settings |
| `core/config/TargetCatalog` | Map target identity to $A$/$Z$ and compatible beam-dependent GEMC variation/geometry |
| `core/geometry/TargetGeometry` | Sole project adapter to external `targets.h`; vertex sampling and supported mass lookup |
| `core/lund/Particle` and `Event` | Generator-independent in-memory record passed to the writer |
| `core/lund/LundWriter` | Guarded run replacement, file rotation, exact serialization, counts, manifest, and summaries |
| `core/presentation/ProgressReporter` | Interactive throttled bar or complete redirected progress lines |

`RunConfig::createFromCommandLine()` is side-effect-free with respect to run output. `LundWriter` begins the output lifecycle. `LundWriter::finalizeRun()` publishes the completion marker only after all required source-specific output succeeds.

## Uniform LUND creator

| Path | Responsibility |
| --- | --- |
| `apps/uniform_lund_creator_main.cpp` | CLI entry, configuration construction, final error boundary |
| `uniform-lund-creator/UniformConfig.h` | Typed hot-loop values and named channel/species choices |
| `uniform-lund-creator/UniformGenerator.cpp` | RNG ownership, event construction, writer/monitoring order, completion |
| `uniform-lund-creator/UniformMonitoring.*` | Histogram definitions, labels, ROOT storage, PDF/PNG rendering |

The uniform LUND creator owns scientific sampling. It creates one vertex per event, writes before monitoring, and publishes the manifest only after monitoring is saved.

## Physical LUND converter

| Path | Responsibility |
| --- | --- |
| `apps/event_generator_to_lund_converter_main.cpp` | Stable physical-conversion CLI and error boundary |
| `event-generator-to-lund-converter/PhysicalConverter.*` | Explicit adapter dispatch |
| `event-generator-to-lund-converter/genie-gst/GenieConverterGST.*` | GST schema validation, event/process selection, particle translation, counts, and cutoff |

The GENIE adapter reads a `TChain("gst")` with typed `TTreeReader` values and arrays. It validates the current array sizes against `nf` before access and imposes no fixed particle buffer. It calls the common writer and creates no monitoring objects.

## Submission

| Path | Responsibility |
| --- | --- |
| `slurm-submission/setup_and_submit.csh` | Sourced shell bridge, shared colors, argument forwarding, and return status |
| `slurm-submission/resolve_inputs.py` | CLI/config/manifest/default resolution and truth-conflict validation |
| `slurm-submission/submit.py` | GEMC and COATJAVA module loading and verification, reports, path checks, preview/execute actions, `sbatch`, submission record |
| `slurm-submission/external/submit_GEMC_sample.sh` | Per-task GEMC then COATJAVA commands and scheduler directives |

Resolution completes for all selected samples before the coordinator changes output or submits the first array. Preview and execution use the same resolved data. The worker receives values through the Slurm environment and does not reimplement resolution.

## Shared workflow support

`src/workflows/support/environment.h` maps colors inherited from `src/launcher/presentation/set_colors.csh` to C++ semantic constants. Workflows share the same success/stop lifecycle. Keep terminal escape definitions in the palette source rather than duplicating them in C++, Python, or CMake.

## Configuration resources

- `config/samples/uniform-lund-creation/` contains complete beam/channel profiles.
- `config/samples/physical-lund-creation/genie-gst.conf` is the current physical example.
- `config/submission.conf` is an optional submission example, not an automatic site file.
- `config/detector/` contains protected GCARD and YAML snapshots.

The [configuration reference](../create-lund/configuration.md) owns accepted LUND settings. The [submission guide](../submit-simulation/guide.md) owns submission settings. Do not duplicate option defaults in implementation-overview prose unless the value is essential to understanding that component.
