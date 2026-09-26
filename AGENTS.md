# Git commit and publication authority

Never create or amend a Git commit, push a branch or tag, or otherwise publish repository changes unless the user explicitly requests that exact Git action in the current request. A request to edit, implement, fix, patch, validate, or "do it" authorizes working-tree changes only and must never be interpreted as permission to commit or push them. Permission from an earlier request does not carry forward to a later request. By default, leave completed changes uncommitted for the user to inspect and report that state clearly.

# Local-only test trees

Every `tests/` tree below `src/` is local development material. Keep these trees on the development workstation because they are still used for local validation, but keep them ignored and out of Git tracking, commits, published source archives, maintained code explanations, and project documentation. Never delete the local files merely because Git no longer tracks them. A committed build must configure and build successfully when none of these local test trees is present.

# Maintained source documentation

Document maintained C++ with file/class purpose and workflow descriptions, named separator banners, Doxygen function contracts, and `#pragma region` / `#pragma endregion` around meaningful sections. Explain algorithm stages, inputs, outputs, assumptions and failure behavior. Keep brief accessors concise and comments consistent with implementation.

Write code explanations in simple, direct terms that a newcomer can understand. Prefer common words and short sentences. Use technical terms only when they name an actual API, data format, scientific quantity, language rule, or behavior that would become less precise if simplified. Explain an unfamiliar technical term where it first matters, and do not use abstract wording when the concrete action can be stated directly.

Give every maintained C++ header and source file exactly one file-level Doxygen block near its beginning, after the ownership header and before includes or `#pragma once`. Include `@file`, a concise `@brief`, and the additional categories needed to explain that file's role. A header's file brief documents the public interface, exposed types and usage contract. Its matching `.cpp` brief documents that translation unit's implementation responsibilities, workflow, internal boundaries and implementation-specific assumptions or failure behavior. Keep both briefs: the presence of a header brief does not replace the source brief. Do not copy the same description between them. For a header-only component, describe both its public contract and inline implementation in its single header brief. For a source-only application, describe the complete file purpose and workflow in that source brief.

Place the complete Doxygen contract for a public or otherwise separately declared C++ function at its declaration, normally in the header. Do not repeat that contract above the out-of-line definition in a `.cpp` file. Use ordinary implementation comments there only when they explain a non-obvious algorithm, invariant, performance choice, or external boundary. A function defined only in a `.cpp` file, including a translation-unit-private helper or command-line entry point, must carry its complete Doxygen contract at that definition. An inline function defined in a header is documented once at that definition. Brief forwarding functions and accessors should have concise declaration contracts and at most one implementation comment explaining why the forwarding boundary exists.

Whenever maintained code changes, update its embedded explanations and every affected user/developer document in the same change. Documentation and code comments are part of the implementation contract, not optional follow-up work. A behavior change is incomplete when the described behavior, interface, assumptions, failure modes, or workflow remains stale.

Write maintained documentation and code explanations so the current project stands on its own. Explain what the maintained code does and why in terms of its present scientific and operational contract. Legacy sources are temporary development references that will eventually be removed; mention them only in explicitly historical, migration, or parity-validation material where the comparison itself matters. Do not make primary workflow documentation, API contracts, or code comments depend on readers having access to or knowledge of legacy code. Do not remove the legacy trees yet.

Apply this layered style to all maintained code objects, not just functions: classes, structs (including private/nested implementation records), enums, meaningful state groups, constants and configuration containers. Explain purpose, how objects are created/used, member meanings and units, ownership/lifetime, invariants and consumers. Use named banners/regions at meaningful object boundaries; keep trivial members concise and avoid inventing algorithms for passive data records.

Keep clear and consistent vertical spacing in every maintained code file. Use exactly one blank line between logical code blocks, such as initialization, validation, conditionals, loops, reporting, cleanup, return paths, named regions, and separate groups of related statements. Keep a comment directly attached to the block it explains. Do not use repeated blank lines, whitespace-only lines, or tab indentation to create visual separation. For maintained C++, apply this rule only where the repository formatter preserves the spacing.

When a code explanation is divided into named categories such as `Purpose:`, `Workflow:`, `Inputs:`, `Outputs:`, or `Failure:`, put each category name on its own line. Start its explanation on the following line, indented beneath the category name. Leave exactly one blank line between categories. Do not leave a blank line before the first category, except when it follows a C++ `@brief` or a Python docstring brief; in those cases, leave exactly one blank line after the brief. Apply this layout to docstrings and comment-based explanations where categories are used; keep the language's comment markers on comment lines. Do not add categories to brief comments that do not need them.

