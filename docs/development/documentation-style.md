# Source documentation conventions

Source explanations must describe what the code actually does. Tell contributors why a component exists, which work it performs, what inputs it reads, what it returns or writes, and what happens on failure.

## General writing

Refer to this work as the “repository,” the “code,” or the specific application or workflow, not the “project.” Use “repository” for files and checkouts, and “code” or a component name for behavior. Preserve exact API names, identifiers, third-party names, URLs, and quotations, including CMake's [`project()`](https://cmake.org/cmake/help/latest/command/project.html) command and `PROJECT_*` variables.

Use simple, direct language. Begin unfamiliar behavior with a concrete action and visible result. Keep exact technical terms when they name a real API, data format, scientific quantity, or language rule, and explain them where they first matter.

For example, say “deletes untracked files, then updates from Git,” not just “refreshes the checkout.” Say “writes the final JSON log only after output succeeds,” not just “publishes the completion boundary.” Name the actor, action, affected files, and result. Define necessary terms such as manifest, adapter, job array, and provenance before relying on them.

Use this style in the README, user and developer guides, configuration references, tutorial notes, and source explanations. State when checks and deletion happen, what failure leaves behind, and what the user should do next. Simplify the wording without changing scientific meanings, API names, options, units, numerical limits, or assumptions.

Name scientific objects and quantities explicitly, then explain them. Use “vertex position” for an event's position in the target and “vertex coordinates” for $V_x$, $V_y$, and $V_z$; do not replace those terms with “starting positions” or “target positions.” This code samples one vertex per written event and assigns its coordinates to every particle in that event. Physical target-cell and foil positions are separate geometry descriptions, not alternative names for the event vertex.

Do not describe repository code as “maintained” or make current behavior depend on readers knowing historical implementations. Keep historical comparisons outside the reader documentation. When the documentation needs to identify the historical code, direct readers only to the plain `legacy-code-archive` tag name. Do not link the tag or name archived checkout paths.

## C++

Every C++ header and source in this repository starts with one file-level Doxygen block. A header explains its public types and use contract; the matching source explains implementation workflow, internal boundaries, assumptions, and failure behavior. Do not copy the same description into both.

Put a declared function's complete contract at its declaration. Do not repeat it above the out-of-line definition. A function defined only in a source file is documented at that definition. Constructors and destructors begin their brief with `Constructor:` or `Destructor:`.

Document classes, structs, enums, configuration records, constants, ownership, units, invariants, and consumers in proportion to their complexity. Use named separator banners and `#pragma region` markers for meaningful sections. Run the repository formatter after C++ edits and treat its output as authoritative.

## Python, shell, and CMake

Python uses module/function docstrings and `# region` markers. Shell files keep the shebang first, then explain purpose, workflow, inputs, output, usage, and sourced-shell status behavior. CMake files use comment sections that explain targets, dependencies, generated files, options, and failures.

User-facing entry points list every owned CLI option with its value, meaning, and default or required state. Wrappers distinguish owned options from forwarded options.

## Runtime presentation

[`src/launcher/presentation/set_colors.csh`](../../src/launcher/presentation/set_colors.csh) is the only ANSI palette. C++ reads it through [`src/workflows/support/environment.h`](../../src/workflows/support/environment.h); Python, shell, and CMake read the inherited environment. Do not define fallback palettes elsewhere.

Errors from this code use one colored `Error:` prefix, and warnings use one colored `Warning:` prefix. Exception payloads remain prefix-free. Copyable commands are printed in multiline shell form with one option/value per continued line. The exception is `module` commands: print `module show gemc/<gemc-version>`, `module show coatjava/<coatjava-version>`, and `module list` on one line.

Every workflow prints shared success artwork once after the complete invocation succeeds. A handled failure prints one blank line, the final error, one blank line, and shared stop artwork once while preserving the original status.

## Citations

Repository Markdown uses GitHub footnotes with the marker immediately after the supported statement and before its closing punctuation. Copy complete citation details from the repository's internal bibliography; readers should not need to open that working file. The Wiki generator converts footnotes into numbered Wiki references and rejects missing, duplicate, or unused definitions.

## Reading paths and examples

Keep the user reading order in the getting-started index and the contributor reading order in the development index. The Wiki home points to these routes instead of repeating them. Each workflow index orders its own pages; the generated sidebar follows those links.

Use the shared [path notation](../getting-started/outputs.md#path-notation): `OUTPUT` is the parent passed to LUND creation, while `RUN` is the resolved run directory. Submission consumes `RUN/lundfiles/`. Use `/path/to/...` for replaceable example paths, and make consecutive commands use the output produced by the preceding command. Give independent studies separate output parents when their resolved run names could coincide.

Preserve exact CLI, configuration, and environment-variable names. Explain aliases where they cross an interface rather than renaming them in prose. Do not treat placeholders such as `<INDEX>` as shell variables.

Link every repository file, directory, and function reference in prose and tables with a repository-relative Markdown link, including [`run.csh`](../../run.csh), source files, profiles, and detector settings. Directory links point to the actual repository directory. Function and method links point to the source file and current declaration or definition line that explains the behavior; put the exact, qualified name in inline code inside the link label. Verify the path and line instead of inventing an anchor. Recheck these links when source changes move the relevant lines. Link third-party functions to their authoritative API documentation or source when a verified target is available.

Keep fenced commands, code examples, and diagrams unchanged during linking edits, and provide file, directory, and function links in the surrounding text. Generated output names, external input paths, and placeholders are not repository files; do not give them invented links. The Wiki builder converts repository links to the publishing fork's URLs and preserves explicit function links. Links must work in the repository Markdown as well as the Wiki.

After editing documentation, check local links and heading anchors, tables, code fences, inline markup, and footnotes. Build the Wiki and check its generated links and formatting too. During prose-only edits, verify that commands and scientific details stayed unchanged. Report only checks that actually ran. Leave changes unstaged and uncommitted unless that Git action was explicitly requested, and preserve any edits already staged by the user.

## Protected sources

These conventions do not authorize edits to external [`targets.h`](../../src/workflows/lund-creation/external/targets.h), the external submission worker, or detector resources. Their update procedure is documented in [external inputs](../concepts/external-inputs.md).
