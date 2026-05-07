# GEMC And Reconstruction Batch Submission

## Purpose

This workflow submits Slurm batch jobs that:

- run GEMC on existing LUND inputs
- write MC HIPO output
- run reconstruction with `recon-util`

Primary orchestration entry points:

- `GEMC-samples/setup_and_submit_jobs.csh`
- `GEMC-samples/scripts/setup_and_submission_scripts/run_setup_and_submission_scripts.csh`
- `GEMC-samples/scripts/setup_and_submission_scripts/genie_job_submission_script.csh`
- `GEMC-samples/scripts/setup_and_submission_scripts/uniform_setup_and_submit.csh`

Batch payloads:

- `GEMC-samples/scripts/job_submission_scripts/submit_GEMC_GENIE_sample.sh`
- `GEMC-samples/scripts/job_submission_scripts/submit_GEMC_uniform_sample.sh`

## How It Is Intended To Run

The top-level wrapper assumes it is launched from inside `GEMC-samples/`.

```bash
cd GEMC-samples
csh setup_and_submit_jobs.csh
```

The orchestration layer:

- sources shared environment setup
- optionally updates the checkout
- selects uniform or GENIE submission flows
- submits Slurm arrays that run GEMC and reconstruction

## Verified Issues In This Checkout

- `setup_and_submit_jobs.csh` sources `scripts/update_script.csh` before submission.
- `update_script.csh` runs `git clean -fxd`, `git reset --hard`, and `git pull`.
- The GENIE submission flow depends on hardcoded output roots under `/lustre24/...`, which are missing locally.
- The batch payload scripts assume the runtime environment provides `gemc`, `recon-util`, and Slurm variables such as `SLURM_ARRAY_TASK_ID`.

## Practical Consequence

The top-level entry point is not safe to run casually in a dirty working tree. Even before cluster submission concerns, it can delete untracked files and discard local changes.

## Suggested Fixes

- Remove destructive Git operations from the default submission path.
- Split repository-update behavior into a separate, explicit maintenance script.
- Add a non-submitting validation mode for checking paths, cards, and YAML files without invoking `sbatch`.
