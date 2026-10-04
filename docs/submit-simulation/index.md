# Submit GEMC and reconstruction jobs

The `submit` workflow submits existing LUND files to Slurm, the job scheduler on Jefferson Lab's computing farm (ifarm). It creates one job array per sample: a group of jobs using the same script, with one task for each selected LUND file. Each task performs two stages:

1. GEMC transports the LUND particles through the CLAS12 detector and writes simulated HIPO.
2. COATJAVA reconstruction, invoked with `recon-util`, reads that file and writes reconstructed HIPO.

Submission does not create LUND files and does not run detector simulation in the login shell.

## Normal use

1. Prepare the [ifarm login environment](ifarm-environment.md).
2. Read the [submission guide](guide.md) and preview a completed run's `lundfiles/` directory.
3. Use the [configuration reference](configuration.md) to review software releases, GCARD/YAML inputs, field scales, file selection, and overrides.
4. Add `--execute` only when the preview and replacement actions are correct.
5. [Follow jobs and verify output](verification.md): inspect every task state and log, and check the HIPO files.

The completion manifest is the JSON log written after LUND creation succeeds. It supplies the filename prefix, list of files, event counts, beam energy, and target information. You can choose the detector files and software versions during submission, but you cannot change the recorded truth-level settings to describe a different sample.

The submission command returns after Slurm accepts the jobs. The printed job ID means they were submitted, not that they have started or finished. Follow the [verification guide](verification.md) to check every task and its output.

The [examples](examples.md) show common commands. The [worker reference](worker-reference.md) is for maintainers of the GEMC/COATJAVA integration rather than routine users.
