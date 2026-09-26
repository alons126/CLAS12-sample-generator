# Migration and compatibility options

The imported sources are retained under `legacy/` and recorded by the public [`legacy-v1.0.0` GitHub release tag](https://github.com/alons126/CLAS12-sample-generator/releases/tag/legacy-v1.0.0). Use the root build and supported CLIs; historical launch scripts perform site-specific operations and may clean/reset repositories. Detailed provenance is in the [launch-chain reference](legacy-workflows.md).

## Entry points

| Previous | Supported replacement |
| --- | --- |
| Sourced `Uniform-sample-generator/run.sh` → edited `CodeRun.cpp` | `uniform-lund-generator --config FILE --channel ... --output NEW_DIR` |
| `Uniform_sample_generator_e_tester.C` | `electron-tester-{2070,4029,5986}MeV.conf` |
| `GENIE_to_LUND_converter.csh` → ROOT macro | `event-generator-to-lund-converter --event-generator genie-gst --input ... --config ... --output ...` |
| Sourced `setup_and_submit_jobs.csh` → selected setup script | `source run.csh --workflow submit` → small sourced bridge → Python setup → one Slurm array per sample |
| Per-energy detector resources | `config/detector/Generation_files_*` |
| Current-directory-dependent output rewrites | Explicit output path |

## Legacy-compatible settings

The writer always uses the established whitespace, precision, and uniform per-file IDs. Particle masses now come directly from the external target source, including its nonzero electron and six-decimal proton values. Production sampling uses the documented electron/charged-hadron p/1-p mixtures and uniform neutron momentum; `hadron-momentum=fixed` preserves the optional neutron-only 1 GeV/c study. Set matching channel, beam energy, target geometry, A/Z, file counts and seeds; choose the same file prefix when needed by downstream tools.

`legacy-coderun.conf` and `legacy-genie-wrapper.conf` capture active reference launch settings. Their counts are production-sized; override `--events` for a small local comparison. The maintained uniform default remains 25,000 events per file, while the pinned upstream uniform generator and physical conversion currently use 10,000; the compatibility profile selects the upstream value explicitly.

The unified external worker has one output-naming contract: `mc_LUNDSTEM_torusFIELD.hipo` and `recon_LUNDSTEM_torusFIELD.hipo`. There is no maintained `--output-naming` mode.

## New requested sampling

The beam-specific production profiles select the requested modes directly; `sampled` remains an accepted CLI compatibility alias:

- 1e: half uniform p and half uniform 1/p from 0.7 GeV/c to beam momentum, retaining 5–40° flat theta.
- en: uniform p from zero to beam momentum and flat theta within the original 5–35° window.
- ep: half uniform p and half uniform 1/p from 0.3 GeV/c to beam momentum, retaining the original 5–45° flat-theta prescription.

All production channels keep the original flat-theta and azimuth prescriptions, and all electron–hadron modes keep the trigger-electron construction. See [sampling equations](../concepts/sampling-models.md) and the sample profiles.

## Diagnostics

Uniform generation writes one `lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root` file. It merges the former general and compatibility monitors, preserves the archived plot organization and rendering style, sets every vertex-z axis to −8–5 cm to cover the target positions of all RG-M targets[^sportes-2026-rgm][^rgm-analysis-note], and extends regional hadron notation for protons, neutrons, pip, and pim. Every uniform channel fills `MonitoringPlotsPath/` with PDF/PNG renderings of the same objects. Physical conversion creates no monitoring histograms. Uniform generation also prepares the `mchipo/` and `reconhipo/` directories for later simulation and reconstruction.

## Retained corrections

Physical conversion retains a remaining-input cutoff generalized to the configured `events-per-file` block used to align with submission `JOB_NEVENTS`, but corrects the premature mid-file stop: the cutoff is checked only before a follow-up file starts. Creation publishes a manifest only after success. Submission resolves array size and event limit from the completed manifest or explicit settings, validates inputs and replaces the selected simulation output directories. `run.csh` owns the intentional clean/reset/pull operation for the disposable ifarm checkout before invoking the workflow driver. Generation may also replace its fully resolved run directory. See the [SSH workflow](../submit-simulation/ifarm-environment.md).

Known behavior differences and detector-level limitations—including unknown historical random states—are listed in [scientific validation boundaries](../development/validation.md).

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
