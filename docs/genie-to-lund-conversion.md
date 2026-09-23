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

Missing branches, wrong types, inconsistent array lengths, empty inputs and unsupported-only inputs produce errors. `El` and `Ef` are not required: output energy is calculated from momentum and the selected particle mass.

`pdgf`, `pxf`, `pyf`, and `pzf` are read through `TTreeReaderArray`. For every loaded entry, ROOT derives each array view's current length from the branch leaf-count metadata. Before any indexed access, conversion requires `nf >= 0`, `pdgf.GetSize() == nf`, and the three momentum-array sizes to equal `pdgf.GetSize()`. A mismatch aborts the run without a completion manifest. This establishes the proper length for every well-formed ROOT entry and rejects inconsistent metadata/data instead of imposing a fixed maximum. No software can promise correctness for a physically corrupted file or a defect inside ROOT itself; within ROOT's validated branch contract, the converter checks every available length before use. Integration tests exercise 300 supported particles and an intentionally mismatched array.

## Retained physics conventions

- Write the scattered electron first.
- Retain protons, neutrons, charged pions and photons; skip other final-state species.
- Do not write neutral pions (PDG 111). GENIE production must decay each neutral pion upstream, before
  the GST files are produced, so that GST final-state truth contains the two photons that GEMC can
  transport and CLAS12 can detect. A residual PDG 111 entry is skipped; the converter does not invent
  a decay or replace it with photons because the required daughter four-momenta are absent.
- Give every particle in an event the same sampled vertex.
- Store `resid` in LUND header field 4.
- Store process code 1=QE, 2=MEC, 3=RES, 4=DIS in header field 10. The converter requires and supports only these four reactions. Events with none of these flags are skipped; supporting another reaction requires updating the required GST branches, process-code mapping, validation, documentation, and tests. When multiple supported flags are true, use the listed priority order.
- Preserve the input entry index in header field 9.
- Apply no acceptance or Q² cuts. The old filename labels and disabled fiducial code were not active selection logic.

Field 10 is a process tag, **not a generator cross-section weight**. Do not interpret it as one downstream. Momentum is in GeV/c, mass in GeV/c², energy is in GeV, and vertex position is in cm. Supported PDG identifiers are declared with the particle record. Electron, proton, neutron, and charged-pion masses come from protected `src/lund-generation/external/targets.h`; the photon mass is zero. See the [data contract](data-contracts.md).

This neutral-pion contract follows the CLAS12 forward electromagnetic-calorimeter design: neutral
mesons are reconstructed from their two-photon decays, and the detector resolves the resulting photon
showers. See G. Asryan et al., [The CLAS12 forward electromagnetic
calorimeter](https://doi.org/10.1016/j.nima.2020.163425), especially the overview and detector
requirements.

## Splitting and completion

`events` is the maximum number of **written** events. Skipped processes do not count toward it. `events-per-file` defaults to 10,000, controls file rollover, and defines the physical-input submission cutoff block. Before an accepted event would start a follow-up LUND file, the converter computes the inclusive number of GST input entries beginning with that event. If fewer than `events-per-file` input entries remain, conversion stops without creating the follow-up file. The first file is always allowed, including when the complete input is shorter than one block.

Once a file starts, the cutoff is not evaluated again inside it. An exact-multiple final block is therefore completed instead of being interrupted after its second event. The cutoff is deliberately based on input entries rather than accepted events; unsupported reactions inside an allowed block can still make its LUND file shorter than `JOB_NEVENTS`. The manifest records exact scanned, written, and per-file counts; no successful manifest is published after an I/O or schema error.

The resolved metadata-named run directory is recreated when it already exists, following the documented replacement lifecycle. Physical conversion writes the split LUND files and `lund-gen-log.json`; it creates no ROOT monitoring file, PDF, or PNG. Monitoring is a uniform-generation responsibility. Historical command mappings and compatibility profiles are isolated in the [migration guide](migration.md) and [launch-chain reference](legacy-workflows.md).
