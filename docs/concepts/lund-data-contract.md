# Event records, LUND output and provenance

## 1. In-memory representation

`Particle` stores PDG code, mass, a `TVector3` momentum and a `TVector3` vertex. `Event` stores a run/input event index, A/Z, beam energy, resonance metadata, weight/process code and an ordered particle vector. Uniform events contain one electron or an electron followed by one selected hadron. Converted events contain the scattered electron followed by retained GST particles in their input order. Retained physical species are protons, neutrons, charged pions and photons. Neutral pions must be decayed upstream into photons before GST production; residual PDG 111 entries are skipped rather than copied or decayed by the converter.

These types are defined in [Event.h](../../src/workflows/lund-creation/core/lund/Event.h). LUND serialization is centralized in [LundWriter.cpp](../../src/workflows/lund-creation/core/lund/LundWriter.cpp).

## 2. Units and conventions

Particle momenta are in GeV/c, masses are in GeV/c², beam and particle energies are in GeV, vertex coordinates are in centimeters, and configured sampling angles are in degrees. The writer uses natural units with c=1 and calculates particle energy as `sqrt(p²+m²)`. Uniform sampling uses ROOT's `TVector3` spherical-coordinate convention, and every particle in an event receives the same sampled interaction vertex. Written particles are active; reserved status, parent, and daughter fields are zero except for the fixed active-particle field described below.

Electron, proton, neutron, and charged-pion masses come from the external target source through the maintained adapter; the photon mass is exactly zero. Physical inputs must provide upstream-generated neutral-pion decay photons because the converter skips residual PDG 111 entries rather than inventing missing decay kinematics.

## 3. LUND header: ten fields

| Field | Uniform value | GENIE conversion value |
| --- | --- | --- |
| 1 | Number of written particles | Number of retained particles including electron |
| 2 | Configured A (default `Ar40` target resolves to 40) | Configured A |
| 3 | Configured Z (default `Ar40` target resolves to 18) | Configured Z |
| 4 | 0 | GST `resid` (historical resonance metadata use) |
| 5 | 0 | 0 |
| 6 | 11 (electron beam) | 11 |
| 7 | Configured beam energy, with output-mode rounding | Same |
| 8 | 1 | 1 |
| 9 | Per-file event index | Global input entry index, including skipped entries |
| 10 | 1 | QE=1, MEC=2, RES=3, DIS=4 |

The GENIE process tag and resonance metadata are historical application conventions, not a claim that field 10 is a physical cross-section weight. We produce LUND files following the format in the [GEMC LUND documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html); the table above describes this repository's actual output.

## 4. Particle record: fourteen fields

| Fields | Contents |
| --- | --- |
| 1 | One-based particle index |
| 2 | 0 (reserved placeholder) |
| 3 | 1 (active particle) |
| 4 | PDG code |
| 5–6 | 0, 0 (reserved parent/daughter placeholders) |
| 7–9 | p_x, p_y, p_z |
| 10 | Energy recomputed as sqrt(p²+m²) |
| 11 | Mass |
| 12–14 | Vertex x, y, z |

The writer rejects empty events and non-finite particle energy/vertex data. GENIE `El` and `Ef` are not used to override the mass-shell energy calculation.

## 5. Output precision and compatibility

The single maintained format uses established whitespace, five decimal places for particle momenta, energy, mass, and vertices, and per-file uniform numbering. Ordinary 1e and GENIE headers write beam energy with six decimals. Electron–hadron and angular-tester headers write it with one decimal (for example, 5.98636 is serialized as 6.0). Internal momentum calculations still use the full configured beam value.

Uniform prefixes are derived as `Uniform_sample_<resolved-label>_<beam-MeV>MeV`. `--prefix` remains an explicit override for a downstream naming requirement. Output directories are explicit and never inferred from the current machine.

## 6. Mass convention

Supported PDG identifiers are declared with the particle record in [`Event.h`](../../src/workflows/lund-creation/core/lund/Event.h). `particleMass()` delegates to the target-source adapter, whose implementation is the only maintained translation unit that includes external [`targets.h`](../../src/workflows/lund-creation/external/targets.h). Electron, proton, neutron, and charged-pion values are read from that source without duplication. The photon mass is exactly zero. The writer calculates energy from the same in-memory mass and serializes both energy and mass to five decimal places.

| Species (PDG) | LUND mass (GeV/c²) |
| --- | ---: |
| electron (11) | 0.00051 |
| proton (2212) | 0.93827 |
| neutron (2112) | 0.93957 |
| pip/pim (±211) | 0.13957 |
| photon (22) | 0 |

