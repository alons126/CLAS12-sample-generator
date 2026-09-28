# Physical event-generator input to LUND

```bash
build/debug/apps/event-generator-to-lund-converter \
  --event-generator genie-gst \
  --config config/samples/physical-lund-creation/genie-gst.conf \
  --input '/path/to/truth/gst*.root' \
  --output runs/genie-gst-example
```

`event-generator` defaults to `genie-gst`; the name identifies GENIE as the producer and GST ROOT as the input format. Other values are rejected until their adapter is implemented. Quote globs so ROOT receives the pattern. GENIE GST inputs must contain a tree named `gst`. `--target` names the nucleus/material; beam energy plus target selects the GEMC variation and matching vertex geometry. `--gemc-target-variation` overrides that choice for an exceptional configuration. The converter never guesses scientific metadata from input filenames.

When `tune = auto`, an input below `master-routine_validation_01-eScattering/` makes the converter look for `input_options.txt` in that directory's parent and read the value after the exact `TUNE` key. An explicit `--tune NAME` takes precedence. Remote inputs, other directory layouts, missing or unreadable metadata, and a missing or empty `TUNE` entry resolve to `unknown` without stopping conversion.

The default `--output-layout nested` writes LUND files below `OUTPUT/<target>/<event-generator>__<tune>/<Q2-cut>__<beam-MeV>MeV/lundfiles`. For example, C12 at 5.98636 GeV with tune `GEM21_11a_00_000` produces `OUTPUT/C12/genie-gst__GEM21_11a_00_000/Q2_0.40__5986MeV/lundfiles`. Inside that directory, an automatic LUND filename prefix is `<target>__<event-generator>[__<version>]__<tune>__<Q2-cut>__<beam-MeV>MeV`. The version group is included only when `event-generator-version` is not `unknown`; the manifest always records the resolved version, including `unknown`. `--output-layout metadata` writes one run-directory name with the same separator rule: `<GEMC-target-variation>__<event-generator>__<version>__<tune>__<Q2-cut>__<beam-MeV>MeV`. Double underscores separate metadata values; hyphens and decimal points remain valid inside one value, as in `genie-gst` and `Q2_0.40`.

Physical conversion creates empty `mchipo/` and `reconhipo/` directories beside `lundfiles/`, matching the uniform completed-run layout. It still creates no monitoring ROOT file or rendered monitoring plots.

Every resolved value remains stored separately in the manifest. C12 selects the small foil at 2 GeV, large foil at 4 GeV, and four foils at 6 GeV. Because the nested path intentionally does not include target variation, an exceptional run with the same target, generator, tune, Q² label, and beam must use a different output parent to avoid replacing the standard run. The run-15733 example does this while selecting the explicit small-foil variation at 4 GeV[^sportes-2026-rgm][^rgm-analysis-note]. GEMC version is selected later by simulation submission and is not a LUND-converter option.

## Required schema

| Branches | ROOT types |
| --- | --- |
| `qel`, `mec`, `res`, `dis` | `Bool_t` |
| `resid`, `nf` | `Int_t` |
| `pxl`, `pyl`, `pzl` | `Double_t` |
| `pdgf[nf]` | `Int_t` array |
| `pxf[nf]`, `pyf[nf]`, `pzf[nf]` | `Double_t` arrays |

Missing branches, wrong types, inconsistent array lengths, empty inputs and unsupported-only inputs produce errors. `El` and `Ef` are not required: output energy is calculated from momentum and the selected particle mass.

`pdgf`, `pxf`, `pyf`, and `pzf` are read through `TTreeReaderArray`. For every loaded entry, ROOT derives each array view's current length from the branch leaf-count metadata. Before any indexed access, conversion requires `nf >= 0`, `pdgf.GetSize() == nf`, and the three momentum-array sizes to equal `pdgf.GetSize()`. A mismatch aborts the run without a completion manifest. This establishes the proper length for every well-formed ROOT entry and rejects inconsistent metadata/data instead of imposing a fixed maximum. No software can promise correctness for a physically corrupted file or a defect inside ROOT itself; within ROOT's validated branch contract, the converter checks every available length before use.

## Retained physics conventions

- Write the scattered electron first.
- Retain protons, neutrons, charged pions and photons; skip other final-state species.
- Do not write neutral pions (PDG 111). GENIE production must decay each neutral pion upstream, before
  the GST files are produced, so that GST final-state truth contains the two photons that GEMC can
  transport and CLAS12 can detect. A residual PDG 111 entry is skipped; the converter does not invent
  a decay or replace it with photons because the required daughter four-momenta are absent.
- Give every particle in an event the same sampled vertex.
- Store `resid` in LUND header field 4.
- Store process code 1=QE, 2=MEC, 3=RES, 4=DIS in header field 10. The converter requires and supports only these four reactions. Events with none of these flags are skipped; supporting another reaction requires updating the required GST branches, process-code mapping, validation, and documentation. When multiple supported flags are true, use the listed priority order.
- Preserve the input entry index in header field 9.
- Apply no acceptance or Q² cuts. The old filename labels and disabled fiducial code were not active selection logic.

Field 10 is a process tag, **not a generator cross-section weight**. Do not interpret it as one downstream. Momentum is in GeV/c, mass in GeV/c², energy is in GeV, and vertex position is in cm. Supported PDG identifiers are declared with the particle record. Electron, proton, neutron, and charged-pion masses come from external `src/workflows/lund-creation/external/targets.h`; the photon mass is zero. See the [data contract](../concepts/lund-data-contract.md).

This neutral-pion contract follows the CLAS12 forward electromagnetic-calorimeter design: neutral
mesons are reconstructed from their two-photon decays, and the detector resolves the resulting photon
showers. See G. Asryan et al., [The CLAS12 forward electromagnetic
calorimeter](https://doi.org/10.1016/j.nima.2020.163425), especially the overview and detector
requirements.

## Splitting and completion

`events` is the maximum number of **written** events. Skipped processes do not count toward it. `events-per-file` defaults to 10,000, controls file rollover, and defines the physical-input submission cutoff block. Before an accepted event would start a follow-up LUND file, the converter computes the inclusive number of GST input entries beginning with that event. If fewer than `events-per-file` input entries remain, conversion stops without creating the follow-up file. The first file is always allowed, including when the complete input is shorter than one block.

Once a file starts, the cutoff is not evaluated again inside it. An exact-multiple final block is therefore completed instead of being interrupted after its second event. The cutoff is deliberately based on input entries rather than accepted events; unsupported reactions inside an allowed block can still make its LUND file shorter than `JOB_NEVENTS`. The manifest records exact scanned, written, and per-file counts; no successful manifest is published after an I/O or schema error.

The resolved run directory is recreated when it already exists, following the documented replacement lifecycle. Physical conversion writes the split LUND files and `lund-creation-log.json`; it creates no ROOT monitoring file, PDF, or PNG. Monitoring is a uniform-creation responsibility.

To support another input format without creating another workflow, follow [Adding another event-generator-to-LUND adapter](../development/adding-event-generator.md). Historical command mappings and compatibility profiles are isolated in the [migration guide](../history/migration.md) and [launch-chain reference](../history/legacy-workflows.md).

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
