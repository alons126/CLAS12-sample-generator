# Architecture and code walkthrough

## Build targets

| Target | Source | Responsibility |
| --- | --- | --- |
| `SampleCommon` | `src/common/` | Configuration, event representation, vertices, LUND output, monitoring |
| `UniformGeneration` | `src/uniform/` | Uniform sampling prescriptions |
| `GenieConversion` | `src/genie/` | Read GST and convert supported events |
| `clas12-uniform` | `apps/uniform_main.cpp` | Parse CLI, call generator, report errors |
| `clas12-genie-to-lund` | `apps/genie_to_lund_main.cpp` | Parse CLI, call converter, report errors |

The root CMake file discovers ROOT and adds subdirectories. `src/CMakeLists.txt` declares reusable libraries and target-scoped dependencies. The test targets alone compile archived reference code; production libraries do not include archived implementations. `apps/CMakeLists.txt` links entry points. Every implementation is compiled once; implementation files are never included from another implementation. ROOT macros and archived analysis helpers are excluded from production targets.

## Following a uniform run

1. The application calls `RunConfig::parse`. Built-in defaults are merged with a `key = value` file and then command-line overrides. Unknown, repeated and invalid settings fail before opening an output directory.
2. `generateUniform` resolves the channel and owns separate kinematic and vertex random streams. `TargetGeometry` samples a common interaction vertex for all particles in the event.
3. `Event` holds metadata and `Particle` values. Generation logic operates on these values, not on text formatting or shell commands.
4. `LundWriter` creates a new run directory, splits events into numbered files, and serializes all channels in the same format.
5. `Monitoring` owns detached ROOT histograms. It fills per-PDG momentum, angles, vertices and angular/momentum correlations.
6. `LegacyMonitoring` writes the original channel histogram names and binning to a separate ROOT file, with optional rendered plots.
7. After output and monitoring finish successfully, `LundWriter::finish` atomically renames the completed manifest into place.

## Following a GENIE run

`convertGenie` loads a `TChain("gst")` and validates required branches. Typed `TTreeReaderValue` and dynamically sized `TTreeReaderArray` objects avoid the imported fixed arrays of 250 particles. The converter assigns the legacy process code, filters supported PDG codes, creates an `Event`, and calls the same writer and monitoring code.

The converter stops at the configured output capacity or end of input. The final partial file is retained. No empty rollover file is opened. Errors reading later chain entries prevent publication of a completed manifest.

## Simulation boundary

`scripts/simulation/run.py` consumes `manifest.json` and explicit detector/site settings. It constructs argument lists for GEMC and `recon-util`; it does not invoke a shell to construct those commands. Dry runs print the commands without creating simulation directories. Execution checks return codes and output files and writes one record per completed file.

`scripts/slurm/submit.py` uses the same planning validation and submits an array with one task per manifest file. Each task invokes the runner with its one-based index. CMake never submits jobs.

## Adding functionality

- Add a sampling prescription in `src/uniform/` with validated settings and an output-level test of its distribution or invariants.
- Add another input converter as a separate library and CLI which produce `Event` values.
- Add target geometry centrally in `TargetGeometry.cpp`, including tests for its vertex bounds. Geometry and nuclear A/Z are separate choices.
- Add detector cards under `config/detector/` and select them explicitly at execution time.
- Keep machine paths, scheduler resources and binary names in site configuration.

Do not infer physics configuration from filenames or output paths. Do not add global RNGs or duplicate LUND formatting in individual workflows.

The complete [source/API inventory](code-reference.md) also covers tests, examples, error paths, and archived supporting utilities.
