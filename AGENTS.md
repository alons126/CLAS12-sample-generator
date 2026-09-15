# Maintained source documentation

Document maintained C++ with file/class purpose and workflow descriptions, named separator banners, Doxygen function contracts, and `#pragma region` / `#pragma endregion` around meaningful sections. Explain algorithm stages, inputs, outputs, assumptions and failure behavior. Keep brief accessors concise and comments consistent with implementation.

Apply this layered style to all maintained code objects, not just functions: classes, structs (including private/nested implementation records), enums, meaningful state groups, constants and configuration containers. Explain purpose, how objects are created/used, member meanings and units, ownership/lifetime, invariants and consumers. Use named banners/regions at meaningful object boundaries; keep trivial members concise and avoid inventing algorithms for passive data records.

Use module/function docstrings and `# region` / `# endregion` comment markers for Python. Use description, purpose, workflow, inputs/outputs, usage and named comment regions for shell scripts. Keep shebangs first and preserve sourced-shell exit-status behavior. Do not invent author/date attribution.

# Protected external and archived sources

The user explicitly forbids editing external files or legacy code. Do not modify anything under `legacy/`, `src/common/targets.h`, or **any file anywhere under `config/detector/` (recursively, regardless of extension or provenance)**. Treat other identified third-party source snapshots as read-only as well. Do not format, annotate, rename, delete or replace these files. A geometry change requiring replacement of the protected header needs an explicit later user instruction superseding this restriction.

Maintained wrappers and test adapters outside these paths may be documented. Existing tests may read protected inputs and create separate fixtures in temporary/build directories; they must not overwrite the protected originals. Do not manually edit generated build outputs.

# Build and configuration documentation

Use comment-based purpose, workflow, input/output and failure descriptions plus named regions in maintained CMakeLists.txt files. Preserve CMake command behavior. Strict JSON must remain valid for its consumer: use supported descriptive metadata (such as CMake preset descriptions), and adjacent `.json.md` references when schemas reject comment keys. VS Code JSONC files may use inline comment banners and regions. Never insert comments or invented metadata into strict runtime JSON.

# External GEMC payloads

The two archived job scripts `legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` are external, including their monitoring modifications. Their unified, generator-independent adaptation `src/common/submit_GEMC_sample.sh` is also protected external code after the user-authorized initial adaptation. Do not edit, format, annotate or replace these files without explicit later authorization. Maintained Python coordinators may call the unified payload; detector-command implementation belongs to that payload.