For a maintained command-line entry point or a wrapper that documents forwarded command-line arguments, include a named `CLI options:` category in its file-level explanation; a parenthetical owner may be added to the heading. List each relevant accepted option with its value, meaning, and default or required status where applicable, as in `src/launcher/workflow.py`. When a wrapper forwards a different entry point's options, name that owner and distinguish wrapper-owned controls from forwarded ones. Keep the list consistent with the actual parser and update it when the CLI changes. Internal helpers that are not user-facing entry points do not need an option inventory.

## Maintained C++ formatting

Apply these rules to maintained C++ source and header files. Do not apply them to protected external or archived sources. Unless a later instruction explicitly says otherwise, none of the code-format or ownership-header rules in this section apply to `.clang-format`, `.vscode/c_cpp_properties.json`, `.vscode/settings.json`, any `*.conf` file, or Git control files such as `.gitignore`, `.gitattributes`, and `.gitmodules`.

- Begin every maintained C++ source and header file with this ownership header, using the file's actual creation date:

  ```cpp
  //
  // Created by Alon Sportes on <creation date>.
  //
  ```

- Leave exactly one blank line before every `#pragma region` and after every `#pragma endregion`.
- Leave exactly one blank line after the file's include block.
- Leave exactly one blank line after namespace-usage declarations such as `using namespace std;`.
- Leave exactly one blank line before every named separator banner.
- Make every separator-banner line, including its label and repeated `-` characters, exactly 170 columns wide.
- Always enclose the body of every C++ `if`, `else if`, `else`, `for`, range-based `for`, `while`, and `do while` statement in braces, even when the body contains only one statement.

### Formatter-overriding exceptions

For maintained C++, the repository `.clang-format` output is authoritative wherever it conflicts with the manual formatting rules above. Do not repeatedly restore whitespace or line layouts that clang-format removes. Apply the current formatter contract as these explicit exceptions:

- Use Google as the base style, attached braces, four-space indentation, four-column tab width, and spaces instead of tab characters.
- Use the formatter's 190-column limit for ordinary code. The explicit 170-column requirement still applies to named separator-banner lines when clang-format leaves those comment lines unchanged.
- Short `if` statements, loops, and blocks may remain on one line when clang-format permits them. Do not expand them again merely to satisfy the manual layout rules.
- Braces that already exist must remain; clang-format's permission to keep a short statement on one line does not authorize manually deleting braces.
- Blank-line requirements around `#pragma region`, `#pragma endregion`, banners, includes, and namespace-usage declarations apply only where clang-format preserves those blank lines. The formatter may remove blank lines at the beginning or end of a block, and that formatted result is compliant.
- Run clang-format after manual C++ formatting changes and treat the resulting layout as the final code format.

Every maintained source-code file and maintained script in another language must begin with the analogous `Created by Alon Sportes on <creation date>.` ownership header using that language's comment syntax. When a script begins with an interpreter directive such as `#!/usr/bin/env python3` or `#!/bin/tcsh`, keep the shebang on the first line, leave exactly one blank line, and then place the ownership header. Use the actual creation date recorded in an existing header or recover it from repository history; never guess or silently substitute the modification date. Ownership headers are required only in source code and scripts. Do not add them to documentation, example-command lists, configuration or profile files regardless of extension, data files, generated outputs, or other non-code resources. These ownership-header requirements also do not apply to protected external or archived files; `.clang-format`; `.vscode/c_cpp_properties.json`; `.vscode/settings.json`; or Git control files such as `.gitignore`, `.gitattributes`, and `.gitmodules`, unless explicitly requested.

Use module/function docstrings and `# region` / `# endregion` comment markers for Python. Use description, purpose, workflow, inputs/outputs, usage and named comment regions for shell scripts. Keep shebangs first and preserve sourced-shell exit-status behavior.

Use the shared `src/launcher/presentation/print_success.csh` and `print_stop.csh` printers with the same lifecycle in every current and future workflow. A workflow coordinator prints the success artwork once, only after every requested stage or sample completes. On interruption or a handled failure, it prints the stop artwork once before the final diagnostic and returns the original nonzero status. Printer failure must never replace or hide the workflow result. Do not add workflow-specific copies of these printers.

