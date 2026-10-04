# GEMC and reconstruction worker

For each array task, Slurm runs [`submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) to process one LUND file. We call this script the worker. The Python submission program, or coordinator, checks settings and submits the array; the worker runs GEMC and COATJAVA on the allocated computing node. The worker was adapted from RG-M code. Routine users should use [`source run.csh --workflow submit`](../../run.csh), not run the worker directly.

## Responsibility split

The coordinator resolves and validates configuration, loads and verifies the selected GEMC and COATJAVA environments, prepares output directories, and calls `sbatch`. Slurm supplies `SLURM_ARRAY_TASK_ID`. The worker then:

1. reads one numbered LUND file;
2. runs GEMC with the chosen GCARD, torus scale, fixed solenoid $-1.0$, and shared event limit;
3. writes simulated HIPO under `mchipo/`;
4. runs COATJAVA reconstruction with `recon-util` and the selected YAML; and
5. writes reconstructed HIPO under `reconhipo/`.

The paths are:

```text
RUN/lundfiles/<PREFIX>_<INDEX>.txt
RUN/mchipo/mc_<PREFIX>_<INDEX>_torus<TORUS>.hipo
RUN/reconhipo/recon_<PREFIX>_<INDEX>_torus<TORUS>.hipo
```

The same `JOB_NEVENTS` limit is passed to GEMC and reconstruction. A shorter LUND file is therefore valid input; the limit is a maximum, not a promise that every file contains that many events.

Here `RUN` is `OUTPATH`, `<PREFIX>` is `SAMPLE_FILE_PREFIX`, `<INDEX>` is `SLURM_ARRAY_TASK_ID`, and `<TORUS>` is `TORUS_FIELD`, following the shared [path notation](../getting-started/outputs.md#path-notation).

## Environment interface

| Variable | Meaning |
| --- | --- |
| `OUTPATH` | Run directory containing `lundfiles/`, `mchipo/`, and `reconhipo/` |
| `SAMPLE_FILE_PREFIX` | Filename prefix without `_INDEX.txt` |
| `JOB_NEVENTS` | Required event limit shared by GEMC and reconstruction |
| `SLURM_ARRAY_TASK_ID` | One-based file/task index supplied by Slurm |
| `GCARD_FILE`, `YAML_FILE` | Detector and reconstruction inputs |
| `TORUS_FIELD` | Torus scale; the worker fixes the solenoid at $-1.0$ |
| `GEMC_DATA_DIR` | Selected standard or custom clas12Tags data directory |
| `GEMC_VERSION`, `COATJAVA_VERSION` | Requested releases, reported and recorded by the coordinator |
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE`, `SAMPLE_TARGET_NUCLEUS`, `Q2_CUT` | Physical provenance printed in the task log |
| `BEAM_ENERGY_LABEL`, `UNIFORM_SAMPLE_CHANNEL` | Beam and uniform-channel labels printed in the task log |

The worker inherits the verified `PATH` and module environment. It does not load modules or select software versions itself. The coordinator exports both version values, reports them before submission, and records them with executable paths in the submission log. The coordinator also resolves and reports `DETECTOR_ENERGY_GROUP` while selecting detector resources; the worker does not read that value.

The worker prints the settings it received into the scheduler log. Printing a value does not check whether it is correct; the submission program performs the checks before submitting jobs.

The worker uses shorter local names in its commands and printouts. These are aliases, not additional settings:

| Coordinator variable | Worker alias |
| --- | --- |
| `OUTPATH` | `JOB_OUT_PATH` |
| `SAMPLE_FILE_PREFIX` | `FILE_PREFIX` |
| `JOB_NEVENTS` | `NEVENTS` |
| `TORUS_FIELD` | `TORUS` |
| `GCARD_FILE` | `GCARD` |
| `YAML_FILE` | `YAML` |

Before running GEMC, each array task prints the requested GEMC and COATJAVA versions, inherited `LOADEDMODULES` value, and executable paths found with `command -v` for `gemc` and `recon-util`. These appear in the scheduler's `.out` file without requiring the `module` shell function, which may be unavailable in the job's Bash shell. Requested versions and inherited module names describe the selected environment; executable paths show which commands the worker will invoke. These printouts are not independent binary version queries and do not load modules or validate their versions.

## Maintenance boundary

The worker contains the GEMC and `recon-util` commands and the `#SBATCH` directives that tell Slurm how to run it. It does not read user options, find input files, create directories, preview actions, submit jobs, check task completion, or validate output. It does not use Bash's `set -e` option to stop on a failed command, so it may run reconstruction after GEMC fails. Check Slurm status and both `.out` and `.err` logs.

When adopting an upstream RG-M worker change, compare the detector commands and scheduler directives while preserving this code's generator-independent environment interface and filenames. Update the coordinator, this reference, and production validation together if that interface changes.
