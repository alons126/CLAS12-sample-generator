# Physical event-generator input to LUND

```bash
build/debug/apps/clas12-generator-to-lund \
  --event-generator genie \
  --config config/samples/genie.conf \
  --input '/path/to/truth/gst*.root' \
  --output runs/genie-example
```

`event-generator` defaults to `genie`; other values are rejected until their adapter is implemented. Quote globs so ROOT receives the pattern. GENIE inputs must contain a tree named `gst`. Select `--rgm-target` to resolve target geometry, nuclear A/Z, and the GEMC target variation together; explicit overrides remain available. The converter never guesses scientific metadata from input filenames.

Physical runs use `<GEMC-target-variation>__<event-generator>-<version>__<tune>__<Q2-cut>__<beam-MeV>_GEMC-<version>` below the supplied output parent. For example, `C12-small`, GENIE 3.2.2, tune `GEM21_11a_00_000`, and GEMC 5.14 produce `rgm_fall2021_C_S__genie-3.2.2__GEM21_11a_00_000__Q2_0_40__5986MeV_GEMC-5.14`. Every component is also stored separately in the manifest.

## Required schema

| Branches | ROOT types |
| --- | --- |
| `qel`, `mec`, `res`, `dis` | `Bool_t` |
| `resid`, `nf` | `Int_t` |
| `pxl`, `pyl`, `pzl` | `Double_t` |
| `pdgf[nf]` | `Int_t` array |
| `pxf[nf]`, `pyf[nf]`, `pzf[nf]` | `Double_t` arrays |

Missing branches, wrong types, inconsistent array lengths, empty inputs and unsupported-only inputs produce errors. `El` and `Ef` are not required: output energy is calculated from momentum and the selected particle mass, as in the imported converter.

## Retained physics conventions

- Write the scattered electron first.
- Retain protons, neutrons, charged/neutral pions and photons; skip other final-state species.
- Give every particle in an event the same sampled vertex.
- Store `resid` in LUND header field 4.
- Store process code 1=QE, 2=MEC, 3=RES, 4=DIS in header field 10. When multiple flags are true, use that priority order. Events with none of these flags are skipped.
- Preserve the input entry index in header field 9.
- Apply no acceptance or Q² cuts. The old filename labels and disabled fiducial code were not active selection logic.

Field 10 is a legacy process tag, **not a generator cross-section weight**. Do not interpret it as one downstream. Momentum is in GeV/c, mass in GeV/c², energy is in GeV, and vertex position is in cm. Supported PDG identifiers and the single rounded mass table come from `src/support/constants.h`; the electron is approximated as massless. See the [data contract](data-contracts.md).

## Splitting and completion

`events` is the maximum number of **written** events. Skipped processes do not count toward it. `events-per-file` defaults to 10,000 for physical conversion and may be overridden; conversion retains a final partial file instead of stopping early when fewer than one full output file of input entries remains.

For six accepted events with `--events 6`, output contains one file with count 6. GEMC/reconstruction consume those exact counts from the manifest. A successfully published manifest records scanned and written counts; no successful manifest is published after an I/O or schema error.

The resolved metadata-named run directory is recreated when it already exists, matching the legacy lifecycle. Physical conversion writes the split LUND files and `lund-gen-log.json`; it creates no ROOT monitoring file, PDF, or PNG. Monitoring is a uniform-generation responsibility.

## Reproduce the legacy wrapper settings

Use `config/samples/legacy-genie-wrapper.conf` to reproduce the active C12 / 2.07052 GeV / small-foil settings in the archived csh wrapper. The [launch-chain reference](legacy-workflows.md) lists the former input-path convention and maps every launch stage to a current command.
