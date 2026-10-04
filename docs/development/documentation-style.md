# Source documentation conventions

Source explanations are part of the implementation contract. They should help a contributor understand what a component owns, why it exists, what enters and leaves it, and how it fails.

## General writing

Use simple, direct language. Begin unfamiliar behavior with a concrete action and visible result. Keep exact technical terms when they name a real API, data format, scientific quantity, or language rule, and explain them where they first matter.

Do not describe project code as “maintained” or make current behavior depend on readers knowing historical implementations. Keep historical comparisons outside the reader documentation. When the documentation needs to identify the historical code, direct readers only to the plain `legacy-code-archive` tag name. Do not link the tag or name archived checkout paths.

## C++

Every project header and source starts with one file-level Doxygen block. A header explains its public types and use contract; the matching source explains implementation workflow, internal boundaries, assumptions, and failure behavior. Do not copy the same description into both.

Put a declared function's complete contract at its declaration. Do not repeat it above the out-of-line definition. A function defined only in a source file is documented at that definition. Constructors and destructors begin their brief with `Constructor:` or `Destructor:`.

Document classes, structs, enums, configuration records, constants, ownership, units, invariants, and consumers in proportion to their complexity. Use named separator banners and `#pragma region` markers for meaningful sections. Run the repository formatter after C++ edits and treat its output as authoritative.

## Python, shell, and CMake

Python uses module/function docstrings and `# region` markers. Shell files keep the shebang first, then explain purpose, workflow, inputs, output, usage, and sourced-shell status behavior. CMake files use comment sections that explain targets, dependencies, generated files, options, and failures.

User-facing entry points list every owned CLI option with its value, meaning, and default or required state. Wrappers distinguish owned options from forwarded options.

## Runtime presentation

[`src/launcher/presentation/set_colors.csh`](../../src/launcher/presentation/set_colors.csh) is the only ANSI palette. C++ reads it through [`src/workflows/support/environment.h`](../../src/workflows/support/environment.h); Python, shell, and CMake read the inherited environment. Do not define fallback palettes elsewhere.

Project errors use one colored `Error:` prefix, and warnings use one colored `Warning:` prefix. Exception payloads remain prefix-free. Copyable commands are printed in multiline shell form with one option/value per continued line. The exception is `module` commands: print `module show gemc/<gemc-version>`, `module show coatjava/<coatjava-version>`, and `module list` on one line.

Every workflow prints shared success artwork once after the complete invocation succeeds. A handled failure prints one blank line, the final error, one blank line, and shared stop artwork once while preserving the original status.

## Citations

Repository Markdown uses GitHub footnotes with the marker immediately after the supported statement and before its closing punctuation. Copy complete citation details from the project's internal bibliography; readers should not need to open that working file. The Wiki generator converts footnotes into numbered Wiki references and rejects missing, duplicate, or unused definitions.

## Reading paths and examples

Keep the user reading order in the getting-started index and the contributor reading order in the development index. The Wiki home points to these routes instead of repeating them. Each workflow index orders its own pages; the generated sidebar follows those links.

Use the shared [path notation](../getting-started/outputs.md#path-notation): `OUTPUT` is the parent passed to LUND creation, while `RUN` is the resolved run directory. Submission consumes `RUN/lundfiles/`. Use `/path/to/...` for replaceable example paths, and make consecutive commands use the output produced by the preceding command. Give independent studies separate output parents when their resolved run names could coincide.

Preserve exact CLI, configuration, and environment-variable names. Explain aliases where they cross an interface rather than renaming them in prose. Do not treat placeholders such as `<INDEX>` as shell variables.

Link every repository file or directory reference in prose and tables with a repository-relative Markdown link, including [`run.csh`](../../run.csh). Keep fenced commands and diagrams unchanged, and provide file links in the surrounding text. Generated output names, external input paths, and placeholders are not repository files; do not give them invented links. The Wiki builder converts repository links to the publishing fork's URLs.

## Protected sources

These conventions do not authorize edits to external [`targets.h`](../../src/workflows/lund-creation/external/targets.h), the external submission worker, or detector resources. Their update procedure is documented in [external inputs](../concepts/external-inputs.md).
