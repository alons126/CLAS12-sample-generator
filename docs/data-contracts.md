# Event records, LUND output and provenance

## 1. In-memory representation

`Particle` stores PDG code, mass, a `TVector3` momentum and a `TVector3` vertex. `Event` stores a run/input event index, A/Z, beam energy, resonance metadata, weight/process code and an ordered particle vector. Uniform events contain one electron or an electron followed by one selected hadron. Converted events contain the scattered electron followed by retained GST particles in their input order.

These types are defined in [Event.h](../src/lund/Event.h). LUND serialization is centralized in [LundWriter.cpp](../src/lund/LundWriter.cpp).

## 2. LUND header: ten fields

| Field | Uniform value | GENIE conversion value |
| --- | --- | --- |
| 1 | Number of written particles | Number of retained particles including electron |
| 2 | Configured A (bare CLI default 1) | Configured A |
| 3 | Configured Z (bare CLI default 1) | Configured Z |
| 4 | 0 | GST `resid` (historical resonance metadata use) |
| 5 | 0 | 0 |
| 6 | 11 (electron beam) | 11 |
| 7 | Configured beam energy, with output-mode rounding | Same |
| 8 | 1 | 1 |
| 9 | Per-file event index | Global input entry index, including skipped entries |
| 10 | 1 | QE=1, MEC=2, RES=3, DIS=4 |

The GENIE process tag and resonance metadata are historical application conventions, not a claim that field 10 is a physical cross-section weight. We produce LUND files following the format in the [GEMC LUND documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html); the table above describes this repository's actual output.

## 3. Particle record: fourteen fields

| Fields | Contents |
| --- | --- |
| 1 | One-based particle index |
| 2 | 0 (legacy placeholder) |
| 3 | 1 (active particle) |
| 4 | PDG code |
| 5–6 | 0, 0 (legacy parent/daughter placeholders) |
| 7–9 | p_x, p_y, p_z |
| 10 | Energy recomputed as sqrt(p²+m²) |
| 11 | Mass |
| 12–14 | Vertex x, y, z |

The writer rejects empty events and non-finite particle energy/vertex data. GENIE `El` and `Ef` are not used to override the mass-shell energy calculation.

## 4. Output precision and compatibility

The single maintained format matches the archived whitespace, five decimal places for particle momenta, energy, mass, and vertices, and per-file uniform numbering. Ordinary 1e and GENIE headers write beam energy with six decimals. Electron–hadron and angular-tester headers write it with one decimal, as in the archived format (for example, 5.98636 is serialized as 6.0). Internal momentum calculations still use the full configured beam value.

Uniform prefixes are derived as `Uniform_sample_<resolved-label>_<beam-MeV>MeV`. `--prefix` remains an explicit override for a downstream naming requirement. Output directories are explicit and never inferred from the current machine.

## 5. Mass convention

All maintained particle identities and masses come from [`src/support/constants.h`](../src/support/constants.h). The values below are based on the [Particle Data Group 2026](https://pdg.lbl.gov/2026/listings/particle_properties.html), converted to GeV/c² and rounded to the five decimal places stored by the LUND writer. The electron is deliberately approximated as massless.

| Species (PDG) | LUND mass (GeV/c²) |
| --- | ---: |
| electron (11) | 0 |
| proton (2212) | 0.93827 |
| neutron (2112) | 0.93957 |
| pip/pim (±211) | 0.13957 |
| pi0 (111) | 0.13498 |
| photon (22) | 0 |

`particleMass()` reads this one table. The stored value also controls the mass-shell energy calculated by the writer.

## 6. File splitting and completion

Uniform generation writes exactly the requested `events` count. GENIE conversion writes up to that capacity after process selection and keeps the final partial file. `events-per-file` controls rollover: the maintained uniform default is 25,000, retained from the earlier imported launcher, while the pinned upstream generator currently defaults to 10,000; physical conversion also defaults to 10,000. File numbering starts at 1; filenames are `lundfiles/PREFIX_INDEX.txt`. A file is opened only when an accepted event is available, and uniform event IDs restart from zero in each file.

For submission, the coordinator validates each manifest entry's positive `events` value and exports it as `JOB_NEVENTS`. The GEMC payload uses that exact count for both GEMC and reconstruction. Consequently, the 25,000-event uniform default, the 10,000-event physical default and final partial physical files are all processed without a separate submission-specific split size.

The writer warns, removes and recreates an existing run directory before generation. It writes `manifest.json.tmp` only after LUND and diagnostics finish, then renames it to `manifest.json`. Failure leaves partial output for inspection without publishing a completed manifest; rerunning the same resolved output replaces those partial results.

The archived converter's short-input and near-end early termination is intentionally not reproduced; see [validation](validation.md).

## 7. Manifest schema version 1

| Member | Type | Meaning |
| --- | --- | --- |
| `schema_version` | integer | Currently 1 |
| `workflow` | string | `uniform` or `physical`; physical generator identity is in `config.event-generator` |
| `version`, `revision`, `root_version` | strings | Project version, configure-time Git revision/dirty marker, ROOT version |
| `targets_sha256` | string | SHA-256 of the external targets.h used at compilation |
| `scanned_events`, `written_events` | integers | Input scan count and output count |
| `config` | object of strings | Fully merged/resolved settings, including RNG/output/mass/sampling modes |
| `files` | array | Each element has relative `path` and integer `events` |

Local input patterns and output paths are resolved to absolute paths in configuration. The manifest does not hash or freeze original GST inputs; preserve them and the source checkout. A dirty revision identifies a modified checkout but is not a complete source snapshot.

Simulation records at `simulation/INDEX.json` contain executed argument lists and gcard/reconstruction-YAML SHA-256 hashes, plus `payload_sha256` identifying the executed external Bash payload. They do not yet capture every environment variable, external database or detector RNG state. Those must be recorded separately for production provenance.
