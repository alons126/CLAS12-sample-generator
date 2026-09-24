# Create LUND files, then submit ifarm simulation

The project has two separate workflows. `create-lund` produces completed LUND files from uniform kinematics or physical generator output. `submit` consumes those files and submits GEMC followed by reconstruction to ifarm Slurm.

```text
source run.csh --workflow create-lund --source uniform|physical ...
  -> launcher/workflow.py -> LUND application -> RUN/lundfiles and completed manifest

source run.csh --workflow submit --lund-dir RUN/lundfiles [overrides]
  -> guarded server-checkout update
  -> source src/slurm-submission/setup_and_submit.csh
     -> submit.py
        -> resolve_inputs.py: manifest + configuration + CLI -> validated settings
        -> preloaded GEMC checks, setup report, output preparation
        -> sbatch --job-name=NAME --array=1-N <protected payload>
  -> Slurm task: GEMC -> recon-util
```

Creation may run locally or on the server. Submission runs in a Python child of the server login shell; detector execution runs only in Slurm jobs. The small sourced shell bridge initializes the shared colors and returns the Python status. `submit.py` owns reports, checks, output replacement and submission; `resolve_inputs.py` only resolves inputs. The protected payload is unchanged.

## Submit workflow-1 output

```tcsh
source run.csh --workflow submit --lund-dir /shared/Uniform_sample_enFD_2070MeV/lundfiles
```

Without `--execute`, this previews the resolved setup and exact `sbatch` command. It checks the preloaded software environment but does not call `sbatch`, clear farm logs, or create/replace simulation output directories. Add `--execute` to perform those actions:

```tcsh
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles --execute
```

The switch is CLI-only: a config file cannot enable execution. The normal `run.csh` disposable-checkout synchronization still runs during preview; sample/output protection does not disable that documented server refresh.

To submit several completed samples in one invocation, repeat `--lund-dir`:

```tcsh
source run.csh --workflow submit \
  --lund-dir /shared/sample-a/lundfiles \
  --lund-dir /shared/sample-b/lundfiles \
  --execute
```

The resolver validates every selected sample first and returns in-memory settings to `submit.py`. The coordinator processes those samples in order and produces one independent Slurm array per sample. No temporary shell assignments or generated wrappers are needed. If a later sample fails setup or `sbatch`, subsequent samples are skipped while arrays already accepted by Slurm remain submitted.

The resolver reads `lund-gen-monitoring/lund-gen-log.json` under the supplied directory. It obtains source, beam energy, target identity, detector target variation, channel/hadron/region, generator/tune/Q² labels, filename prefix and completed file counts from the manifest. `OUTPATH` is the supplied directory's parent, so copied samples do not depend on the original absolute generation path. It does not infer scientific metadata from directory names.

The array size is the number of completed files, not the requested generation capacity. The default `JOB_NEVENTS` is the largest event count among the selected files. Physical LUND conversion uses `events-per-file` as both its rollover size and its remaining-input cutoff scale, keeping generation aligned with the intended per-task limit. The cutoff is evaluated before starting a follow-up file and never interrupts a file already in progress. Because it counts input entries rather than accepted reactions, unsupported reactions inside an allowed block can still leave that file shorter than `JOB_NEVENTS`; that task reaches input EOF before the limit. Confirm EOF behavior with the selected detector versions during server validation.

**GEMC defaults to 5.14.** A concrete manifest version supplies the generation-time plan; `unknown`, `none` or `auto` fall back to 5.14. A config or `--gemc-version` overrides either. The default GCARD uses the explicit detector target variation, beam and resolved GEMC version. Default YAML and torus settings preserve the established 2070/4029/5986 MeV conventions: +0.5 at 2 GeV and −1.0 at 4/6 GeV. Other beam energies require explicit `--gcard`, `--yaml` and `--torus`. The payload retains fixed solenoid −1.0.

The resolver automatically derives two deliberately distinct labels from `beam-energy`: `BEAM_ENERGY_LABEL` identifies the sample as `2070MeV`, `4029MeV`, or `5986MeV`, while `DETECTOR_ENERGY_GROUP` selects the corresponding `2GeV`, `4GeV`, or `6GeV` detector-resource directory. For other energies, both use the nearest-MeV label and explicit detector inputs are required.

## Configuration and CLI overrides

All settings have matching `key = value` config entries and `--key value` CLI flags. Precedence is **CLI > explicit config > manifest > defaults**. CLI paths are relative to the checkout; paths inside a config are relative to that config. Blank lines and full-line `#` comments are accepted; duplicate, unknown and empty keys fail. Values are plain text, not executable shell expressions.

```tcsh
source run.csh --workflow submit --config config/submission.conf
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles \
  --num-jobs 10 --events-per-job 1000 \
  --gcard /shared/cards/custom.gcard --yaml /shared/reconstruction/custom.yaml
```

