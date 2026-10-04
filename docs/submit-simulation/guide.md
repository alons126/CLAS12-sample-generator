# Simulation submission guide

Run submission from a csh/tcsh login shell on ifarm after LUND creation has published its completion manifest.

## Preview one run

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

Preview is the default. It resolves the sample, loads and verifies the selected GEMC and COATJAVA environments, checks all detector inputs, inspects both simulation-output paths, creates a missing `mchipo/` or `reconhipo/` directory, and prints the exact `sbatch` command. It preserves existing output and does not call `sbatch` or write a submission log.

Add `--execute` only after reviewing that report:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles \
    --execute
```

Execution warns, removes the exact existing `mchipo/` and `reconhipo/` directories recursively, recreates them empty, and submits the array. It never removes `lundfiles/`. The `--execute` switch is CLI-only; a configuration file cannot enable it.

This replacement covers both entire directories even when `--num-jobs` selects only some files. Preserve any earlier HIPO output elsewhere first. Do not resubmit into the same run while earlier tasks are still writing there; the coordinator does not detect or stop those tasks.

## Select files and settings

The completion manifest supplies truth metadata, the file inventory, and per-file event counts. By default, submission selects every listed file and uses the largest event count among them as its common GEMC/reconstruction event limit. Use `--num-jobs N` to select the first N files and `--events-per-job N` to override that limit.

Consult the [configuration reference](configuration.md) for all options, config-file precedence, software selection, GCARD/YAML defaults, magnetic-field checks, and the manual-input interface. Review those settings before adding `--execute`.

## Submit several runs

Repeat `--lund-dir`:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run-a/lundfiles \
    --lund-dir /path/to/run-b/lundfiles \
    --num-jobs 5
```

Every sample is fully resolved before any output replacement or submission begins. The coordinator then processes samples in order and creates one independent array per sample. If a later sample fails, later samples are skipped, but arrays already accepted by Slurm remain submitted.

## Submission record and failures

After `sbatch` returns `Submitted batch job NUMBER`, the coordinator prints the numeric job ID and atomically writes `RUN/reconhipo/slurm-submission-log.json`. It records the ID, exact command, resolved parameters, both requested software versions, executable paths, GEMC's data directory, loaded-module names, runtime Git state, and SHA-256 hashes of the GCARD, YAML, and worker payload.

If Slurm accepts an array but its response cannot be parsed, or writing the submission log fails afterward, inspect Slurm before retrying. Retrying blindly can create a duplicate array. An earlier accepted array is never cancelled automatically when a later sample fails.

Optional farm-output cleanup deletes only direct regular files in the exact reviewed directory and runs once per invocation. Subdirectories and symbolic links are left alone. It requires both `--clear-farm-out true` and `--farm-out DIRECTORY`; omit both to preserve scheduler logs. With execution enabled, this cleanup happens before the software-module checks, so a later validation failure does not restore deleted logs.

## Follow the submitted array

An accepted job ID is the handoff to Slurm, not a completion result. Continue with [follow jobs and verify output](verification.md) to check task states, scheduler logs, and expected HIPO files. The coordinator neither waits for tasks nor retries failures.
