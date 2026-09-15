# Migration and compatibility options

The imported sources are retained under `legacy/`. Use the root build and supported CLIs; historical launch scripts perform site-specific operations and may clean/reset repositories. Detailed provenance is in the [launch-chain reference](legacy-workflows.md).

## Entry points

| Previous | Supported replacement |
| --- | --- |
| Sourced `Uniform-sample-generator/run.sh` → edited `CodeRun.cpp` | `clas12-uniform --config FILE --channel ... --output NEW_DIR` |
| `Uniform_sample_generator_e_tester.C` | `electron-tester.conf` |
| `GENIE_to_LUND_converter.csh` → ROOT macro | `clas12-genie-to-lund --input ... --config ... --output ...` |
| Sourced `setup_and_submit_jobs.csh` → selected setup script | Common simulation runner or Slurm submitter consuming a manifest |
| Per-energy detector resources | `config/detector/Generation_files_*` |
| Current-directory-dependent output rewrites | Explicit output path |

## Legacy-compatible settings

The default `lund-format=legacy` restores historical whitespace, precision and uniform per-file IDs. The default `mass-convention=legacy` restores the archived pion values. `nucleon-momentum=fixed` preserves the 1 GeV mode. Set matching channel, beam energy, target geometry, A/Z, file counts and seeds; choose the same file prefix when needed by downstream tools.

`legacy-coderun.conf` and `legacy-genie-wrapper.conf` capture the active archived launch settings. Their counts are production-sized; override `--events` for local tests. Output splitting is automatic at 10,000 events per file.

The earlier refactor's output remains available through `--lund-format precise`, `--mass-convention standard`, and runner `--output-naming indexed`. Default runner filenames now follow the legacy `mc_LUNDSTEM_torusFIELD.hipo` and `recon_LUNDSTEM_torusFIELD.hipo` convention.

## New requested sampling

Disable fixed nucleon momentum with `--nucleon-momentum sampled`:

- en: uniform p and uniform solid angle within the original 5–35° theta window.
- ep: half uniform p and half uniform 1/p, retaining the original 5–45° flat-theta prescription.

Both keep the original azimuth and trigger-electron prescription. See [sampling equations](sampling-models.md) and the sampled config examples.

## Diagnostics

`monitoring.root` retains the common per-PDG diagnostics introduced by the refactor. `legacy_histograms.root` additionally restores the original uniform names/correlations and the original GENIE electron diagnostic. `--render-plots true` creates PDF/PNG products; plot styles and names are standardized. Numerical parity is checked by independent tests.

## Retained corrections

The software retains all accepted events in the final partial GENIE file instead of reproducing the archived early-termination bug. It propagates actual file counts to simulation, validates input/configuration, rejects existing outputs, and publishes a manifest only after success. The launcher updates Git only when explicitly requested or enabled in run settings, using a clean-checkout fast-forward pull. No supported command deletes a run directory. See the [SSH workflow](ssh-workflow.md).

Full parity scope and limitations—including unknown historical random states and untested detector execution—are listed in [validation](validation.md).