For maintained Python, leave exactly one blank line after a function or method docstring before executable code. Separate a `for`, `while`, `if`/`elif`/`else`, `try`/`except`/`finally`, or `with` block from preceding and following unrelated statements with exactly one blank line; keep the clauses of one compound statement together. Apply the same spacing to distinct return, raise, assignment/definition, and function-call stages. Consecutive statements that form one logical block, such as related assignments, calls, or a guard and its immediate return or raise, stay together. Do not insert blank lines at the beginning or end of an indented suite or between a comment and the code it explains. Preserve Python's required indentation and run syntax checks after spacing edits.

Use `src/launcher/presentation/set_colors.csh` as the single definition of ANSI color values. `run.csh` sources it before any workflow diagnostic and exports the palette to Python, CMake, shell helpers, and C++ child processes. Use `src/workflows/support/environment.h` as the project-wide C++ interface to those inherited values. Other C++ files may select its semantic constants but must not define terminal escape sequences locally. Keep this header outside any one workflow because it is shared workflow infrastructure. Shell helpers that may run in a separate process source `set_colors.csh`; Python and CMake decode the inherited environment values. Do not duplicate ANSI values or define fallback palettes elsewhere.

Every maintained error diagnostic that the project prints before returning or exiting with failure must have exactly this visible form: `ERROR_COLOR + "Error: " + RESET_COLOR + message`. Every maintained warning diagnostic must have exactly this visible form: `WARNING_COLOR + "Warning: " + RESET_COLOR + message`. Use the shared semantic colors from `src/workflows/support/environment.h` in C++ and the inherited environment-variable palette from `src/launcher/presentation/set_colors.csh` in shell, Python, and CMake. Keep exception payloads and validation messages free of `Error:` and `Warning:`; the final printing boundary adds the prefix exactly once. Preserve diagnostics emitted by external commands, but follow them with a standardized project-owned error when their failure stops a maintained workflow.

# Project architecture and scope

The project currently implements two user-facing workflows:

1. **Create LUND files.** One unified LUND-creation workflow supports:
   - uniform, deliberately unphysical acceptance samples generated from configured random kinematics; and
   - physical samples converted from event-generator output, currently GENIE GST ROOT trees.

   Both paths must share configuration, target-vertex handling, LUND serialization, output naming, file splitting, monitoring, provenance, and completion behavior wherever their semantics are the same. Generator-specific adapters are responsible only for reading or producing particle/event content. Design the physical-input boundary so another event generator or input format can be added as a small adapter without duplicating the common LUND workflow.

   A physical converter copies supported truth-level event content into LUND; it does not run the event generator and must not invent kinematics absent from the input. A uniform generator samples configured kinematics and is not a physical interaction model. Preserve this distinction in code, naming, output metadata, monitoring, and documentation.

2. **Submit CLAS12 simulation jobs to ifarm.** This workflow submits Slurm array jobs that run GEMC followed by CLAS12 reconstruction (coatjava/recon-util) using existing LUND, GCARD, and YAML inputs. The project does not provide a user-facing local-simulation workflow. Preview/validation may run locally, but detector simulation execution belongs to ifarm Slurm jobs.

Keep these workflows separate: creating LUND files does not automatically submit simulation, and submission consumes already completed LUND output.

Design the source tree and shared infrastructure so later user-facing workflows can be added without placing their code inside an existing workflow or turning the launcher into one workflow's implementation. Expected downstream additions include, but are not limited to:

- **Create analysis NTuples.** A C++ workflow reads reconstructed HIPO files after the Slurm simulation/reconstruction jobs finish and writes ROOT files containing the four-momenta of final-state particles needed by analysis.
- **Create and skim reconstructed HIPO samples.** A downstream workflow creates new HIPO samples from completed reconstruction output and removes events that fail an explicit selection, such as events without a reconstructed electron.

Treat each added workflow as a peer under `src/workflows/`, with one clear entry point, configuration path, input contract, output contract, and ownership boundary. Create its directory only when implementation work begins; do not add empty placeholders for planned workflows. Add shared code only when at least two workflows have the same real requirement; keep workflow-specific physics, file formats, filtering rules, and execution commands inside the owning workflow. Do not add speculative generic orchestration merely because more workflows are expected.

