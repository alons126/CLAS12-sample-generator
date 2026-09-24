# Reading explanations inside the source

The maintained source uses the explanation style of the reference `FiducialVolumeMaps` class. Start with a file's purpose and workflow description, then use named banners and foldable regions to locate an operation. Public headers describe interfaces and ownership; implementations explain algorithms, parameters, output meanings and important failure behavior. Inline comments identify the intent of major stages.

C++ uses Doxygen blocks (`@file`, `@class`/`@struct`, `@brief`, `@param`, `@return`/`@note`) and paired `#pragma region` / `#pragma endregion` markers. Existing `#pragma once` inclusion guards remain separate from those navigation markers. Python uses module/function docstrings and `# region` / `# endregion` comments. Shell scripts use comment blocks for purpose, workflow, usage, inputs and exit-status behavior, with the same comment-region convention. Region folding depends on editor support; shell/Python region markers are ordinary comments.

Write explanations in simple, direct language that a newcomer can follow. Prefer common words and short sentences. Keep a technical term when it names a real API, format, scientific quantity, language rule, or exact behavior; explain it where needed instead of replacing it with vague wording. State concrete actions directly—for example, say that a function “reads an environment variable” rather than that it “provides an environment-access boundary.”

Every maintained C++ header and source has one file-level Doxygen block after its ownership header and before includes or `#pragma once`. A header brief explains the public interface, exposed types and usage contract. Its corresponding `.cpp` brief explains the implementation responsibilities, workflow, internal boundaries and implementation-specific assumptions or failures. Both are retained and written for their different audiences instead of repeating the same text. A header-only component covers its public contract and inline implementation in its one header brief; a source-only application or test describes its complete purpose and workflow in the source brief.

For C++, put a function's complete public Doxygen contract on its declaration, normally in the header, and do not repeat it above the out-of-line `.cpp` definition. The definition should contain ordinary comments only for implementation details that are not clear from the code. Functions defined only in a `.cpp`, including private helpers and command-line entry points, keep their complete Doxygen contract at the definition. Inline header definitions are documented once in place. This keeps generated API documentation complete without making declarations and definitions drift apart.

Descriptions follow each component's actual responsibilities. Small accessors need a short contract; event loops and orchestration functions need ordered stages. Names, units, ownership, configuration precedence and differences between preview and execution should be explicit. Comments must be updated when the implementation changes.

The shared palette values are defined only in `src/launcher/environment/set_colors.csh`. `src/lund-generation/core/support/environment.h` is the only maintained C++ color source: it maps the inherited `*_COLOR` values onto semantic constants. Other C++ files select those constants and must not contain literal ANSI escape definitions or fallback palettes.

External and archived files are excluded from edits: `legacy/`, `src/lund-generation/external/targets.h`, `src/slurm-submission/external/submit_GEMC_sample.sh`, and every file recursively under `config/detector/`. The documentation convention does not authorize changes to those files. Maintained test adapters can explain how they read external sources and create isolated reference fixtures. Repository instructions are recorded in `AGENTS.md`.

## Build and JSON configuration

Maintained `CMakeLists.txt` files use comment-based description, purpose, workflow, input/output and failure notes, plus named `# region` / `# endregion` sections. The comments explain target dependencies, optional workflows, generated files and installation boundaries without changing CMake commands.

Strict JSON does not support comments. Runtime profiles keep their existing schemas and have adjacent `.json.md` field references. Do not add invented `_comment` keys to readers that reject unknown settings. VS Code's JSONC files support inline comment banners and regions despite their `.json` filenames.

| Configuration | Explanation |
| --- | --- |
| `config/run.json` | [Launcher build/test defaults and precedence](../../config/run.json.md) |
| `.vscode/c_cpp_properties.json` | Inline comments explain compile-command-based editor configuration |
| `.vscode/settings.json` | Inline comments explain language associations, formatting, folding and highlighting |

Generated build-tree JSON and protected external/legacy configurations are excluded.

## Data objects and state

The same layers apply to structs, classes, enums, private implementation records, constants and configuration containers. An object description explains its purpose, producers/consumers, ownership and lifetime; member comments explain units, defaults, allowed values and relationships. Named object regions make nested definitions navigable. Passive records need a usage/lifecycle explanation rather than an invented algorithm. For example, `Particle` and `Event` document the generator-to-writer data contract, while diagnostic storage records explain ownership of detached ROOT histograms.

Every file under `config/detector/` is classified as external and read-only for the assistant, recursively and regardless of extension. This includes current and future files, not only recognized gcard/YAML resources.

The unified `src/slurm-submission/external/submit_GEMC_sample.sh` and its two archived source payloads are external code, including their monitoring modifications, and are excluded from routine edits. Skip them in routine source-documentation passes; their interface is described in [the GEMC payload guide](../submit-simulation/worker-reference.md).
