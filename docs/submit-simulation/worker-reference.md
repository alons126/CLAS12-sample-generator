# GEMC and reconstruction worker

`src/workflows/slurm-submission/external/submit_GEMC_sample.sh` is the per-array-task boundary. It is adapted from RG-M submission code and is intentionally kept separate from the Python coordinator. Routine users should run `source run.csh --workflow submit`; they should not call this file directly.

## Responsibility split

The coordinator resolves and validates configuration, loads and verifies the selected GEMC and COATJAVA environments, prepares output directories, and calls `sbatch`. Slurm supplies `SLURM_ARRAY_TASK_ID`. The worker then:

1. reads one numbered LUND file;
2. runs GEMC with the chosen GCARD, torus scale, fixed solenoid $-1.0$, and shared event limit;
3. writes simulated HIPO under `mchipo/`;
4. runs COATJAVA reconstruction with `recon-util` and the selected YAML; and
5. writes reconstructed HIPO under `reconhipo/`.

The paths are:

```text
RUN/lundfiles/<PREFIX>_<TASK>.txt
RUN/mchipo/mc_<PREFIX>_<TASK>_torus<TORUS>.hipo
RUN/reconhipo/recon_<PREFIX>_<TASK>_torus<TORUS>.hipo
```

The same `JOB_NEVENTS` limit is passed to GEMC and reconstruction. A shorter LUND file is therefore valid input; the limit is a maximum, not a promise that every file contains that many events.

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
| `COATJAVA`, `CLAS12DIR` | Installation paths set by the selected COATJAVA module |
| `GEMC_VERSION`, `COATJAVA_VERSION` | Requested releases, reported and recorded by the coordinator |
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE`, `SAMPLE_TARGET_NUCLEUS`, `Q2_CUT` | Physical provenance printed in the task log |
| `BEAM_ENERGY_LABEL`, `UNIFORM_SAMPLE_CHANNEL` | Beam and uniform-channel labels printed in the task log |

The worker inherits the verified `PATH` and module environment. It does not load modules or select software versions itself. The coordinator exports both version values, reports them before submission, and records them with verified executable paths in the submission log. The current worker does not echo those version values. The coordinator also resolves and reports `DETECTOR_ENERGY_GROUP` while selecting detector resources; the worker does not read that value.

The echo statements make received values visible in the scheduler log; they do not perform validation. Validation belongs to the coordinator.

## Maintenance boundary

The worker owns the concrete GEMC and `recon-util` commands plus the Slurm header. It does not parse user options, discover files, create directories, preview actions, call `sbatch`, monitor task completion, or validate output. It also does not use `set -e`; if GEMC fails, Bash may continue to the reconstruction command. Operators must therefore inspect Slurm status and both log streams.

When adopting an upstream RG-M worker change, compare the detector commands and scheduler directives while preserving this project's generator-independent environment interface and filenames. Update the coordinator, this reference, and production validation together if that interface changes.
