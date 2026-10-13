# Troubleshooting

Start with the last error diagnostic and the phase that printed it. Do not repeatedly source [`run.csh`](../../run.csh) to diagnose missing software: it can clean the disposable checkout before the selected workflow fails. Use the read-only [prerequisite checks](installation.md#verify-prerequisites) and [ifarm command checks](../submit-simulation/ifarm-environment.md#check-commands-before-launching) first.

Messages that mention a "manifest" refer to the [LUND creation log](../concepts/lund-data-contract.md#lund-creation-log), `lund-creation-log.json`.

## Before sample creation or submission

| Symptom | What to check | Recovery and output effects |
| --- | --- | --- |
| `cmake`, `python3`, or `root-config` is missing | Selected environment and the version checks linked above | Activate the intended local environment or correct the ifarm login setup. No sample is created by dependency checks. |
| CMake cannot find ROOT, required components, or a supported C++ standard | ROOT location and compatible compiler | Use the [explicit ROOT build procedure](../development/building.md#select-root-explicitly) in a fresh build directory. A build failure prevents creation. |
| The launcher cannot identify the checkout, or Git update fails | Repository directory, current branch, upstream, and remote access | Use a normal clone on a branch with the intended upstream. The updater may already have deleted disposable files before a pull fails; it does not restore them. Keep valuable edits outside that checkout. |
| `modulecmd` is unavailable or a module cannot be selected | Login setup, command paths, and requested GEMC/COATJAVA releases | Correct the [ifarm environment](../submit-simulation/ifarm-environment.md#login-environment). Do not bypass version checks. Simulation output is not replaced before these checks, but explicitly requested farm-log cleanup can already have occurred. |
| A GCARD or YAML does not exist | Detector lookup group, target variation, and the independently selected software versions | Select compatible existing resources or supply reviewed explicit paths. A LUND-supported target does not guarantee an included detector card. See [detector defaults](../submit-simulation/configuration.md#detector-defaults). |
| `worker paths may contain only ...` or a label is not filename-safe | Resolved storage paths, `prefix`, `tune`, and `q2-cut` | Use the [shared naming restrictions](../create-lund/configuration.md#names-that-can-be-submitted). Copy a complete run to a permitted storage path if needed. Preserve scientific metadata; do not edit the LUND creation log merely to evade validation. |
| An option conflicts with the LUND creation log | Whether the option asserts a different source, beam, target, channel, prefix, tune, or selection | Remove an accidental override. If the recorded truth metadata is wrong, preserve the old run and create corrected input under another output parent. Submission cannot relabel it. |

## During LUND creation

| Symptom | What to check | Recovery and output effects |
| --- | --- | --- |
| `No GST entries found` or `Missing GST branch` | Quoted input pattern, file access, the `gst` tree, and the [required fields](../create-lund/physical.md#required-gst-fields) | Supply valid electron-scattering GST. Initial empty-input and branch-presence checks run before output replacement. |
| Branch-type, array-length, or ROOT read failure | Stored branch types and arrays in every input file | Repair or replace the upstream input. These checks can fail after output replacement; partial LUND files may remain without a final LUND creation log. |
| `No supported QE/MEC/RES/DIS events` | Interaction flags and whether the intended GST dataset was selected | Supply input with supported events. The run fails without a final LUND creation log; it does not manufacture events. |
| Conversion completes below the requested event count | The printed stop reason and counts of scanned input entries and written events | Input exhaustion or the [physical tail cutoff](../create-lund/physical.md#file-cutoff) can end a successful run below 100%. Inspect the final LUND creation log before deciding more input is needed. |
| Missing final LUND creation log or only `.json.tmp` remains | Terminal diagnostics, storage permissions, free space, and the last completed phase | Treat the run as incomplete. Preserve diagnostics, correct the cause, then rerun under a different output parent if the partial files must be retained. |
| Missing or unreadable uniform PDF/PNG despite a LUND creation log | ROOT rendering diagnostics and the [monitoring inventory](../create-lund/monitoring.md#check-monitoring-output) | Inspect the ROOT file and exports separately. The LUND creation log does not independently verify rendering. A creation rerun replaces the entire run, including HIPO output. |

Creation does not resume a partial run. Reusing the same resolved run path replaces it and everything inside it. A changed prefix alone does not create a different run directory. See [output replacement](../create-lund/configuration.md#output-replacement).

## During or after submission

| Symptom | What to check | Recovery and output effects |
| --- | --- | --- |
| `num-jobs` exceeds the available files | LUND creation log inventory and requested file count | Select at most the available count. A 100-event quickstart has one file, not five. |
| Missing, empty, or externally linked LUND input | Every file listed in the LUND creation log, including files outside the requested subset | Restore the complete run from a verified copy. Do not submit an interrupted creation as manual input. |
| Unsafe or invalid simulation-output directory | Exact `mchipo/` and `reconhipo/` paths | Both must be directories, not symbolic links or ordinary files. Investigate existing contents before changing those paths. |
| Slurm rejects the account or partition | Access to the [worker's account and partition](../submit-simulation/ifarm-environment.md#scheduler-resources) | Contact site support or the allocation administrator. Execution can already have replaced simulation output before `sbatch` rejects the job. |
| Submission reports an unparseable response or cannot write its log | Slurm queue/accounting and printed scheduler response | Check whether an array was accepted before retrying. A retry can duplicate jobs; accepted arrays are not cancelled automatically. |
| A task exceeds memory or time limits | Slurm state, allocation, and both scheduler logs | Consult the workflow maintainer about resources or smaller LUND files. Preserve results before resubmission: execution replaces both entire simulation-output directories, even for a subset of files. |
| Missing HIPO files, unreadable output, or a detector-stage error | [Task states, logs, and HIPO checks](../submit-simulation/verification.md) | A successful Slurm status alone is insufficient. The worker can proceed to reconstruction after GEMC fails. Investigate both stages before retrying. |

When reporting a problem, include the command, final error, source revision, selected environment, LUND creation log or Slurm submission log when present, and relevant scheduler logs. Do not include credentials. For accepted or possibly accepted jobs, include the job ID and check for active tasks before replacing output.
