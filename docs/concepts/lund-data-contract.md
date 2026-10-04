# LUND data contract and provenance

The common record and writer separate source-specific event logic from the [LUND format consumed by GEMC](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html).

## In-memory records

`Particle` stores a PDG identifier, mass, three-momentum, and vertex. `Event` stores an event ID, $A$, $Z$, beam energy, two source-specific header values, and an ordered particle vector.

Uniform events contain one electron or an electron followed by one hadron. Physical GENIE events contain the scattered electron followed by supported GST particles in input order. Every particle in one event has the same vertex.

Electron, proton, neutron, and charged-pion masses come through `TargetGeometry` from external `targets.h`; photon mass is exactly zero. The writer calculates energy as $E=\sqrt{P^2+m^2}$, using natural units ($c=1$) for the numerical calculation.

## Ten-field event header

| Field | Uniform | GENIE GST conversion |
| --- | --- | --- |
| 1 | Particle count | Retained particle count, including electron |
| 2 | Configured $A$ | Configured $A$ |
| 3 | Configured $Z$ | Configured $Z$ |
| 4 | 0 | GST `resid` |
| 5 | 0 | 0 |
| 6 | 11 | 11 |
| 7 | Beam energy | Beam energy |
| 8 | RG-M convention: 1 | RG-M convention: 1 |
| 9 | Zero-based generated-event ID | Zero-based GST entry index |
| 10 | 1 | QE=1, MEC=2, RES=3, DIS=4 |

For GENIE GST conversion, field 4 is the LUND slot otherwise used for target polarization. Storing `resid` there follows the pinned RG-M GENIE-to-LUND converter, in which `RES_ID` replaced the earlier `targP` polarization value[^rgm-resid]. This is a compatibility mapping: `resid` remains the GENIE resonance identifier and is not interpreted as polarization.

Field 8 is always the literal value `1`. GEMC lists this user-defined column as the interacted-nucleon ID, but RG-M LUND-writing code widely uses `1` for uniform particles, GENIE events, and GCF events[^rgm-field-8]. This project preserves that RG-M convention. The value is not an interaction count and is not interpreted as a proton or neutron PDG identifier; GEMC retains this user-defined header value but does not use it for particle transport.

Field 5 is zero in GEMC's first-particle spin-$z$ slot. Field 6 identifies an electron beam (PDG 11), and field 7 records its energy in $\mathrm{GeV}$. This project uses field 9 for an event ID rather than the user-defined process ID in GEMC's example convention. It uses field 10 for a process tag in converted GENIE input rather than an event weight or cross section. Readers must use these project meanings instead of treating the header as a generic weighting prescription.

Field 9 must fit a signed 32-bit integer (at most 2147483647); the writer rejects larger IDs. Physical IDs can have gaps because unsupported input events are skipped.

Every header uses this exact format:

```text
%i \t %i \t %i \t %f \t %f \t %i \t %f \t %i \t %d \t %.2f \n
```

Fields 4, 5, and 7 therefore have six digits after the decimal point; field 10 has two. A $5.98636\,\mathrm{GeV}$ beam is written as `5.986360`.

## Fourteen-field particle record

| Field | Meaning |
| --- | --- |
| 1 | One-based particle index |
| 2 | 0 in GEMC's user-defined lifetime slot (nanoseconds) |
| 3 | 1, propagated particle |
| 4 | PDG identifier |
| 5 | 0 in the user-defined parent-index slot |
| 6 | 0 in the user-defined first-daughter-index slot |
| 7–9 | $P_x$, $P_y$, $P_z$ in $\mathrm{GeV}/c$ |
| 10 | Calculated energy $E$ in $\mathrm{GeV}$ |
| 11 | Mass $m$ in $\mathrm{GeV}/c^2$ |
| 12–14 | $V_x$, $V_y$, $V_z$ in $\mathrm{cm}$ |

Momentum, energy, mass, and vertex values have five digits after the decimal point. The writer rejects empty events and non-finite energy or vertex data.

### Auxiliary truth records (not yet tested)

For convenience, a LUND file can in principle include additional truth-level quantities encoded as particle records whose field 3 is `0`. GEMC propagates only records whose field 3 is `1` through Geant4. The pinned RG-M GCF-to-LUND converter demonstrates the `type = 0` convention for a non-propagated truth-level record[^rgm-type-zero].

This project has not yet tested this technique or verified how a field-3 value of `0` is preserved through GEMC output, HIPO, and reconstruction. The current `LundWriter` always writes `1` and provides no option for auxiliary records. Treat `type = 0` as an experimental extension: validate the resulting files and downstream banks before production use, and include every added record in the event-header particle count.

## Supported species and order

Supported physical output species are the electron $e^-$ (11), photon $\gamma$ (22), charged pions $\pi^\pm$ ($\pm211$), neutron $n$ (2112), and proton $p$ (2212). The scattered electron is first. Supported final-state particles retain GST order.

Neutral pions are not written. The input production must decay them upstream so their photons exist in GST. The physical LUND converter skips a residual PDG 111 instead of inventing daughter momenta. The [physical-conversion guide](../create-lund/physical.md#selection-and-translation) identifies the GENIE decay setting required during input production.

The serialized masses are:

| Species | $m$ ($\mathrm{GeV}/c^2$) |
| --- | ---: |
| $e^-$ | 0.00051 |
| $p$ | 0.93827 |
| $n$ | 0.93957 |
| $\pi^{+}$ / $\pi^{-}$ | 0.13957 |
| $\gamma$ | 0.00000 |

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

Git fields describe the checkout when CMake configured the executable. Reusing a build after changing the working tree does not update them automatically; reconfigure with CMake and rebuild when provenance must reflect new source.

## Submission record

After `sbatch` accepts an array, the separate `reconhipo/slurm-submission-log.json` records the returned job ID, exact command, resolved worker environment, runtime Git state, and hashes of the GCARD, YAML, and worker payload. It records the handoff to Slurm, not task completion, detector database state, or scientific validity.

[^rgm-resid]: RG-M, *GENIE to LUND converter*, pinned revision `d0d6050`, [source line defining `RES_ID` in place of `targP`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GENIE_to_LUND.C#L25).

[^rgm-field-8]: Examples at pinned RG-M revision `d0d6050`: [`p_LUND.cpp`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Ana/Q2_Ana/p_LUND.cpp#L93); [`iso_p_LUND.cpp`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Ana/Q2_Ana/iso_p_LUND.cpp#L88); [`GENIE_to_LUND.C`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GENIE_to_LUND.C#L27); [`GCF_to_LUND.C`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GCF_to_LUND.C#L27); Neutron Veto [`generate_neutrons.C`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/NeutronVeto/Simulation/generate_neutrons.C#L32) and [`generate_protons.C`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/NeutronVeto/Simulation/generate_protons.C#L32); and Simulation [`generate_neutrons.C`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/generate_neutrons.C#L33).

[^rgm-type-zero]: RG-M, *GCF to LUND converter*, pinned revision `d0d6050`, [non-propagated truth record with particle type `0`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GCF_to_LUND.C#L103).
