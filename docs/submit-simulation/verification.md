# Follow jobs and verify output

Start here after Slurm accepts an array. `RUN` is the resolved sample run directory; `JOB_ID` is the submitted array ID. See [path notation](../getting-started/outputs.md#path-notation) if these placeholders are unfamiliar.

## Verify the finished array

The submission program does not keep checking jobs, retry failed tasks, count their output files, or open HIPO files. You must check these yourself. After the array finishes:

1. Review every array task in Slurm or the [outstanding Jobs dashboard](https://scicomp.jlab.org/scicomp/slurmJob/activeJob), and inspect its `.out` and `.err` files. Use `squeue -u <username>` to check your queued or running jobs.
2. Check that every submitted task has both a GEMC file in `RUN/mchipo/` and a reconstructed file in `RUN/reconhipo/`. For example, a five-task array should produce files with indexes 1 through 5 in each directory.
3. Open at least one reconstructed file:

   ```text
   hipo-utils -dump RUN/reconhipo/<file>.hipo
   ```

The file must open and display CLAS12 data banks, the named groups of records inside a HIPO file. Opening one file is only a basic check. It does not show that the other tasks succeeded or that the simulation is suitable for your analysis.

`squeue` and the active-jobs dashboard are for following current jobs. A job disappearing from the queue is not proof of success. Where Slurm accounting is available, inspect finished array tasks with [`sacct`](https://slurm.schedmd.com/sacct.html):

```bash
sacct \
    -j JOB_ID \
    --format=JobID,State,ExitCode
```

Replace `JOB_ID` with the array ID printed at submission. Read task states and exit codes alongside the worker logs and output inventory.

## Troubleshoot failed jobs

After tasks finish, possible failure indicators are:

- Errors when running `hipo-utils -dump RUN/reconhipo/<file>.hipo`.
- Missing expected files in `RUN/mchipo/` or `RUN/reconhipo/`, or both.
- A failure status for a task in the [outstanding Jobs dashboard](https://scicomp.jlab.org/scicomp/slurmJob/activeJob).

Inspect the affected task's `.out` and `.err` files and any available dashboard diagnostics to find what went wrong. The default farm-output location, `/u/scifarm/farm_out/<username>`, is indicated in [`submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) as `/farm_out/<username>`; both paths refer to the same directory. Explicitly, the script sets the destinations for the `.out` and `.err` files with:

```shell
#SBATCH --output=/farm_out/%u/%x-%j-%N.out
#SBATCH --error=//farm_out/%u/%x-%j-%N.err
```

The `--farm-out` option selects an explicit cleanup directory, not the scheduler-log destination. Avoid clearing these logs until you have investigated the failure.

Missing output while a task is still queued or running is not by itself a failure. Conversely, a successful Slurm status does not guarantee both detector stages succeeded: the worker can continue to reconstruction after GEMC fails. Check the logs and expected HIPO files together.

## Slurm commands

Jefferson Lab's [Slurm batch user guide](https://scicomp.jlab.org/docs/farm_slurm_batch) explains `sbatch` and farm batch operation. Useful commands are:

- Submit a GEMC job: `sbatch <submit-script>`
- Cancel one job: `scancel <job-id>`
- Cancel all of your jobs: `scancel --user=<username>`
- Check your jobs: `squeue -u <username>`; see also the [active-jobs dashboard](https://scicomp.jlab.org/scicomp/slurmJob/activeJob)
- Print the current priority for all pending production jobs: `squeue -t pd -p production -o "%.8Q %.10u/%10a" | uniq -c`

Cancel-all acts on every job owned by the named account, so use it only when that full scope is intended.
