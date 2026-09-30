# LUND data contract and provenance

The common record and writer separate source-specific event logic from the [LUND format consumed by GEMC](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html).

## In-memory records

`Particle` stores a PDG identifier, mass, three-momentum, and vertex. `Event` stores an event ID, A, Z, beam energy, two source-specific header values, and an ordered particle vector.

Uniform events contain one electron or an electron followed by one hadron. Physical GENIE events contain the scattered electron followed by supported GST particles in input order. Every particle in one event has the same vertex.

Electron, proton, neutron, and charged-pion masses come through `TargetGeometry` from external `targets.h`; photon mass is exactly zero. The writer calculates energy as sqrt(p²+m²).

## Ten-field event header

| Field | Uniform | GENIE GST conversion |
| --- | --- | --- |
| 1 | Particle count | Retained particle count, including electron |
| 2 | Configured A | Configured A |
| 3 | Configured Z | Configured Z |
| 4 | 0 | GST `resid` |
| 5 | 0 | 0 |
| 6 | 11 | 11 |
| 7 | Beam energy | Beam energy |
| 8 | 1 | 1 |
| 9 | Zero-based generated-event ID | Zero-based GST entry index |
| 10 | 1 | QE=1, MEC=2, RES=3, DIS=4 |

Field 10 is a process tag for converted GENIE input, not an event weight or cross section.

Every header uses this exact format:

```text
%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %d \t %.2f \n
```

Fields 4, 5, and 7 therefore have six digits after the decimal point; field 10 has two. A 5.98636 GeV beam is written as `5.986360`.

## Fourteen-field particle record

| Field | Meaning |
| --- | --- |
| 1 | One-based particle index |
| 2 | 0, reserved |
| 3 | 1, active particle |
| 4 | PDG identifier |
| 5–6 | 0, 0, reserved parent/status fields |
| 7–9 | px, py, pz in GeV/c |
| 10 | Calculated energy in GeV |
| 11 | Mass in GeV/c² |
| 12–14 | Vx, Vy, Vz in cm |

Momentum, energy, mass, and vertex values have five digits after the decimal point. The writer rejects empty events and non-finite energy or vertex data.

## Supported species and order

Supported physical output species are electron (11), photon (22), charged pions (±211), neutron (2112), and proton (2212). The scattered electron is first. Supported final-state particles retain GST order.

Neutral pions are not written. The input production must decay them upstream so their photons exist in GST. The converter skips a residual PDG 111 instead of inventing daughter momenta.

The serialized masses are:

| Species | Mass in GeV/c² |
| --- | ---: |
| electron | 0.00051 |
| proton | 0.93827 |
| neutron | 0.93957 |
| $\pi^{+}$ / $\pi^{-}$ | 0.13957 |
| photon | 0.00000 |

These are the five-decimal LUND values. Energy is calculated before serialization from the source precision in `targets.h` (for example, electron 0.000511 and proton 0.938272); photon mass is exactly zero.

## Splitting and file names

LUND filenames are `lundfiles/<PREFIX>_<INDEX>.txt`, with one-based file indexes. A file opens only when an event is ready, so a successful run has no empty rollover file. Uniform event IDs remain continuous across files.

The writer rotates at `events-per-file`. Physical conversion also uses that number for its follow-up-file input-tail rule, as defined in the [physical guide](../create-lund/physical.md). Actual per-file counts, including a short final file, are recorded in the manifest.

## Completion manifest

After all required output succeeds, the writer closes the LUND stream, writes `lund-creation-log.json.tmp`, and atomically renames it to `lund-creation-log.json`. The final name marks completion.

Schema version 1 contains:

| Member | Meaning |
| --- | --- |
| `schema_version` | Manifest schema, currently 1 |
| `workflow` | `uniform` or `physical` |
| `version`, `revision`, `root_version` | Project version, configure-time Git description, and ROOT version |
| `targets_sha256` | Hash of the compiled external target header |
| `git` | Configure-time repository, branch, commit, status, tag, and tracking details |
| `scanned_events`, `written_events` | Source entries examined and events written |
| `config` | Complete resolved settings as strings |
| `files` | Relative LUND paths and event counts in creation order |

The `git` object includes the repository, branch, commit message, complete commit hash, commit date and author, working-tree status summary, nearest tag, detached-HEAD state, tracking branch, ahead/behind counts, and an exact-commit GitHub source link when the repository address permits one.

For uniform creation, scanned and written counts are equal. For physical conversion, their difference includes skipped interactions and may also reflect where conversion stopped. The manifest does not hash the original GST input or capture a dirty checkout as a restorable source snapshot. Preserve source input, checkout revision, and campaign records separately.

Git fields describe the checkout when CMake configured the executable. Reusing a build after changing the working tree does not update them; rebuild when provenance must reflect new source.

## Submission record

After `sbatch` accepts an array, the separate `reconhipo/slurm-submission-log.json` records the returned job ID, exact command, resolved worker environment, runtime Git state, and hashes of the GCARD, YAML, and worker payload. It records the handoff to Slurm, not task completion, detector database state, or scientific validity.
