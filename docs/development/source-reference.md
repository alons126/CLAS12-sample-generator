# Source and API map

Use this page to find the file that performs a particular task. CLI means command-line interface; API means the functions and types other code can use. The [architecture guide](../concepts/architecture.md) explains how these files work together.

## Launcher and build

| Path | Responsibility |
| --- | --- |
| [`run.csh`](../../run.csh) | Check early help and submission syntax, clean and update the ifarm checkout, then start the selected workflow |
| [`src/launcher/workflow.py`](../../src/launcher/workflow.py) | Read build settings, run CMake, choose the LUND application, and pass its arguments unchanged |
| [`src/launcher/checkout/`](../../src/launcher/checkout) | Check the repository, delete disposable files, discard edits, pull code, and update submodules |
| [`src/launcher/environment/`](../../src/launcher/environment) | Find the checkout and check the host environment before running |
| [`src/launcher/presentation/`](../../src/launcher/presentation) | Shared colors, startup output, and success/stop artwork |
| [`CMakeLists.txt`](../../CMakeLists.txt) | Select build dependencies, applications, and C++ standard; generate source-version information and define installation rules |
| [`cmake/Version.h.in`](../../cmake/Version.h.in) | Template for project version, Git details, and the target-header hash compiled into the applications |

[`config/run.json`](../../config/run.json) controls only launcher build/run behavior. Sample definitions belong under [`config/samples/`](../../config/samples); detector resources belong under [`config/detector/`](../../config/detector).

## Shared LUND core

Paths in the following three LUND sections are relative to [`src/workflows/lund-creation/`](../../src/workflows/lund-creation). Paths in the submission section are relative to [`src/workflows/`](../../src/workflows).

| Path or type | Responsibility |
| --- | --- |
| [`core/config/RunConfig.h`](../../src/workflows/lund-creation/core/config/RunConfig.h) | Combine defaults, profile, and command line; calculate automatic values; check and return final settings |
| [`core/config/TargetCatalog.h`](../../src/workflows/lund-creation/core/config/TargetCatalog.h) | Map target identity to $A$/$Z$ and compatible beam-dependent GEMC variation/geometry |
| [`core/geometry/TargetGeometry.h`](../../src/workflows/lund-creation/core/geometry/TargetGeometry.h) | Sole project adapter to external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); vertex sampling and supported mass lookup |
| [`core/lund/Event.h`](../../src/workflows/lund-creation/core/lund/Event.h) (`Particle` and `Event`) | Generator-independent in-memory record passed to the writer |
| [`core/lund/LundWriter.h`](../../src/workflows/lund-creation/core/lund/LundWriter.h) | Check and replace the run directory, split and format LUND files, count events, and write the completion log |
| [`core/presentation/ProgressReporter.h`](../../src/workflows/lund-creation/core/presentation/ProgressReporter.h) | Update one progress bar in a terminal, or print occasional full lines when output is redirected |

`RunConfig::createFromCommandLine()` reads and checks settings without creating or deleting sample output. `LundWriter` then prepares the run directory and writes files. `LundWriter::finalizeRun()` writes the final completion manifest only after all required output succeeds.

## Uniform LUND creator

| Path | Responsibility |
| --- | --- |
| [`apps/uniform_lund_creator_main.cpp`](../../src/workflows/lund-creation/apps/uniform_lund_creator_main.cpp) | Read command-line settings, start uniform creation, and print the final error on failure |
| [`uniform-lund-creator/UniformConfig.h`](../../src/workflows/lund-creation/uniform-lund-creator/UniformConfig.h) | Store typed settings used repeatedly in the event loop, including channel and particle choices |
| [`uniform-lund-creator/UniformGenerator.cpp`](../../src/workflows/lund-creation/uniform-lund-creator/UniformGenerator.cpp) | Create random-number generators, build and write events, fill monitoring, and finish output |
| [`uniform-lund-creator/UniformMonitoring.h`](../../src/workflows/lund-creation/uniform-lund-creator/UniformMonitoring.h) and [`.cpp`](../../src/workflows/lund-creation/uniform-lund-creator/UniformMonitoring.cpp) | Histogram definitions, labels, ROOT storage, PDF/PNG rendering |

The uniform LUND creator chooses random momenta and angles. It samples one vertex position per event and writes the event before adding it to monitoring histograms. It writes the final manifest only after the monitoring files are saved.

## Physical LUND converter

| Path | Responsibility |
| --- | --- |
| [`apps/event_generator_to_lund_converter_main.cpp`](../../src/workflows/lund-creation/apps/event_generator_to_lund_converter_main.cpp) | Read command-line settings, start physical conversion, and print the final error on failure |
| [`event-generator-to-lund-converter/PhysicalConverter.h`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.h) and [`.cpp`](../../src/workflows/lund-creation/event-generator-to-lund-converter/PhysicalConverter.cpp) | Call the adapter selected by `event-generator` |
| [`event-generator-to-lund-converter/genie-gst/GenieConverterGST.h`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.h) and [`.cpp`](../../src/workflows/lund-creation/event-generator-to-lund-converter/genie-gst/GenieConverterGST.cpp) | Check GST fields, select supported events and particles, count entries and output, and decide when to stop |

The GENIE adapter reads a `TChain("gst")` with typed `TTreeReader` values and arrays. It validates the current array sizes against `nf` before access and imposes no fixed particle buffer. It calls the common writer and creates no monitoring objects.

## Submission

| Path | Responsibility |
| --- | --- |
| [`slurm-submission/setup_and_submit.csh`](../../src/workflows/slurm-submission/setup_and_submit.csh) | Sourced shell bridge, shared colors, argument forwarding, and return status |
| [`slurm-submission/resolve_inputs.py`](../../src/workflows/slurm-submission/resolve_inputs.py) | CLI/config/manifest/default resolution and truth-conflict validation |
| [`slurm-submission/submit.py`](../../src/workflows/slurm-submission/submit.py) | GEMC and COATJAVA module loading and verification, reports, path checks, preview/execute actions, `sbatch`, submission record |
| [`slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) | Per-task GEMC then COATJAVA commands and scheduler directives |

The submission program reads and checks settings for all selected samples before changing output or submitting the first array. Preview and execution use those same settings. Slurm passes them to the worker as environment variables; the worker does not calculate them again.

## Shared workflow support

[`src/workflows/support/environment.h`](../../src/workflows/support/environment.h) maps colors inherited from [`src/launcher/presentation/set_colors.csh`](../../src/launcher/presentation/set_colors.csh) to C++ semantic constants. Workflows share the same success/stop lifecycle. Keep terminal escape definitions in the palette source rather than duplicating them in C++, Python, or CMake.

## Configuration resources

- [`config/samples/uniform-lund-creation/`](../../config/samples/uniform-lund-creation) contains complete beam/channel profiles.
- [`config/samples/physical-lund-creation/genie-gst.conf`](../../config/samples/physical-lund-creation/genie-gst.conf) is the current physical example.
- [`config/submission.conf`](../../config/submission.conf) is an optional submission example, not an automatic site file.
- [`config/detector/`](../../config/detector) contains protected GCARD and YAML snapshots.

Look up accepted creation settings in the [LUND configuration reference](../create-lund/configuration.md), and submission settings in the [submission configuration reference](../submit-simulation/configuration.md). Do not repeat option defaults in source-overview descriptions unless readers need the value to understand that component.
