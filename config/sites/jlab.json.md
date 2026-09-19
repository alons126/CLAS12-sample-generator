# jlab.json — detector programs and Slurm resource settings

## Description and purpose

[jlab.json](jlab.json) provides a server profile for the maintained simulation runner and Slurm submitter. The values preserve historical campaign examples; confirm that they match the intended server allocation before execution. This is strict JSON, with explanations kept here because consumers reject unknown keys.

## Workflow

1. Load the server's software environment and prepare a completed LUND manifest.
2. Pass this profile to `src/slurm-submission/submit.py` with manifest, detector and field options.
3. The submitter validates the plan and prints an `sbatch` array command.
4. With `--execute`, submit one task per LUND file; each worker calls the common simulation runner.

## Executables and scheduler fields

| Key | Checked-in value | Meaning |
| --- | --- | --- |
| `gemc` | `gemc` | Worker-visible executable name or path |
| `recon` | `recon-util` | Worker-visible reconstruction executable name or path |
| `slurm.account` | `clas12` | Scheduler account passed as `--account` |
| `slurm.partition` | `production` | Scheduler partition passed as `--partition` |
| `slurm.time` | `20:00:00` | Requested task wall-time limit |
| `slurm.mem` | `2000M` | Requested job memory |
| `slurm.output` | `/farm_out/%u/%x-%j-%N.out` | Standard-output log path interpreted by Slurm |
| `slurm.error` | `/farm_out/%u/%x-%j-%N.err` | Standard-error log path interpreted by Slurm |

The four resource keys account/partition/time/mem are required nonempty strings for submission; output/error are optional. Slurm expands the log filename patterns (`%u` user, `%x` job name, `%j` job ID, `%N` node name); the Python driver does not expand them. The submitter sets the array range from the manifest and uses one node/one task per array job. These are not fields in this JSON profile.

## Inputs, outputs and failure behavior

Shared paths and required software must be visible to workers. The profile does not establish SSH, source modules, select gcards, infer field polarity or install dependencies. Preview does not submit anything. Malformed site settings fail validation; a rejected `sbatch` command propagates failure. A successful submission does not itself prove detector processing completed. See [simulation and Slurm](../../docs/gemc-reconstruction-batch-submission.md) and [field settings](../../docs/external-inputs.md).