At the user interface, uniform generation and physical-event conversion are `--source uniform` and `--source physical` modes of the single `create-lund` workflow, not separate top-level workflows. The installed executables are named `uniform-lund-creator` and `event-generator-to-lund-converter`. The physical executable accepts `--event-generator`, defaulting to the format-specific `genie-gst` adapter. Do not expose `--source genie`, `--source genie-gst`, or the retired `clas12-genie-to-lund` name. Likewise, the per-array-task GEMC/reconstruction runner is an internal worker of ifarm submission, not a third user-facing workflow. Maintained executables and scripts may remain separate internally when that keeps dependencies and code simple.

# Legacy design sources

Treat the two archived trees as independent historical sources that came from different Git repositories:

- `legacy/Uniform-sample-generator/` is a pinned Git submodule for the independent upstream repository and defines uniform 1e/ep/en generation, its manual `CodeRun.cpp` selection, random kinematics, output naming, monitoring, and ROOT launch chain. Treat the pinned commit as the current external reference; do not edit files inside the submodule from this repository.
- `legacy/GEMC-samples/` defines GENIE-GST-to-LUND conversion, detector configuration resources, and ifarm setup/submission for both physical and uniform LUND samples.

When unifying behavior, compare both implementations rather than assuming one archive is a later version of the other. Preserve scientifically meaningful behavior and common operational conventions. Consolidate duplicated code only after identifying real differences in input semantics, particle content, sampling, naming, monitoring, and submission variables.

## LUND workflow invariants and differences

Unify the mechanics of LUND creation without erasing the semantics of each source workflow. The common layer may own configuration validation, LUND record serialization, output-directory layout, file lifecycle, provenance, and summaries. Uniform monitoring remains inside `src/workflows/lund-creation/uniform-lund-creator/`; physical adapters create no monitoring plots. Each sample adapter must still define how events are obtained, which events and particles are retained, how header fields are populated, and how vertices and kinematics are produced.

Target geometry and target identity are related but distinct inputs. Use the protected external `targets.h` geometry implementation as the authoritative vertex source. A target-geometry key selects the spatial distribution, while nuclear `A` and `Z` are LUND header metadata; never infer one silently from the other. Sample exactly one interaction vertex for each written event and give that same vertex to every particle in the event.

Preserve these legacy target behaviors unless a maintained configuration explicitly selects a documented alternative:

- Production uniform 1e, ep, and en generation used the `Ar` geometry. The electron and nucleon in ep/en share the sampled vertex.
- The standalone uniform electron tester used the fixed point `(0, 0, -3 cm)` rather than sampling a target volume.
- GENIE conversion accepted the geometry key as an input, independently accepted `A` and `Z`, sampled one vertex for every retained GST event, and assigned it to the scattered electron and every retained final-state particle.
- The target helper uses its own deterministic `TRandom3(12345)` stream for vertices. Uniform kinematics used a separate `TRandom3(0)` stream. RNG ownership, seeds, draw order, and the separation of vertex and kinematic streams affect reproducibility and must be deliberate and documented.
- Maintained configuration accepts zero or nonzero kinematic and vertex seeds. Explain that ROOT treats `TRandom3(0)` as automatic, nonrepeatable seeding: leave it as an explicit user choice, record the configured zero in provenance, and never claim that a zero-seeded run can be replayed from that value alone.

The uniform adapter creates events rather than reading them. Its maintained channel contract is:

- Production 1e writes one electron with a 50/50 uniform-in-momentum/uniform-in-inverse-momentum mixture from 0.7 GeV/c to beam momentum, theta uniform from 5 to 40 degrees except for the 2.07052 GeV outbending profile whose minimum is 2 degrees, and phi uniform from -180 to 180 degrees. Explicit upstream-parity settings may restore uniform momentum from zero to beam.
- Expose `--channel 1e|eh|electron-tester`; `electron-tester` is the beam-momentum 5–40° angular scan and `eh` requires the independently validated `--hadron proton|neutron|pip|pim` and `--hadron-region FD|CD` selections. Resolve output labels to `epFD`, `enFD`, `epipFD`, `epimFD` and the corresponding CD forms.
- Electron--hadron samples write a beam-momentum trigger electron at 25 degrees whose phi is chosen near the opposite CLAS12 sector from the hadron. Retain that correlation for CD as a deliberate separation check even though it is not obligatory there. The electron tester always scans theta 5--40 degrees and full phi; it provides the rough estimate from which the 25-degree prescription was selected.
- FD theta is 5--45 degrees for protons and charged pions and 5--35 for neutrons. CD theta is 35--145 for nucleons and 35--140 for charged pions. Phi is always -180--180 degrees.
- Momentum minima are proton 0.3/0.2 GeV/c, charged pion 0.2/0.1 GeV/c, and neutron 0/0 GeV/c for FD/CD respectively. Charged hadrons use the 50/50 uniform-p/uniform-1/p mixture; neutrons use uniform p. Fixed 1 GeV/c is neutron-only in either region.
- Uniform production writes every generated event, has fixed particle multiplicity for the selected channel, historically writes `A=1`, `Z=1`, zero polarizations, beam PID 11, interaction count 1, and weight 1, and resets the event number in each output file. Any intentional change to these header semantics must be exposed and documented.

