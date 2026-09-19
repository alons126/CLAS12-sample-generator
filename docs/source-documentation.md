# Reading explanations inside the source

The maintained source uses the explanation style of the reference `FiducialVolumeMaps` class. Start with a file's purpose and workflow description, then use named banners and foldable regions to locate an operation. Public headers describe interfaces and ownership; implementations explain algorithms, parameters, output meanings and important failure behavior. Inline comments identify the intent of major stages.

C++ uses Doxygen blocks (`@file`, `@class`/`@struct`, `@brief`, `@param`, `@return`/`@note`) and paired `#pragma region` / `#pragma endregion` markers. Existing `#pragma once` inclusion guards remain separate from those navigation markers. Python uses module/function docstrings and `# region` / `# endregion` comments. Shell scripts use comment blocks for purpose, workflow, usage, inputs and exit-status behavior, with the same comment-region convention. Region folding depends on editor support; shell/Python region markers are ordinary comments.

Descriptions follow each component's actual responsibilities. Small accessors need a short contract; event loops and orchestration functions need ordered stages. Names, units, ownership, configuration precedence and differences between preview and execution should be explicit. Comments must be updated when the implementation changes.

Maintained C++ terminal colors are defined only in `src/lund-generation/core/support/environment.h`. Other C++ files select its semantic constants and must not contain literal ANSI escape definitions.

External and archived files are excluded and protected from edits: `legacy/`, `src/lund-generation/external/targets.h`, `src/slurm-submission/external/submit_GEMC_sample.sh`, and every file recursively under `config/detector/`. The documentation convention does not authorize changes to those files. Maintained test adapters can explain how they read the protected sources and create isolated reference fixtures. Repository instructions are recorded in `AGENTS.md`.

## Build and JSON configuration

Maintained `CMakeLists.txt` files use comment-based description, purpose, workflow, input/output and failure notes, plus named `# region` / `# endregion` sections. The comments explain target dependencies, optional workflows, generated files and installation boundaries without changing CMake commands.

Strict JSON does not support comments. CMake presets use their supported `displayName` and `description` metadata; runtime profiles keep their existing schemas and have adjacent `.json.md` field references. Do not add invented `_comment` keys to readers that reject unknown settings. VS Code's JSONC files support inline comment banners and regions despite their `.json` filenames.

| Configuration | Explanation |
| --- | --- |
| `CMakePresets.json` | [Preset commands and fields](../CMakePresets.json.md), plus descriptions inside the presets |
| `config/run.json` | [Launcher build/test defaults and precedence](../config/run.json.md) |
| `.vscode/c_cpp_properties.json` | Inline comments explain compile-command-based editor configuration |
| `.vscode/settings.json` | Inline comments explain language associations, formatting, folding and highlighting |

Generated build-tree JSON and protected external/legacy configurations are excluded.

## Data objects and state

The same layers apply to structs, classes, enums, private implementation records, constants and configuration containers. An object description explains its purpose, producers/consumers, ownership and lifetime; member comments explain units, defaults, allowed values and relationships. Named object regions make nested definitions navigable. Passive records need a usage/lifecycle explanation rather than an invented algorithm. For example, `Particle` and `Event` document the generator-to-writer data contract, while diagnostic storage records explain ownership of detached ROOT histograms.

Every file under `config/detector/` is classified as external and read-only for the assistant, recursively and regardless of extension. This includes current and future files, not only recognized gcard/YAML resources.

The unified `src/slurm-submission/external/submit_GEMC_sample.sh` and its two archived source payloads are protected external code, including their monitoring modifications. Skip them in routine source-documentation passes; their interface is described in [the GEMC payload guide](gemc-payload.md).
