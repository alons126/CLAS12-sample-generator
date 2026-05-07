# CLAS12-sample-generator

This repository contains CLAS12 GEMC sample-generation helpers for:

- GENIE-to-LUND conversion
- Uniform $(e,e')$, $(e,e'pFD)$, $(e,e'nFD)$ samples
- GEMC + reconstruction batch submission

## Layout

- `GEMC-samples/setup_and_submit_jobs.csh`
  Top-level entry point for setup and submission.
- `GEMC-samples/GENIE_to_LUND_converter.csh`
  Wrapper for the ROOT GENIE-to-LUND converter macro.
- `GEMC-samples/GENIE_to_LUND_converter/GENIE_to_LUND_converter.C`
  ROOT macro that converts GENIE `gst` trees into LUND files.
- `GEMC-samples/scripts/setup_and_submission_scripts/uniform_setup_and_submit.csh`
  Uniform sample setup and submission flow.
- `GEMC-samples/scripts/setup_and_submission_scripts/genie_job_submission_script.csh`
  GENIE sample setup and submission flow.
- `GEMC-samples/scripts/job_submission_scripts/*.sh`
  Slurm batch payload scripts.
- `GEMC-samples/Generation_files_*`
  Beam-energy-specific `gcard` and YAML inputs.

## Run Assumption

The current `csh` wrappers assume they are launched from inside `GEMC-samples/`.

Example:

```bash
cd GEMC-samples
source setup_and_submit_jobs.csh
```

Running the wrappers from the repository root will fail because they use relative `source ./scripts/...` paths.

## Current Verified State

The shell scripts parse cleanly with `csh -n` / `bash -n`, and the YAML files parse successfully.

The following runtime issues were verified in this checkout:

- The scripts depend on hardcoded lab paths under `/lustre24/...` and `/w/hallb-scshelf2102/...`.
- Those paths do not exist in the current local environment, so the submission scripts exit before doing useful work.
- `GENIE_to_LUND_converter/GENIE_to_LUND_converter.C` does not compile locally because it includes absolute external headers and sources that are not present here.
- The converter macro currently hardcodes its output path suffix to `_devGEMC_rgm_fall2021_Ar`, which is wrong for non-Ar targets.

## High-Risk Behavior

`GEMC-samples/scripts/update_script.csh` is destructive. It runs:

- `git clean -fxd`
- `git reset --hard`
- `git pull`

Do not run `GEMC-samples/setup_and_submit_jobs.csh` unless that behavior is intended.

## Known Script Caveats

- `genie_job_submission_script.csh` now correctly uses `unsetenv TARGET_VARIATION`.
- That block still has no final fallback `else`, so unsupported beam-energy / target combinations can leave `TARGET_VARIATION` unset and later produce a bad `GCARD_FILE` path.

## Inputs Present In This Repository

Beam-energy directories currently present:

- `GEMC-samples/Generation_files_2GeV`
- `GEMC-samples/Generation_files_4GeV`
- `GEMC-samples/Generation_files_6GeV`

Available config families currently present:

- `devGEMC5.12`
- `5.14`

## Suggested Next Fixes

- Make all wrapper scripts resolve paths relative to their own file location instead of the current shell directory.
- Replace hardcoded site-specific paths with overridable environment variables.
- Remove or guard destructive Git operations from the default entry point.
- Make the GENIE converter derive its output target variation instead of hardcoding `rgm_fall2021_Ar`.