The GENIE adapter converts existing GST truth and must not resample its particle kinematics. It retains only QE, MEC, RES, and DIS events, writes the interaction code as 1, 2, 3, or 4 in the final header field, places GST `resid` in the legacy target-polarization header position, and uses the configured `A`, `Z`, and beam energy. It writes the scattered electron plus supported final-state particles with PDG IDs 2212, 2112, 211, -211, and 22; therefore multiplicity varies by event. Neutral pions must be decayed upstream during GENIE production so the GST truth contains their daughter photons; skip residual PDG 111 entries rather than copying them or inventing missing decay kinematics. Count and report scanned, rejected, and written events separately. The legacy fiducial-map query is commented out and must not be presented or applied as an active cut.

Preserve the LUND particle-record contract: particles are active, momenta are in GeV/c, masses are in GeV/c², energy is in GeV and is calculated from the mass shell, vertices are in centimeters, and the remaining status/parent fields are zero. Keep particle ordering stable: scattered electron first, followed by the selected uniform hadron or supported GST final-state particles in input order for GENIE. Define maintained PDG identifiers with the particle record and obtain electron, proton, neutron, and charged-pion masses from the protected `src/workflows/lund-creation/external/targets.h` source through a maintained read-only adapter. The photon mass remains exactly zero. Do not create a second maintained mass table.

File splitting and naming need a common interface but adapter-aware semantics. Uniform generation produces the requested number of generated events and numbers events from zero within each file. GENIE scans input until it reaches the accepted-event capacity, but after writing an accepted event it stops when fewer than one configured `events-per-file` block of input entries remains. This cutoff keeps physical-file production aligned with the per-array-task event count supplied as `JOB_NEVENTS`; it is based on remaining input entries rather than remaining accepted events and therefore can leave a short final file. Explain this exact behavior. Skipped events make input-entry counts differ from output-event counts. Keep recognizable uniform channel/beam names and physical target/tune/Q2/beam provenance in output names without deriving scientific metadata only from path substrings.

Monitoring belongs only to uniform generation. It covers generated electron/hadron momenta, angles, vertices, and channel correlations in one `<prefix>_monitoring_plots.root` file. Preserve the archived plot definitions and rendering style, extending hadron labels with `FD` or `CD` and supporting proton, neutron, pip, and pim. Physical conversion prints input, conversion, and completion summaries but creates no ROOT monitoring histograms or rendered monitoring plots.

Legacy hard-coded filesystem paths, hostname/current-directory heuristics, and metadata parsed from filenames are historical operational choices rather than physics requirements. Replace them with explicit validated configuration in maintained code. Preserve the legacy generators' output behavior: after fully resolving and validating the intended run directory, clearly report it, recursively remove an existing directory at that exact path, and recreate it. Guard against empty, root, checkout, or otherwise unsafe deletion targets. State this replacement behavior prominently in help text and user documentation.

For uniform samples, target geometry and nuclear metadata must remain configurable together without conflating them. Checked-in production profiles should use the physically consistent `A` and `Z` for their selected target; Ar profiles use `A=40`, `Z=18`. Support other targets through explicit profile/config values or a small validated target-metadata table, while retaining CLI overrides for unusual studies.

Physical-sample output directories must use explicit metadata rather than parse it from the input path. Use the recognizable structure `<target-or-GEMC-target-variation>__<event-generator>-<generator-version>__<tune>__<Q2-cut>__<beam-energy-MeV>_<GEMC-version>`, sanitizing each component for use as one directory name. Record the unsanitized metadata separately in the manifest. If a field does not apply, use a documented token such as `none`; do not omit fields silently.

