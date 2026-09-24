# Maintained source documentation

Document maintained C++ with file/class purpose and workflow descriptions, named separator banners, Doxygen function contracts, and `#pragma region` / `#pragma endregion` around meaningful sections. Explain algorithm stages, inputs, outputs, assumptions and failure behavior. Keep brief accessors concise and comments consistent with implementation.

Whenever maintained code changes, update its embedded explanations and every affected user/developer document in the same change. Documentation and code comments are part of the implementation contract, not optional follow-up work. Tests alone do not complete a behavior change when the described behavior, interface, assumptions, failure modes, or workflow has changed.

Write maintained documentation and code explanations so the current project stands on its own. Explain what the maintained code does and why in terms of its present scientific and operational contract. Legacy sources are temporary development references that will eventually be removed; mention them only in explicitly historical, migration, or parity-validation material where the comparison itself matters. Do not make primary workflow documentation, API contracts, or code comments depend on readers having access to or knowledge of legacy code. Do not remove the legacy trees yet.

Apply this layered style to all maintained code objects, not just functions: classes, structs (including private/nested implementation records), enums, meaningful state groups, constants and configuration containers. Explain purpose, how objects are created/used, member meanings and units, ownership/lifetime, invariants and consumers. Use named banners/regions at meaningful object boundaries; keep trivial members concise and avoid inventing algorithms for passive data records.

Keep clear and consistent vertical spacing in every maintained code file. Use exactly one blank line between logical code blocks, such as initialization, validation, conditionals, loops, reporting, cleanup, return paths, named regions, and separate groups of related statements. Keep a comment directly attached to the block it explains. Do not use repeated blank lines, whitespace-only lines, or tab indentation to create visual separation. For maintained C++, apply this rule only where the repository formatter preserves the spacing.

When a code explanation is divided into named categories such as `Purpose:`, `Workflow:`, `Inputs:`, `Outputs:`, or `Failure:`, put each category name on its own line. Start its explanation on the following line, indented beneath the category name. Leave exactly one blank line between categories. Do not leave a blank line before the first category, except when it follows a C++ `@brief` or a Python docstring brief; in those cases, leave exactly one blank line after the brief. Apply this layout to docstrings and comment-based explanations where categories are used; keep the language's comment markers on comment lines. Do not add categories to brief comments that do not need them.

For a maintained command-line entry point or a wrapper that documents forwarded command-line arguments, include a named `CLI options:` category in its file-level explanation; a parenthetical owner may be added to the heading. List each relevant accepted option with its value, meaning, and default or required status where applicable, as in `src/launcher/workflow.py`. When a wrapper forwards a different entry point's options, name that owner and distinguish wrapper-owned controls from forwarded ones. Keep the list consistent with the actual parser and update it when the CLI changes. Internal helpers and tests that are not user-facing entry points do not need an option inventory.

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

For maintained Python, leave exactly one blank line after a function or method docstring before executable code. Separate a `for`, `while`, `if`/`elif`/`else`, `try`/`except`/`finally`, or `with` block from preceding and following unrelated statements with exactly one blank line; keep the clauses of one compound statement together. Apply the same spacing to distinct return, raise, assignment/definition, and function-call stages. Consecutive statements that form one logical block, such as related assignments, calls, or a guard and its immediate return or raise, stay together. Do not insert blank lines at the beginning or end of an indented suite or between a comment and the code it explains. Preserve Python's required indentation and run syntax checks after spacing edits.

Use `src/lund-generation/core/support/environment.h` as the only source of ANSI color definitions in maintained C++. Other C++ files may select its semantic constants but must not define terminal escape sequences locally. Shell and Python launchers use their separate environment-variable palette because they cannot include a C++ header.

# Project architecture and scope

Keep the project centered on exactly two user-facing workflows:

1. **Create LUND files.** One unified LUND-creation workflow supports:
   - uniform, deliberately unphysical acceptance samples generated from configured random kinematics; and
   - physical samples converted from event-generator output, currently GENIE GST ROOT trees.

   Both paths must share configuration, target-vertex handling, LUND serialization, output naming, file splitting, monitoring, provenance, and completion behavior wherever their semantics are the same. Generator-specific adapters are responsible only for reading or producing particle/event content. Design the physical-input boundary so another event generator or input format can be added as a small adapter without duplicating the common LUND workflow.

   A physical converter copies supported truth-level event content into LUND; it does not run the event generator and must not invent kinematics absent from the input. A uniform generator samples configured kinematics and is not a physical interaction model. Preserve this distinction in code, naming, output metadata, monitoring, and documentation.

