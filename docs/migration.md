# Migration and compatibility options

The imported sources are retained under `legacy/`. Use the root build and supported CLIs; historical launch scripts perform site-specific operations and may clean/reset repositories. Detailed provenance is in the [launch-chain reference](legacy-workflows.md).

## Entry points

| Previous | Supported replacement |
| --- | --- |
| Sourced `Uniform-sample-generator/run.sh` → edited `CodeRun.cpp` | `clas12-uniform --config FILE --channel ... --output NEW_DIR` |
| `Uniform_sample_generator_e_tester.C` | `electron-tester-{2070,4029,5986}MeV.conf` |
| `GENIE_to_LUND_converter.csh` → ROOT macro | `clas12-generator-to-lund --event-generator genie --input ... --config ... --output ...` |
| Sourced `setup_and_submit_jobs.csh` → selected setup script | Common simulation runner or Slurm submitter consuming a manifest |
| Per-energy detector resources | `config/detector/Generation_files_*` |
| Current-directory-dependent output rewrites | Explicit output path |

## Legacy-compatible settings

The writer always uses the established whitespace, precision, and uniform per-file IDs. It uses one rounded PDG-based mass table and approximates the electron as massless. Production sampling now uses the documented electron/charged-hadron p/1-p mixtures and uniform neutron momentum; `hadron-momentum=fixed` preserves the optional neutron-only 1 GeV/c study. Set matching channel, beam energy, target geometry, A/Z, file counts and seeds; choose the same file prefix when needed by downstream tools.

`legacy-coderun.conf` and `legacy-genie-wrapper.conf` capture active reference launch settings. Their counts are production-sized; override `--events` for local tests. The maintained uniform default remains 25,000 events per file, while the pinned upstream uniform generator and physical conversion currently use 10,000; the compatibility profile selects the upstream value explicitly.

Runner `--output-naming indexed` retains the earlier alternative naming mode. Default runner filenames follow the legacy `mc_LUNDSTEM_torusFIELD.hipo` and `recon_LUNDSTEM_torusFIELD.hipo` convention.

## New requested sampling

The beam-specific production profiles select the requested modes directly; `sampled` remains an accepted CLI compatibility alias:

- 1e: half uniform p and half uniform 1/p from 0.7 GeV/c to beam momentum, retaining 5–40° flat theta.
- en: uniform p from zero to beam momentum and flat theta within the original 5–35° window.
- ep: half uniform p and half uniform 1/p from 0.3 GeV/c to beam momentum, retaining the original 5–45° flat-theta prescription.

All production channels keep the original flat-theta and azimuth prescriptions, and all electron–hadron modes keep the trigger-electron construction. See [sampling equations](sampling-models.md) and the sample profiles.

## Diagnostics

Uniform generation writes one `lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root` file. It merges the former general and compatibility monitors, preserves the archived plot format, and extends regional hadron notation for protons, neutrons, pip, and pim. Every uniform channel fills `MonitoringPlotsPath/` with PDF/PNG renderings of the same objects. Physical conversion creates no monitoring histograms. Uniform generation also prepares the `mchipo/` and `reconhipo/` directories for later simulation and reconstruction.

## Retained corrections

The software retains all accepted events in the final partial GENIE file instead of reproducing the archived early-termination bug. It propagates actual file counts to simulation, validates input/configuration, warns before replacing existing outputs, and publishes a manifest only after success. `run.csh` owns the intentional clean/reset/pull operation for the disposable ifarm checkout before invoking the workflow driver. Generation may also replace its fully resolved run directory. See the [SSH workflow](ssh-workflow.md).

Full parity scope and limitations—including unknown historical random states and untested detector execution—are listed in [validation](validation.md).
