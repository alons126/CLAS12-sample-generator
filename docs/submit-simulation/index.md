# Submit GEMC and reconstruction jobs

The `submit` workflow turns an existing LUND run into one ifarm Slurm array per sample. Each task performs two stages:

1. GEMC transports the LUND particles through the CLAS12 detector and writes simulated HIPO.
2. COATJAVA reconstruction, invoked with `recon-util`, reads that file and writes reconstructed HIPO.

Submission does not create LUND files and does not run detector simulation in the login shell.

## Normal use

1. Prepare the [ifarm login environment](ifarm-environment.md).
2. Read the [submission guide](guide.md) and preview a completed run's `lundfiles/` directory.
3. Use the [configuration reference](configuration.md) to review software releases, GCARD/YAML inputs, field scales, file selection, and overrides.
4. Add `--execute` only when the preview and replacement actions are correct.
5. [Follow jobs and verify output](verification.md): inspect every task state and log, and check the HIPO files.

The completion manifest normally supplies truth metadata, prefix, file inventory, event counts, target variation, and beam energy. Command-line or submission-config values can select simulation policy, but they cannot silently relabel manifest truth.

Submission responsibility ends when `sbatch` accepts the array. An accepted job ID proves that Slurm received the request; it does not prove that GEMC or reconstruction later succeeded. The [verification guide](verification.md) defines the required operator checks.

The [examples](examples.md) show common commands. The [worker reference](worker-reference.md) is for maintainers of the GEMC/COATJAVA integration rather than routine users.
