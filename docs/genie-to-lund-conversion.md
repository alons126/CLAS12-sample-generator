# GENIE GST to LUND conversion

```bash
build/debug/apps/clas12-genie-to-lund \
  --config config/samples/genie.conf \
  --input '/path/to/truth/gst*.root' \
  --output runs/genie-example
```

Quote globs so ROOT receives the pattern. Inputs must contain a tree named `gst`. Beam energy, target geometry and nuclear A/Z are explicit settings; the converter no longer guesses them from input filenames. The sample config uses Ar geometry and A=40/Z=18. Check these values against the actual input and selected detector card.

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

Field 10 is a legacy process tag, **not a generator cross-section weight**. Do not interpret it as one downstream. Momentum is in GeV, mass in GeV, vertex in cm. Shared masses are listed in `src/common/TargetGeometry.cpp`. The default `mass-convention=legacy` preserves the restored converter's 0.13957 GeV for all pion species, including pi-zero. `standard` explicitly selects different charged/neutral pion constants; see the [data contract](data-contracts.md).

## Splitting and completion

`files × events-per-file` is the maximum number of **written** events. Skipped processes do not count toward it. Conversion retains a final partial file instead of stopping early when fewer than 10,000 input entries remain.

For six accepted events with `--files 3 --events-per-file 4`, output contains two files with counts 4 and 2. GEMC/reconstruction consume those exact counts from the manifest. A successfully published manifest records scanned and written counts; no successful manifest is published after an I/O or schema error.

Use a new output directory for each conversion. `monitoring.root` contains per-PDG diagnostics for written particles. The additional `legacy_histograms.root` contains the original electron theta-versus-phi diagnostic, filled before process selection. `--render-plots true` adds PDF/PNG output. Rendering filenames/styles are standardized; histogram contents are tested against the archived converter.

## Reproduce the legacy wrapper settings

Use `config/samples/legacy-genie-wrapper.conf` to reproduce the active C12 / 2.07052 GeV / small-foil settings in the archived csh wrapper. The [launch-chain reference](legacy-workflows.md) lists the former input-path convention and maps every launch stage to a current command.
