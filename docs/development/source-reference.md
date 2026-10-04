# Source and API map

This page maps implementation files to responsibilities. The [architecture guide](../concepts/architecture.md) explains why those boundaries exist.

## Launcher and build

| Path | Responsibility |
| --- | --- |
| [`run.csh`](../../run.csh) | Operational entry point; bare help and limited submission syntax checks, disposable-checkout refresh, and workflow selection |
| [`src/launcher/workflow.py`](../../src/launcher/workflow.py) | `create-lund` build settings, CMake stages, executable selection, and exact child-argument forwarding |
| [`src/launcher/checkout/`](../../src/launcher/checkout) | Checked cleanup, reset, pull, and submodule synchronization |
| [`src/launcher/environment/`](../../src/launcher/environment) | Checkout and host preparation |
| [`src/launcher/presentation/`](../../src/launcher/presentation) | Shared colors, startup output, and success/stop artwork |
| [`CMakeLists.txt`](../../CMakeLists.txt) | Dependencies, workflow switches, C++ standard, compiled provenance, and top-level install rules |
| [`cmake/Version.h.in`](../../cmake/Version.h.in) | Generated project, Git, and target-header provenance |

[`config/run.json`](../../config/run.json) controls only launcher build/run behavior. Sample definitions belong under [`config/samples/`](../../config/samples); detector resources belong under [`config/detector/`](../../config/detector).

## Shared LUND core

Paths in the following three LUND sections are relative to [`src/workflows/lund-creation/`](../../src/workflows/lund-creation). Paths in the submission section are relative to [`src/workflows/`](../../src/workflows).

| Path or type | Responsibility |
| --- | --- |
| [`core/config/RunConfig.h`](../../src/workflows/lund-creation/core/config/RunConfig.h) | Merge defaults/profile/CLI, resolve automatic values, validate, and expose final settings |
| [`core/config/TargetCatalog.h`](../../src/workflows/lund-creation/core/config/TargetCatalog.h) | Map target identity to $A$/$Z$ and compatible beam-dependent GEMC variation/geometry |
| [`core/geometry/TargetGeometry.h`](../../src/workflows/lund-creation/core/geometry/TargetGeometry.h) | Sole project adapter to external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); vertex sampling and supported mass lookup |
| [`core/lund/Event.h`](../../src/workflows/lund-creation/core/lund/Event.h) (`Particle` and `Event`) | Generator-independent in-memory record passed to the writer |
| [`core/lund/LundWriter.h`](../../src/workflows/lund-creation/core/lund/LundWriter.h) | Guarded run replacement, file rotation, exact serialization, counts, manifest, and summaries |
| [`core/presentation/ProgressReporter.h`](../../src/workflows/lund-creation/core/presentation/ProgressReporter.h) | Interactive throttled bar or complete redirected progress lines |

`RunConfig::createFromCommandLine()` is side-effect-free with respect to run output. `LundWriter` begins the output lifecycle. `LundWriter::finalizeRun()` publishes the completion marker only after all required source-specific output succeeds.

## Uniform LUND creator

| Path | Responsibility |
| --- | --- |
| [`apps/uniform_lund_creator_main.cpp`](../../src/workflows/lund-creation/apps/uniform_lund_creator_main.cpp) | CLI entry, configuration construction, final error boundary |
| [`uniform-lund-creator/UniformConfig.h`](../../src/workflows/lund-creation/uniform-lund-creator/UniformConfig.h) | Typed hot-loop values and named channel/species choices |
| [`uniform-lund-creator/UniformGenerator.cpp`](../../src/workflows/lund-creation/uniform-lund-creator/UniformGenerator.cpp) | RNG ownership, event construction, writer/monitoring order, completion |
| [`uniform-lund-creator/UniformMonitoring.h`](../../src/workflows/lund-creation/uniform-lund-creator/UniformMonitoring.h) and [`.cpp`](../../src/workflows/lund-creation/uniform-lund-creator/UniformMonitoring.cpp) | Histogram definitions, labels, ROOT storage, PDF/PNG rendering |

The uniform LUND creator owns scientific sampling. It creates one vertex per event, writes before monitoring, and publishes the manifest only after monitoring is saved.

## Physical LUND converter

| Path | Responsibility |
| --- | --- |
| [`apps/event_generator_to_lund_converter_main.cpp`](../../src/workflows/lund-creation/apps/event_generator_to_lund_converter_main.cpp) | Stable physical-conversion CLI and error boundary |
| [`event-generator-to-lund-converter/PhysicalConverter.h`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.h) and [`.cpp`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.cpp) | Explicit adapter dispatch |
| [`event-generator-to-lund-converter/genie-gst/GenieConverterGST.h`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.h) and [`.cpp`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.cpp) | GST schema validation, event/process selection, particle translation, counts, and cutoff |

The GENIE adapter reads a `TChain("gst")` with typed `TTreeReader` values and arrays. It validates the current array sizes against `nf` before access and imposes no fixed particle buffer. It calls the common writer and creates no monitoring objects.

## Submission

| Path | Responsibility |
| --- | --- |
| [`slurm-submission/setup_and_submit.csh`](../../src/workflows/slurm-submission/setup_and_submit.csh) | Sourced shell bridge, shared colors, argument forwarding, and return status |
| [`slurm-submission/resolve_inputs.py`](../../src/workflows/slurm-submission/resolve_inputs.py) | CLI/config/manifest/default resolution and truth-conflict validation |
| [`slurm-submission/submit.py`](../../src/workflows/slurm-submission/submit.py) | GEMC and COATJAVA module loading and verification, reports, path checks, preview/execute actions, `sbatch`, submission record |
| [`slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) | Per-task GEMC then COATJAVA commands and scheduler directives |

Resolution completes for all selected samples before the coordinator changes output or submits the first array. Preview and execution use the same resolved data. The worker receives values through the Slurm environment and does not reimplement resolution.

## Shared workflow support

[`src/workflows/support/environment.h`](../../src/workflows/support/environment.h) maps colors inherited from [`src/launcher/presentation/set_colors.csh`](../../src/launcher/presentation/set_colors.csh) to C++ semantic constants. Workflows share the same success/stop lifecycle. Keep terminal escape definitions in the palette source rather than duplicating them in C++, Python, or CMake.

## Configuration resources

- [`config/samples/uniform-lund-creation/`](../../config/samples/uniform-lund-creation) contains complete beam/channel profiles.
- [`config/samples/physical-lund-creation/genie-gst.conf`](../../config/samples/physical-lund-creation/genie-gst.conf) is the current physical example.
- [`config/submission.conf`](../../config/submission.conf) is an optional submission example, not an automatic site file.
- [`config/detector/`](../../config/detector) contains protected GCARD and YAML snapshots.

The [LUND configuration reference](../create-lund/configuration.md) owns accepted creation settings. The [submission configuration reference](../submit-simulation/configuration.md) owns submission settings. Do not duplicate option defaults in implementation-overview prose unless the value is essential to understanding that component.
