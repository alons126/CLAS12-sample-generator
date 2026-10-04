# Simulation submission guide

After LUND creation writes its final JSON log, run submission with [`run.csh`](../../run.csh) from a csh/tcsh login shell on ifarm. That log is the completion manifest: it tells submission which files were written and what settings created them.

## Preview one run

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

Without `--execute`, submission runs a preview. It reads the sample settings, selects and checks the GEMC and COATJAVA versions, checks the detector files, and prints the exact `sbatch` command it would use. It keeps existing output files, but creates `mchipo/` or `reconhipo/` if either directory is missing. It does not submit jobs or write a submission log.

Add `--execute` only after reviewing that report:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles \
    --execute
```

With `--execute`, submission warns, deletes `mchipo/` and `reconhipo/` and everything inside them, recreates both directories empty, and submits the jobs. It never deletes `lundfiles/`. You must put `--execute` on the command line; a configuration file cannot enable it.

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

The submission program first reads and checks the settings for every sample. It then prepares output and submits one job array per sample, in the order given. If processing one sample fails, it stops without processing the remaining samples. Jobs submitted earlier are not cancelled.

## Submission record and failures

After `sbatch` returns `Submitted batch job NUMBER`, the coordinator prints the numeric job ID and atomically writes `RUN/reconhipo/slurm-submission-log.json`. It records the ID, exact command, resolved parameters, both requested software versions, executable paths, GEMC's data directory, loaded-module names, runtime Git state, and SHA-256 hashes of the GCARD, YAML, and worker payload.

If Slurm accepts an array but its response cannot be parsed, or writing the submission log fails afterward, inspect Slurm before retrying. Retrying blindly can create a duplicate array. An earlier accepted array is never cancelled automatically when a later sample fails.

To delete old scheduler logs, supply both `--clear-farm-out true` and `--farm-out DIRECTORY`. With `--execute`, cleanup runs once and deletes ordinary files directly inside that directory. It does not enter subdirectories or delete symbolic links (paths that point to other files or directories). Omit these options to keep the logs. Cleanup happens before the software-module checks; if a later check fails, the deleted logs are not restored.

## Follow the submitted array

The job ID confirms that Slurm accepted the submission. The submission program does not wait for jobs to finish or retry failed tasks. Continue with [follow jobs and verify output](verification.md) to check task states, logs, and HIPO files.
