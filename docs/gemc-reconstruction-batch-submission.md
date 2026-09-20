# Create LUND files, then submit ifarm simulation

The project has two separate workflows. `create-lund` produces completed LUND files from uniform kinematics or physical generator output. `submit` consumes those files and submits GEMC followed by reconstruction to ifarm Slurm.

```text
source run.csh --workflow create-lund --source uniform|physical ...
  -> launcher/workflow.py -> LUND application -> RUN/lundfiles and completed manifest

source run.csh --workflow submit --lund-dir RUN/lundfiles [overrides]
  -> guarded server-checkout update
  -> source src/slurm-submission/setup_and_submit.csh
     -> resolve_inputs.py: manifest + configuration + CLI -> validated shell settings
     -> GEMC module, legacy setup report/checks, output preparation
     -> sbatch --job-name=NAME --array=1-N <protected payload>
  -> Slurm task: GEMC -> recon-util
```

Creation may run locally or on the server. Setup runs in the server login shell; detector execution runs only in Slurm jobs. The Python helper reads and validates inputs only; the sourced shell owns modules, output replacement and submission. The protected payload is unchanged.

## Submit workflow-1 output

```tcsh
source run.csh --workflow submit --lund-dir /shared/Uniform_sample_enFD_2070MeV/lundfiles
```

Without `--execute`, this previews the resolved setup and exact `sbatch` command. It loads/checks the configured software environment but does not call `sbatch`, clear farm logs, or create/replace simulation output directories. Add `--execute` to perform those actions:

```tcsh
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles --execute
```

The switch is CLI-only: a config file cannot enable execution. The normal `run.csh` disposable-checkout synchronization still runs during preview; sample/output protection does not disable that documented server refresh.

The resolver reads `lund-gen-monitoring/lund-gen-log.json` under the supplied directory. It obtains source, beam energy, target identity, detector target variation, channel/hadron/region, generator/tune/Q² labels, filename prefix and completed file counts from the manifest. `OUTPATH` is the supplied directory's parent, so copied samples do not depend on the original absolute generation path. It does not infer scientific metadata from directory names.

The array size is the number of completed files, not the requested generation capacity. The default `JOB_NEVENTS` is the largest event count among the selected files. A shorter final file remains in the same array with this shared limit and reaches input EOF. This limit is not an exact per-file count; confirm EOF behavior with the selected detector versions during server validation.

**GEMC defaults to 5.14.** A concrete manifest version supplies the generation-time plan; `unknown`, `none` or `auto` fall back to 5.14. A config or `--gemc-version` overrides either. The default GCARD uses the explicit detector target variation, beam and resolved GEMC version. Default YAML and torus settings preserve the established 2070/4029/5986 MeV conventions: +0.5 at 2 GeV and −1.0 at 4/6 GeV. Other beam energies require explicit `--gcard`, `--yaml` and `--torus`. The payload retains fixed solenoid −1.0.

## Configuration and CLI overrides

All settings have matching `key = value` config entries and `--key value` CLI flags. Precedence is **CLI > explicit config > manifest > defaults**. CLI paths are relative to the checkout; paths inside a config are relative to that config. Blank lines and full-line `#` comments are accepted; duplicate, unknown and empty keys fail. Values are plain text, not executable shell expressions.

```tcsh
source run.csh --workflow submit --config config/submission.conf
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles \
  --num-jobs 10 --events-per-job 1000 \
  --gcard /shared/cards/custom.gcard --yaml /shared/reconstruction/custom.yaml
```

[config/submission.conf](../config/submission.conf) is a commented example to adapt, not an automatically loaded site profile. Use `source run.csh --workflow submit --help` for every option. Help and malformed CLI arguments return before server synchronization.