[config/submission.conf](../../config/submission.conf) is a commented example to adapt, not an automatically loaded site profile. Use `source run.csh --workflow submit --help` for every option. Help and malformed CLI arguments return before server synchronization.

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

Without a manifest, source, beam energy in GeV, target identity and prefix are required. Uniform input also needs its channel (`1e`, `eh`, `electron-tester`, or an explicit legacy/regional label). `eh` requires hadron and region. Physical input accepts `--event-generator` (default `genie-gst`), `--tune` and `--q2-cut` (default unknown). Supply a detector target variation or explicit GCARD. The file inventory must be contiguous `PREFIX_1.txt` through `PREFIX_N.txt`. The resolver discovers the job count, but requires `events-per-job` because it does not scan whole LUND files to count events.

## Server execution and output replacement

Use a csh/tcsh login shell with the ifarm module command and reconstruction available. When `CLAS12TAGS_DIR` is empty, submission derives the shared clas12Tags parent from the inherited `GEMC_DATA_DIR` (or uses `/u/scigroup/cvmfs/geant4/almalinux9-gcc11/clas12Tags` when none is inherited), checks the parent and requested-version directory, and only then loads `gemc/<version>` in the Python child environment. Informational output produced by both module operations streams directly to the terminal, preserving the module system's original colors; only its generated Python environment code stays internal. The workflow verifies that the resulting `GEMC_DATA_DIR` is the requested version and that the resolved `gemc` executable is inside that directory. The report prints `SLURM_GEMC_EXECUTABLE`, which is the executable inherited by `sbatch`.

For a custom GEMC detector implementation, such as testing target geometry, clone or fork [gemc/clas12Tags](https://github.com/gemc/clas12Tags) on shared storage and pass its checkout with `--clas12tags-dir DIRECTORY`. The selected GEMC module and executable are still loaded and verified, while the standard shared-version directory precheck is skipped. The setup validates `CLAS12TAGS_DIR` and then exports `GEMC_DATA_DIR=$CLAS12TAGS_DIR` before submission. `SBATCH_EXPORT=ALL` and `SLURM_EXPORT_ENV=ALL` preserve the selected directory in every Slurm task. Scheduler/log defaults remain in the protected payload's `#SBATCH` directives.

Python copies the inherited environment, loads the selected GEMC module in that copy, then overwrites its sample settings with the resolved values before calling `sbatch`. Stale shell locals cannot shadow these values. Unlike the former shell coordinator, Python does not leave module or per-sample exports in the interactive shell; the verified environment is passed to Slurm and its workers. Consequently, `which gemc` at the prompt after submission may still show the login shell's earlier version. Use the reported `SLURM_GEMC_EXECUTABLE` to identify what preview or execution will pass to Slurm.

`run.csh` refreshes the disposable server clone first. **Server edits are discarded; commit and push code/config changes from the local clone first.** Keep LUND/output on shared storage outside the disposable checkout. Explicit configs outside the checkout are also supported. See [SSH execution](ifarm-environment.md).

**With `--execute`, submission removes and recreates `OUTPATH/mchipo` and `OUTPATH/reconhipo` for both uniform and physical samples.** LUND inputs are preserved. Uniform monitoring is produced during LUND generation and does not use a simulation `rootfiles` directory. Checks reject unsafe output paths and symlinks before replacement. Optional farm-output cleanup deletes only files directly in the configured farm-output directory, once per invocation.

One array is submitted per sample. Failure stops later samples and returns a nonzero `$status` without closing the sourced shell; already submitted jobs remain submitted. Worker paths must contain only letters, digits, `/`, `.`, `_` and `-`, because the protected payload retains its legacy unquoted command arguments. There is no local detector-execution workflow.

## Validation

`submission-legacy-parity` checks captured working C-shell report fixtures for both sources in preview/execute modes, array arguments and exported settings using temporary configs and fake tools. It also exercises manifest-driven submission, multiple samples, cleanup, failures and the protected detector-command contract. The migration comparison preserved report text and ANSI colors, normalizing platform-specific `wc` padding. Physical reporting retains the resolved generator/tune instead of unsetting them before use, and farm cleanup uses the intended banner color; these correct two failures in the former shell implementation. `submission-input-resolution` checks defaults (including GEMC 5.14), precedence, moved manifests, partial final files, manual input, truth conflicts and malformed inputs; when built, it also consumes actual uniform-generator output. Tests never submit real jobs. Server detector behavior requires a small ifarm validation run.

Run the submission and launcher checks with the shared palette initialized (the existing launcher preflight uses it):

```tcsh
source src/launcher/environment/set_colors.csh
ctest --test-dir build/debug --output-on-failure -R 'submission|ssh-launcher'
```
