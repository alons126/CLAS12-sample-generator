# CMakePresets.json — configuration, build and test entry points

## Description and purpose

[CMakePresets.json](CMakePresets.json) provides named commands for the maintained CMake build. Its supported `displayName` and `description` fields explain each preset inside the JSON itself. This companion supplies the workflow and field contracts without introducing unsupported JSON comments or keys.

## Workflow

1. `cmake --preset debug` configures the project and discovers dependencies.
2. `cmake --build --preset debug --parallel 4` compiles the configured targets.
3. `ctest --preset debug` runs tests when `BUILD_TESTING=ON`.

For Release, use the `release` configure/build presets and `ctest --test-dir build/release --output-on-failure`. There is no named Release test preset. Presets do not launch sample production or submit jobs.

## Fields and sections

| Field | Meaning |
| --- | --- |
| `version: 2` | Preset-file schema version; distinct from the project version and build type |
| `configurePresets` | Configuration commands that create/update a build tree |
| `name` | CLI identifier passed to `--preset` |
| `displayName`, `description` | Human-readable metadata; do not change compilation settings |
| `generator` | `Unix Makefiles`, inherited by Release |
| `binaryDir` | Build directory; `${sourceDir}` expands to the checkout root |
| `cacheVariables.CMAKE_BUILD_TYPE` | `Debug` or `Release` compiler configuration |
| `inherits` | Release starts with the Debug preset and overrides its directory/build type |
| `buildPresets` | Build commands tied to existing configure presets |
| `configurePreset` | Configure preset whose build directory is used |
| `testPresets` | Named CTest invocations; currently Debug only |
| `output.outputOnFailure` | Show captured diagnostics when a test fails |

## Inputs, outputs and failure behavior

ROOT and the compiler must be available in the calling environment. Configuration generates build files and `compile_commands.json`; building produces libraries, executables and optional test helpers. Missing dependencies or incompatible ROOT settings fail configuration. A disabled test cache must be re-enabled with `cmake --preset debug -DBUILD_TESTING=ON` before relying on CTest.

`source run.csh` uses its JSON launcher settings and explicit CMake commands, rather than invoking these presets. See [SSH workflow](docs/submit-simulation/ifarm-environment.md).