2. **Submit CLAS12 simulation jobs to ifarm.** This workflow submits Slurm array jobs that run GEMC followed by CLAS12 reconstruction (coatjava/recon-util) using existing LUND, GCARD, and YAML inputs. The project does not provide a user-facing local-simulation workflow. Preview/validation may run locally, but detector simulation execution belongs to ifarm Slurm jobs.

Keep these workflows separate: creating LUND files does not automatically submit simulation, and submission consumes already completed LUND output. Avoid extra workflow modes, abstraction layers, or orchestration features unless they directly support one of these two responsibilities.

At the user interface, uniform generation and physical-event conversion are `--source uniform` and `--source physical` modes of the single `create-lund` workflow, not separate top-level workflows. The physical executable is named `clas12-generator-to-lund`; it accepts `--event-generator`, defaulting to the format-specific `genie-gst` adapter. Do not expose `--source genie`, `--source genie-gst`, or the retired `clas12-genie-to-lund` name. Likewise, the per-array-task GEMC/reconstruction runner is an internal worker of ifarm submission, not a third user-facing workflow. Maintained executables and scripts may remain separate internally when that keeps dependencies and code simple.

# Legacy design sources

Treat the two archived trees as independent historical sources that came from different Git repositories:

- `legacy/Uniform-sample-generator/` is a pinned Git submodule for the independent upstream repository and defines uniform 1e/ep/en generation, its manual `CodeRun.cpp` selection, random kinematics, output naming, monitoring, and ROOT launch chain. Treat the pinned commit as the current external reference; do not edit files inside the submodule from this repository.
- `legacy/GEMC-samples/` defines GENIE-GST-to-LUND conversion, detector configuration resources, and ifarm setup/submission for both physical and uniform LUND samples.

When unifying behavior, compare both implementations rather than assuming one archive is a later version of the other. Preserve scientifically meaningful behavior and common operational conventions. Consolidate duplicated code only after identifying real differences in input semantics, particle content, sampling, naming, monitoring, and submission variables.

## LUND workflow invariants and differences

Unify the mechanics of LUND creation without erasing the semantics of each source workflow. The common layer may own configuration validation, LUND record serialization, output-directory layout, file lifecycle, provenance, and summaries. Uniform monitoring remains inside `src/lund-generation/uniform-to-lund-converter/`; physical adapters create no monitoring plots. Each sample adapter must still define how events are obtained, which events and particles are retained, how header fields are populated, and how vertices and kinematics are produced.

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

Preserve the LUND particle-record contract: particles are active, momenta are in GeV/c, masses are in GeV/c², energy is in GeV and is calculated from the mass shell, vertices are in centimeters, and the remaining status/parent fields are zero. Keep particle ordering stable: scattered electron first, followed by the selected uniform hadron or supported GST final-state particles in input order for GENIE. Define maintained PDG identifiers with the particle record and obtain electron, proton, neutron, and charged-pion masses from the protected `src/lund-generation/external/targets.h` source through a maintained read-only adapter. The photon mass remains exactly zero. Do not create a second maintained mass table.

File splitting and naming need a common interface but adapter-aware semantics. Uniform generation produces the requested number of generated events and numbers events from zero within each file. GENIE scans input until it reaches the accepted-event capacity, but after writing an accepted event it stops when fewer than one configured `events-per-file` block of input entries remains. This cutoff keeps physical-file production aligned with the per-array-task event count supplied as `JOB_NEVENTS`; it is based on remaining input entries rather than remaining accepted events and therefore can leave a short final file. Explain and test this exact behavior. Skipped events make input-entry counts differ from output-event counts. Keep recognizable uniform channel/beam names and physical target/tune/Q2/beam provenance in output names without deriving scientific metadata only from path substrings.

Monitoring belongs only to uniform generation. It covers generated electron/hadron momenta, angles, vertices, and channel correlations in one `<prefix>_monitoring_plots.root` file. Preserve the archived plot definitions and rendering style, extending hadron labels with `FD` or `CD` and supporting proton, neutron, pip, and pim. Physical conversion prints input, conversion, and completion summaries but creates no ROOT monitoring histograms or rendered monitoring plots.

Legacy hard-coded filesystem paths, hostname/current-directory heuristics, and metadata parsed from filenames are historical operational choices rather than physics requirements. Replace them with explicit validated configuration in maintained code. Preserve the legacy generators' output behavior: after fully resolving and validating the intended run directory, clearly report it, recursively remove an existing directory at that exact path, and recreate it. Guard against empty, root, checkout, or otherwise unsafe deletion targets. State this replacement behavior prominently in help text and user documentation.

