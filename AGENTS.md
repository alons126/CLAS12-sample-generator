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

At the user interface, uniform generation and physical-event conversion are source modes of the single LUND-creation workflow, not separate top-level workflows. Likewise, the per-array-task GEMC/reconstruction runner is an internal worker of ifarm submission, not a third user-facing workflow. Maintained executables and scripts may remain separate internally when that keeps dependencies and code simple.

# Legacy design sources

Treat the two archived trees as independent historical sources that came from different Git repositories:

- `legacy/Uniform-sample-generator/` defines uniform 1e/ep/en generation, its manual `CodeRun.cpp` selection, random kinematics, output naming, monitoring, and ROOT launch chain.
- `legacy/GEMC-samples/` defines GENIE-GST-to-LUND conversion, detector configuration resources, and ifarm setup/submission for both physical and uniform LUND samples.

When unifying behavior, compare both implementations rather than assuming one archive is a later version of the other. Preserve scientifically meaningful behavior and common operational conventions. Consolidate duplicated code only after identifying real differences in input semantics, particle content, sampling, naming, monitoring, and submission variables.

## LUND workflow invariants and differences

Unify the mechanics of LUND creation without erasing the semantics of each source workflow. The common layer may own configuration validation, LUND record serialization, output-directory layout, file lifecycle, monitoring lifecycle, provenance, and summaries. Each sample adapter must still define how events are obtained, which events and particles are retained, how header fields are populated, how vertices and kinematics are produced, and which monitoring quantities are meaningful.

Target geometry and target identity are related but distinct inputs. Use the protected external `targets.h` geometry implementation as the authoritative vertex source. A target-geometry key selects the spatial distribution, while nuclear `A` and `Z` are LUND header metadata; never infer one silently from the other. Sample exactly one interaction vertex for each written event and give that same vertex to every particle in the event.

Preserve these legacy target behaviors unless a maintained configuration explicitly selects a documented alternative:

- Production uniform 1e, ep, and en generation used the `Ar` geometry. The electron and nucleon in ep/en share the sampled vertex.
- The standalone uniform electron tester used the fixed point `(0, 0, -3 cm)` rather than sampling a target volume.
- GENIE conversion accepted the geometry key as an input, independently accepted `A` and `Z`, sampled one vertex for every retained GST event, and assigned it to the scattered electron and every retained final-state particle.
- The target helper uses its own deterministic `TRandom3(12345)` stream for vertices. Uniform kinematics used a separate `TRandom3(0)` stream. RNG ownership, seeds, draw order, and the separation of vertex and kinematic streams affect reproducibility and must be deliberate and documented.

The uniform adapter creates events rather than reading them. Its legacy channel contracts are:

- 1e writes one electron with momentum uniform from zero to the beam energy, theta uniform from 5 to 40 degrees, and phi uniform from -180 to 180 degrees.
- ep/en write a beam-momentum trigger electron at 25 degrees whose phi is chosen near the opposite CLAS12 sector from the nucleon. The nucleon retains the legacy angular acceptance: proton theta 5--45 degrees, neutron theta 5--35 degrees, and phi -180--180 degrees.
- Fixed 1 GeV nucleon momentum remains a selectable legacy mode. With that mode disabled, en directions are isotropic within the legacy neutron angular acceptance; ep momentum is a 50/50 mixture of uniform-in-momentum and uniform-in-inverse-momentum sampling over the configured positive bounds. Retain the legacy ep/en angular prescriptions.
- Uniform production writes every generated event, has fixed particle multiplicity for the selected channel, historically writes `A=1`, `Z=1`, zero polarizations, beam PID 11, interaction count 1, and weight 1, and resets the event number in each output file. Any intentional change to these header semantics must be exposed and documented.

The GENIE adapter converts existing GST truth and must not resample its particle kinematics. It retains only QE, MEC, RES, and DIS events, writes the interaction code as 1, 2, 3, or 4 in the final header field, places GST `resid` in the legacy target-polarization header position, and uses the configured `A`, `Z`, and beam energy. It writes the scattered electron plus supported final-state particles with PDG IDs 2212, 2112, 211, -211, 111, and 22; therefore multiplicity varies by event. Count and report scanned, rejected, and written events separately. The legacy fiducial-map query is commented out and must not be presented or applied as an active cut.

Preserve the LUND particle-record contract shared by the archives: particles are active, momenta and masses are in GeV units, energy is calculated from the mass shell, vertices are in centimeters, and the remaining legacy status/parent fields are zero. Keep particle ordering stable: scattered electron first, followed by the channel nucleon for uniform ep/en or supported GST final-state particles in input order for GENIE.

File splitting and naming need a common interface but adapter-aware semantics. Uniform generation historically produced the requested number of files with the requested number of generated events and numbered events from zero within each file. GENIE historically scanned input until it filled 10,000 accepted events per file or reached the requested file limit; skipped events make input-entry counts differ from output-event counts. Its early-stop handling for a final partial file is legacy behavior that may be corrected, but a correction must be explicit, tested, and documented rather than copied accidentally. Keep recognizable uniform channel/beam names and physical target/tune/Q2/beam provenance in output names without deriving scientific metadata only from path substrings.

Monitoring must also retain workflow-specific content. Uniform monitoring covers generated electron/nucleon momenta, angles, vertices, and channel correlations. GENIE conversion historically monitors the scattered-electron theta-versus-phi distribution and prints input, conversion, and completion summaries. Share rendering and reporting infrastructure where useful, while letting each adapter register its own scientifically relevant histograms and counters.

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

The user explicitly forbids editing external files or legacy code. Do not modify anything under `legacy/`, `src/common/external/targets.h`, `src/common/external/submit_GEMC_sample.sh`, or **any file anywhere under `config/detector/` (recursively, regardless of extension or provenance)**. Treat other identified third-party source snapshots as read-only as well. Do not format, annotate, rename, delete or replace these files. A geometry or payload change requiring replacement of a protected external file needs an explicit later user instruction superseding this restriction.

Maintained wrappers and test adapters outside these paths may be documented. Existing tests may read protected inputs and create separate fixtures in temporary/build directories; they must not overwrite the protected originals. Do not manually edit generated build outputs.

# Build and configuration documentation

Use comment-based purpose, workflow, input/output and failure descriptions plus named regions in maintained CMakeLists.txt files. Preserve CMake command behavior. Strict JSON must remain valid for its consumer: use supported descriptive metadata (such as CMake preset descriptions), and adjacent `.json.md` references when schemas reject comment keys. VS Code JSONC files may use inline comment banners and regions. Never insert comments or invented metadata into strict runtime JSON.

# External GEMC payloads

The two archived job scripts `legacy/GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` are external, including their monitoring modifications. Their unified, generator-independent adaptation `src/common/external/submit_GEMC_sample.sh` is also protected external code after the user-authorized initial adaptation. Do not edit, format, annotate or replace these files without explicit later authorization. Maintained Python coordinators may call the unified payload; detector-command implementation belongs to that payload.
