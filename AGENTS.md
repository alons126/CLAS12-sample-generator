# Maintained source documentation

Document maintained C++ with file/class purpose and workflow descriptions, named separator banners, Doxygen function contracts, and `#pragma region` / `#pragma endregion` around meaningful sections. Explain algorithm stages, inputs, outputs, assumptions and failure behavior. Keep brief accessors concise and comments consistent with implementation.

Apply this layered style to all maintained code objects, not just functions: classes, structs (including private/nested implementation records), enums, meaningful state groups, constants and configuration containers. Explain purpose, how objects are created/used, member meanings and units, ownership/lifetime, invariants and consumers. Use named banners/regions at meaningful object boundaries; keep trivial members concise and avoid inventing algorithms for passive data records.

Use module/function docstrings and `# region` / `# endregion` comment markers for Python. Use description, purpose, workflow, inputs/outputs, usage and named comment regions for shell scripts. Keep shebangs first and preserve sourced-shell exit-status behavior. Do not invent author/date attribution.

# Project architecture and scope

Keep the project centered on exactly two user-facing workflows:

1. **Create LUND files.** One unified LUND-creation workflow supports:
   - uniform, deliberately unphysical acceptance samples generated from configured random kinematics; and
   - physical samples converted from event-generator output, currently GENIE GST ROOT trees.

   Both paths must share configuration, target-vertex handling, LUND serialization, output naming, file splitting, monitoring, provenance, and completion behavior wherever their semantics are the same. Generator-specific adapters are responsible only for reading or producing particle/event content. Design the physical-input boundary so another event generator or input format can be added as a small adapter without duplicating the common LUND workflow.

   A physical converter copies supported truth-level event content into LUND; it does not run the event generator and must not invent kinematics absent from the input. A uniform generator samples configured kinematics and is not a physical interaction model. Preserve this distinction in code, naming, output metadata, monitoring, and documentation.

2. **Submit CLAS12 simulation jobs to ifarm.** This workflow submits Slurm array jobs that run GEMC followed by CLAS12 reconstruction (coatjava/recon-util) using existing LUND, GCARD, and YAML inputs. The project does not provide a user-facing local-simulation workflow. Preview/validation may run locally, but detector simulation execution belongs to ifarm Slurm jobs.

Keep these workflows separate: creating LUND files does not automatically submit simulation, and submission consumes already completed LUND output. Avoid extra workflow modes, abstraction layers, or orchestration features unless they directly support one of these two responsibilities.

# Legacy design sources

Treat the two archived trees as independent historical sources that came from different Git repositories:

- `legacy/Uniform-sample-generator/` defines uniform 1e/ep/en generation, its manual `CodeRun.cpp` selection, random kinematics, output naming, monitoring, and ROOT launch chain.
- `legacy/GEMC-samples/` defines GENIE-GST-to-LUND conversion, detector configuration resources, and ifarm setup/submission for both physical and uniform LUND samples.

When unifying behavior, compare both implementations rather than assuming one archive is a later version of the other. Preserve scientifically meaningful behavior and common operational conventions. Consolidate duplicated code only after identifying real differences in input semantics, particle content, sampling, naming, monitoring, and submission variables.

The intended detector chain is: truth-level particles in LUND -> GEMC/Geant4 detector transport and response -> CLAS12 reconstruction with the selected YAML. LUND is the interface between event preparation and detector simulation. GCARD controls the GEMC detector configuration; YAML controls reconstruction. Acceptance-map calculation and physics analysis are downstream and outside these two workflows.

# Simplicity requirements

Prefer a direct, newcomer-readable call chain over generic orchestration. Each user-facing workflow should have one obvious entry point, one documented configuration path, and one clear output contract. Do not duplicate detector commands between Python and shell or create several wrappers with indistinguishable roles. If a protected legacy-derived payload owns GEMC and reconstruction commands, maintained code should validate inputs and submit that payload rather than reimplementing its command body.

Documentation must begin with the two workflows and show the exact call chain before implementation details. Explain which steps run locally and which run on ifarm, what each input controls, where outputs are written, and which files a newcomer normally edits. Embedded explanations and external documentation must agree with actual behavior.

# Protected external and archived sources

The user explicitly forbids editing external files or legacy code. Do not modify anything under `legacy/`, `src/common/external/targets.h`, `src/common/external/submit_GEMC_sample.sh`, or **any file anywhere under `config/detector/` (recursively, regardless of extension or provenance)**. Treat other identified third-party source snapshots as read-only as well. Do not format, annotate, rename, delete or replace these files. A geometry or payload change requiring replacement of a protected external file needs an explicit later user instruction superseding this restriction.

Maintained wrappers and test adapters outside these paths may be documented. Existing tests may read protected inputs and create separate fixtures in temporary/build directories; they must not overwrite the protected originals. Do not manually edit generated build outputs.

# Build and configuration documentation

Use comment-based purpose, workflow, input/output and failure descriptions plus named regions in maintained CMakeLists.txt files. Preserve CMake command behavior. Strict JSON must remain valid for its consumer: use supported descriptive metadata (such as CMake preset descriptions), and adjacent `.json.md` references when schemas reject comment keys. VS Code JSONC files may use inline comment banners and regions. Never insert comments or invented metadata into strict runtime JSON.

# External GEMC payloads

The two archived job scripts `legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` are external, including their monitoring modifications. Their unified, generator-independent adaptation `src/common/external/submit_GEMC_sample.sh` is also protected external code after the user-authorized initial adaptation. Do not edit, format, annotate or replace these files without explicit later authorization. Maintained Python coordinators may call the unified payload; detector-command implementation belongs to that payload.