| Options | Meaning |
| --- | --- |
| `--lund-dir` | Existing RUN/lundfiles; repeat for several samples using the same overrides |
| `--num-jobs`, `--events-per-job` | Optional first-N file selection and per-task event limit |
| `--gemc-version` | GEMC version used for detector resources and output naming; defaults to 5.14 |
| `--gcard`, `--yaml`, `--torus` | Explicit simulation choices overriding derived defaults |
| `--clas12tags-dir` | Use a custom checkout or fork of [gemc/clas12Tags](https://github.com/gemc/clas12Tags) as `GEMC_DATA_DIR` |
| `--job-name` | Override the metadata-derived Slurm name |
| `--clear-farm-out true|false`, `--farm-out` | Optional log cleanup, off by default; explicit path required when enabled |
| `--fc-status 0|1` | Legacy physical naming/report label only; applies no cut |

Truth metadata overrides must agree with the manifest: source, beam, target identity, particle content, prefix and physical provenance cannot silently be relabeled. Detector variation and simulation policy may be changed independently. File paths, counts and totals are validated; incomplete `.json.tmp` output is rejected. No manifest is rewritten. Each selected sample must have a distinct output directory, and resolution of all samples finishes before any setup/submission begins.

## LUND without a manifest

Supply missing metadata through a config or CLI. For uniform input, for example:

```tcsh
source run.csh --workflow submit --lund-dir /shared/archive/lundfiles \
  --source uniform --beam-energy 2.07052 --rgm-target Ar40 \
  --channel eh --hadron neutron --hadron-region FD \
  --prefix Uniform_en_sample_2070MeV --events-per-job 10000 \
  --gemc-target-variation rgm_fall2021_Ar
```

Without a manifest, source, beam energy in GeV, target identity and prefix are required. Uniform input also needs its channel (`1e`, `eh`, `electron-tester`, or an explicit legacy/regional label). `eh` requires hadron and region. Physical input accepts `--event-generator` (default genie), `--tune` and `--q2-cut` (default unknown). Supply a detector target variation or explicit GCARD. The file inventory must be contiguous `PREFIX_1.txt` through `PREFIX_N.txt`. The resolver discovers the job count, but requires `events-per-job` because it does not scan whole LUND files to count events.

## Server execution and output replacement

Use a csh/tcsh login shell with GEMC and reconstruction already available. The submission workflow does not load or replace the GEMC module. By default, it uses the `GEMC_DATA_DIR` supplied by the preloaded ifarm GEMC environment.

For a custom GEMC detector implementation, such as testing target geometry, clone or fork [gemc/clas12Tags](https://github.com/gemc/clas12Tags) on shared storage and pass its checkout with `--clas12tags-dir DIRECTORY`. The setup validates `CLAS12TAGS_DIR` and then exports `GEMC_DATA_DIR=$CLAS12TAGS_DIR` before submission. `SBATCH_EXPORT=ALL` and `SLURM_EXPORT_ENV=ALL` preserve the selected directory in every Slurm task. Scheduler/log defaults remain in the protected payload's `#SBATCH` directives.

Before each project-owned `setenv`, the maintained submission path removes both a same-named tcsh local variable and any inherited environment value. Resolver-generated handoff files follow the same rule. This prevents persistent login-shell state from shadowing validated submission settings; the preloaded `GEMC_DATA_DIR` remains active unless the workflow replaces it with `--clas12tags-dir`.

`run.csh` refreshes the disposable server clone first. **Server edits are discarded; commit and push code/config changes from the local clone first.** Keep LUND/output on shared storage outside the disposable checkout. Explicit configs outside the checkout are also supported. See [SSH execution](ssh-workflow.md).

**With `--execute`, submission removes and recreates `OUTPATH/mchipo` and `OUTPATH/reconhipo`; uniform submission also replaces `OUTPATH/rootfiles`.** LUND inputs are preserved. Checks reject unsafe output paths and symlinks before replacement. Optional farm-output cleanup deletes only files directly in the configured farm-output directory, once per invocation.

One array is submitted per sample. Failure stops later samples and returns a nonzero `$status` without closing the sourced shell; already submitted jobs remain submitted. Worker paths must contain only letters, digits, `/`, `.`, `_` and `-`, because the protected payload retains its legacy unquoted command arguments. There is no local detector-execution workflow.

## Validation

`submission-legacy-parity` compares full archived setup stdout, module calls, array arguments and exported settings using temporary configs and fake tools. It also exercises the manifest-to-shell handoff, multiple samples, failures and protected detector-command parity. `submission-input-resolution` checks defaults (including GEMC 5.14), precedence, moved manifests, partial final files, manual input, truth conflicts and malformed inputs; when built, it also consumes actual uniform-generator output. Tests never submit real jobs. Server detector behavior requires a small ifarm validation run.