The table shows five-decimal serialized values. Internally, [`targets.h`](../../src/workflows/lund-creation/external/targets.h) supplies electron `0.000511` and proton `0.938272`, so mass-shell energy uses those source values before rounding.
Neutral-pion mass is deliberately absent from the maintained table because PDG 111 is not a supported LUND output species. CLAS12 reconstructs neutral pions from their two-photon decays, so physical GST input must already contain the daughter photons generated upstream.

## 7. File splitting and completion

Uniform generation writes exactly the requested `events` count. GENIE conversion writes up to that accepted-event capacity, with an additional submission cutoff tied to `events-per-file`. Before an accepted event would start a follow-up LUND file, conversion compares the inclusive GST input-entry count beginning with that entry against `events-per-file`; it stops without creating the file when the count is smaller. The first file is always allowed, and the cutoff is never reevaluated inside a file that has started. Because this is an input-entry test rather than an accepted-event test, unsupported reactions inside an allowed block can still make its LUND file short. `events-per-file` defaults to 10,000 for physical conversion and also controls normal rollover. Uniform generation defaults to 25,000 events per file. File numbering starts at 1; filenames are `lundfiles/PREFIX_INDEX.txt`. A file is opened only when an accepted event is available, and uniform event IDs restart from zero in each file.

Submission resolves manifest/config/CLI inputs into shell settings: `NUM_OF_JOBS` selects numbered LUND files and `JOB_NEVENTS` supplies the shared per-task event limit to GEMC and reconstruction. Physical conversion uses its `events-per-file` value as the input-tail cutoff block so generation and the intended per-task limit share one scale. The cutoff prevents a known-short raw-input tail from starting a follow-up file without interrupting an exact final block; unsupported reactions can still yield fewer written events than raw entries. The completed manifest supplies actual per-file counts automatically; explicit configuration supports inputs without a manifest.

The writer warns, removes and recreates an existing run directory before creation. It writes `lundfiles/lund-creation-monitoring/lund-creation-log.json.tmp` only after LUND output and any required uniform monitoring finish, then renames it to `lundfiles/lund-creation-monitoring/lund-creation-log.json`. Physical conversion has no monitoring stage. Failure leaves partial output for inspection without publishing a completed manifest; rerunning the same resolved output replaces those partial results.

Before output creation, the writer prints a setup report grouped into run limits, beam/target values, active source settings, and resolved output paths. It omits fixed serialization constants and settings unused by the selected channel. After the manifest is published, the completion report contains only generated/scanned events, written events, LUND file count, and completion status.

For an input shorter than `events-per-file`, the first file is allowed to consume the available input. For longer input, the rule is evaluated only when an accepted event would start a follow-up file. Exact-multiple blocks are completed. See [validation](../development/validation.md).

## 8. Manifest schema version 1

| Member | Type | Meaning |
| --- | --- | --- |
| `schema_version` | integer | Currently 1 |
| `workflow` | string | `uniform` or `physical`; physical generator identity is in `config.event-generator` |
| `version`, `revision`, `root_version` | strings | Project version, short configure-time Git description/dirty marker, ROOT version |
| `targets_sha256` | string | SHA-256 of the external [`targets.h`](../../src/workflows/lund-creation/external/targets.h) used at compilation |
| `git` | object | Configure-time repository URL, branch, commit subject/hash/date/author, porcelain status summary, nearest tag, detached-HEAD state, tracking branch and ahead/behind counts, and GitHub tree link |
| `scanned_events`, `written_events` | integers | Input scan count and output count |
| `config` | object of strings | Fully merged/resolved settings, including RNG/output/mass/sampling modes |
| `files` | array | Each element has relative `path` and integer `events` |

Local input patterns and output paths are resolved to absolute paths in configuration. Git fields describe the checkout when CMake configured the executable, not a later working-tree change made without rebuilding. The manifest does not hash or freeze original GST inputs; preserve them and the source checkout. A dirty status identifies modified paths but is not a complete source snapshot.

Before `sbatch` is called, execution writes `reconhipo/slurm-submission-log.json`. It contains the exact submission command, every resolved coordinator/worker parameter, runtime Git information, and SHA-256 hashes of the selected GCARD, reconstruction YAML, and external worker payload. The record proves what the coordinator handed to Slurm; it does not claim that an accepted array task completed, capture external databases or detector RNG state, or replace scheduler task logs.