For uniform samples, target geometry and nuclear metadata must remain configurable together without conflating them. Checked-in production profiles should use the physically consistent `A` and `Z` for their selected target; Ar profiles use `A=40`, `Z=18`. Support other targets through explicit profile/config values or a small validated target-metadata table, while retaining CLI overrides for unusual studies. Legacy byte-parity tests may supply the archived `A=1`, `Z=1` values explicitly when testing the old record format.

Physical-sample output directories must use explicit metadata rather than parse it from the input path. Use the recognizable structure `<target-or-GEMC-target-variation>__<event-generator>-<generator-version>__<tune>__<Q2-cut>__<beam-energy-MeV>_<GEMC-version>`, sanitizing each component for use as one directory name. Record the unsanitized metadata separately in the manifest. If a field does not apply, use a documented token such as `none`; do not omit fields silently.

The intended detector chain is: truth-level particles in LUND -> GEMC/Geant4 detector transport and response -> CLAS12 reconstruction with the selected YAML. LUND is the interface between event preparation and detector simulation. GCARD controls the GEMC detector configuration; YAML controls reconstruction. Acceptance-map calculation and physics analysis are downstream and outside these two workflows.

# Simplicity requirements

Prefer a direct, newcomer-readable call chain over generic orchestration. Each user-facing workflow should have one obvious entry point, one documented configuration path, and one clear output contract. Do not duplicate detector commands between Python and shell or create several wrappers with indistinguishable roles. If a protected legacy-derived payload owns GEMC and reconstruction commands, maintained code should validate inputs and submit that payload rather than reimplementing its command body.

Documentation must begin with the two workflows and show the exact call chain before implementation details. Explain which steps run locally and which run on ifarm, what each input controls, where outputs are written, and which files a newcomer normally edits. Embedded explanations and external documentation must agree with actual behavior.

# Local-to-ifarm synchronization

The user's operational model has two clones with different roles. Development and commits happen in the local VS Code/GitHub checkout. The ifarm checkout is a disposable execution mirror: sourcing `run.csh` intentionally updates it to the remote revision, discards server-side tracked changes, and removes untracked/generated files according to the documented exclusions before building, creating LUND files, or submitting jobs. Do not reinterpret this synchronization as accidental data loss or replace it with a clean-working-tree refusal.

Keep the destructive synchronization explicit and narrowly scoped. It must first resolve and verify the repository root, display the checkout path and operations, fail if it cannot identify the intended Git worktree/remote/branch, preserve documented server build/output exclusions, check each Git command, and propagate failure without continuing to build or submit. Never run cleanup relative to an unresolved caller directory. Documentation must tell newcomers that server-side edits are disposable and must be committed and pushed from the local clone before sourcing the launcher.

# Protected external and archived sources

The user explicitly forbids editing external files or legacy code. Do not modify anything under `legacy/`, `src/lund-generation/external/targets.h`, `src/slurm-submission/external/submit_GEMC_sample.sh`, or **any file anywhere under `config/detector/` (recursively, regardless of extension or provenance)**. Treat other identified third-party source snapshots as read-only as well. Do not format, annotate, rename, delete or replace these files. A geometry or payload change requiring replacement of a protected external file needs an explicit later user instruction superseding this restriction.

Maintained wrappers and test adapters outside these paths may be documented. Existing tests may read protected inputs and create separate fixtures in temporary/build directories; they must not overwrite the protected originals. Do not manually edit generated build outputs.

# Build and configuration documentation

Use comment-based purpose, workflow, input/output and failure descriptions plus named regions in maintained CMakeLists.txt files. Preserve CMake command behavior. Strict JSON must remain valid for its consumer: use supported descriptive metadata (such as CMake preset descriptions), and adjacent `.json.md` references when schemas reject comment keys. VS Code JSONC files may use inline comment banners and regions. Never insert comments or invented metadata into strict runtime JSON.

# External GEMC payloads

The two archived job scripts `legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` are external, including their monitoring modifications. Their unified, generator-independent adaptation `src/slurm-submission/external/submit_GEMC_sample.sh` is also protected external code after the user-authorized initial adaptation. Do not edit, format, annotate or replace these files without explicit later authorization. Maintained Python coordinators may call the unified payload; detector-command implementation belongs to that payload.
