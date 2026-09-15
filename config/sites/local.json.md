# local.json — local detector executable selection

## Description and purpose

[local.json](local.json) selects the programs used by `scripts/simulation/run.py` for GEMC and reconstruction. It is strict JSON: the runner rejects unknown site keys, so documentation belongs in this companion.

## Workflow

1. Generate or convert events into a completed LUND run.
2. Pass this file with `--site config/sites/local.json` to the simulation runner.
3. The runner combines these program names with explicit manifest/card/YAML/field options.
4. It previews the commands, or runs GEMC then reconstruction when `--execute` is supplied.

## Fields

| Key | Value | Meaning |
| --- | --- | --- |
| `gemc` | `gemc` | GEMC executable name on PATH, or replace with its executable path |
| `recon` | `recon-util` | Reconstruction executable name on PATH, or its executable path |

Executable values are single names/paths, not shell command strings containing arguments. A name such as `gemc` must be available in the environment when executing. The runner does not load software modules.

## Inputs, outputs and failure behavior

Detector settings, field scales and the input manifest are supplied separately. GEMC creates simulated HIPO; reconstruction creates reconstructed HIPO. Missing executables fail execution validation; a failed GEMC command prevents reconstruction. Preview does not require installed detector binaries. This profile has no `slurm` section and cannot provide the scheduler resources required by the submitter. See [simulation and Slurm](../../docs/gemc-reconstruction-batch-submission.md).