The intended detector chain is: truth-level particles in LUND -> GEMC/Geant4 detector transport and response -> CLAS12 reconstruction with the selected YAML -> optional downstream data-reduction workflows. LUND is the interface between event preparation and detector simulation. GCARD controls the GEMC detector configuration; YAML controls reconstruction. NTuple creation, reconstructed-HIPO production/skimming, acceptance-map calculation, and physics analysis are downstream of the two currently implemented workflows. When a downstream stage becomes maintained here, keep it as a separate peer workflow with an explicit handoff from completed reconstruction output.

# Simplicity requirements

Prefer a direct, newcomer-readable call chain over generic orchestration. Each user-facing workflow should have one obvious entry point, one documented configuration path, and one clear output contract. Do not duplicate detector commands between Python and shell or create several wrappers with indistinguishable roles. If a protected legacy-derived payload owns GEMC and reconstruction commands, maintained code should validate inputs and submit that payload rather than reimplementing its command body.

Documentation must begin with the currently implemented workflows and show each exact call chain before implementation details. Add future workflows to that overview when they are implemented; do not document planned examples as existing commands. Explain which steps run locally and which run on ifarm, what each input controls, where outputs are written, which completed workflow output feeds the next stage, and which files a newcomer normally edits. Embedded explanations and external documentation must agree with actual behavior.

Use `docs/references.bib` as the maintained bibliography for citations in project documentation. Add or reuse stable BibTeX keys there when a documented scientific or technical claim needs a formal reference. In Markdown, cite sources with rendered footnote markers such as `claim[^source-key].` and place the full footnote definitions at the bottom of that same page, using the matching BibTeX entries. Put citation markers before the period, comma, semicolon, colon, question mark, or exclamation mark that closes the cited text. Do not expose raw LaTeX `\cite{...}` commands or direct readers to `references.bib` instead of giving the page its rendered citation and reference text.

Format repository file references in documentation as Markdown links or inline code so wiki publication can link them to the publishing branch. Prefer repository-relative paths when a basename is ambiguous. Format function references as inline code and use a qualified name when needed to identify one definition; the generated wiki must link each resolvable function reference to its current source line. Keep fenced commands and code examples unchanged and directly copyable.

# Local-to-ifarm synchronization

The user's operational model has two clones with different roles. Development and commits happen in the local VS Code/GitHub checkout. The ifarm checkout is a disposable execution mirror: sourcing `run.csh` intentionally updates it to the remote revision, discards server-side tracked changes, and removes untracked/generated files according to the documented exclusions before building, creating LUND files, or submitting jobs. Do not reinterpret this synchronization as accidental data loss or replace it with a clean-working-tree refusal.

Keep the destructive synchronization explicit and narrowly scoped. It must first resolve and verify the repository root, display the checkout path and operations, fail if it cannot identify the intended Git worktree/remote/branch, preserve documented server build/output exclusions, check each Git command, and propagate failure without continuing to build or submit. Never run cleanup relative to an unresolved caller directory. Documentation must tell newcomers that server-side edits are disposable and must be committed and pushed from the local clone before sourcing the launcher.

# Protected external and archived sources

The user explicitly forbids editing external files or legacy code. Do not modify anything under `legacy/`, `src/workflows/lund-creation/external/targets.h`, `src/workflows/slurm-submission/external/submit_GEMC_sample.sh`, or **any file anywhere under `config/detector/` (recursively, regardless of extension or provenance)**. Treat other identified third-party source snapshots as read-only as well. Do not format, annotate, rename, delete or replace these files. A geometry or payload change requiring replacement of a protected external file needs an explicit later user instruction superseding this restriction.

Maintained wrappers outside these paths may be documented. Local development tools may read protected inputs and create separate fixtures in temporary/build directories, but they must not overwrite the protected originals. Do not manually edit generated build outputs.

# Build and configuration documentation

Use comment-based purpose, workflow, input/output and failure descriptions plus named regions in maintained CMakeLists.txt files. Preserve CMake command behavior. Strict JSON must remain valid for its consumer: use supported descriptive metadata (such as CMake preset descriptions), and adjacent `.json.md` references when schemas reject comment keys. VS Code JSONC files may use inline comment banners and regions. Never insert comments or invented metadata into strict runtime JSON.

# External GEMC payloads

The two archived job scripts `legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` are external, including their monitoring modifications. Their unified, generator-independent adaptation `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` is also protected external code after the user-authorized initial adaptation. Do not edit, format, annotate or replace these files without explicit later authorization. Maintained Python coordinators may call the unified payload; detector-command implementation belongs to that payload.
