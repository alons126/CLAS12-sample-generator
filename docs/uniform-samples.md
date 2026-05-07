# Uniform Samples

## Purpose

This workflow prepares and submits uniform CLAS12 samples for:

- `(e,e')`
- `(e,e'pFD)`
- `(e,e'nFD)`

Primary entry point:

- `GEMC-samples/scripts/setup_and_submission_scripts/uniform_setup_and_submit.csh`

Batch payload:

- `GEMC-samples/scripts/job_submission_scripts/submit_GEMC_uniform_sample.sh`

## How It Is Intended To Run

The script assumes it is launched from inside `GEMC-samples/`.

```bash
cd GEMC-samples
csh scripts/setup_and_submission_scripts/uniform_setup_and_submit.csh
```

The setup script:

- selects beam energies
- selects particle species
- resolves the beam-energy-specific `gcard` and YAML files
- prepares output directory structure
- submits Slurm arrays with `sbatch`

## Inputs

The workflow uses:

- `Generation_files_2GeV`
- `Generation_files_4GeV`
- `Generation_files_6GeV`

It resolves files such as:

- `rgm_fall2021-cv.yaml`
- `rgm_fall2021-ai_4Gev.yaml`
- `rgm_fall2021-ai_6Gev.yaml`
- target-specific `gcard` files

## Verified Issues In This Checkout

- The script depends on hardcoded site-specific directories such as `/lustre24/expphy/volatile/clas12/asportes/...`.
- `CLAS12TAGS_DIR` is hardcoded and missing in the current local environment.
- The workflow assumes `module`, `gemc`, `recon-util`, and `sbatch` are available in the target runtime environment.
- The script removes and recreates output subdirectories before job submission.

## Practical Consequence

In this checkout, the script starts correctly when run from `GEMC-samples/`, but exits immediately when checking the missing `CLAS12TAGS_DIR`.

## Suggested Fixes

- Make the external paths configurable through environment variables.
- Add a dry-run mode that resolves inputs without deleting output directories or submitting jobs.
- Add explicit validation for cluster-only commands before the script reaches submission.
