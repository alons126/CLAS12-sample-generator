# Create LUND files, then submit ifarm simulation

The project has two separate workflows. `create-lund` produces completed LUND files from uniform kinematics or physical generator output. `submit` consumes those files and submits GEMC followed by reconstruction to ifarm Slurm.

```text
source run.csh --workflow create-lund --source uniform|physical ...
  -> launcher/workflow.py -> LUND application -> completed OUTPATH/lundfiles

source run.csh --workflow submit
  -> guarded server-checkout update
  -> source src/slurm-submission/setup_and_submit.csh
  -> settings, GEMC module, checks, output preparation, legacy setup report
  -> sbatch --job-name=NAME --array=1-N src/slurm-submission/external/submit_GEMC_sample.sh
  -> Slurm task: GEMC -> recon-util
```

Creation may run locally or on the server. Submission setup runs in the server login shell; detector execution runs only in Slurm jobs. The setup script does not generate LUND or run GEMC locally.

## What to edit

Edit **`src/slurm-submission/setup_and_submit.csh`** in the local checkout, commit and push. At the top, select named samples with `set samples = ( uniform-example )`, `physical-example`, or both. The following `Sample settings` section supplies one plain `switch` case per sample; copy a case to add another run. These are examples to edit for the actual completed samples, not automatically discovered datasets.

The common settings select `GEMC_VERSION`, optional module loading, `CLAS12TAGS_DIR`, and optional farm-output clearing. Each sample explicitly supplies:

| Setting | Meaning |
| --- | --- |
| `source` | `uniform` or `physical`; selects the corresponding legacy report |
| `OUTPATH_BASE`, `OUTPATH` | Existing shared-storage base and exact completed sample directory |
| `SAMPLE_FILE_PREFIX` | Exact LUND prefix, without `_INDEX.txt`; do not infer metadata from it |
| `NUM_OF_JOBS` | Number of array tasks; requires every file numbered 1 through N |
| `JOB_NEVENTS` | Event limit passed to both GEMC and reconstruction in every task |
| `TEMP_BEAM_E`, `TEMP_OUTPATH_PARTICLE` | Beam and uniform channel labels |
| `SAMPLE_TARGET_NUCLEUS`, `TARGET_VARIATION` | Nuclear label and independently selected detector geometry |
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE`, `Q2_CUT` | Explicit physical provenance or `none` for inapplicable fields |
| `SLURM_JOB_NAME` | Recognizable sample/job name |
| `GCARD_FILE`, `YAML_FILE` | Detector configuration and reconstruction configuration |

The small beam switch supplies the archived detector-file conventions: 2070 MeV uses torus +0.5 and `rgm_fall2021-cv.yaml`; 4029/5986 MeV use torus −1.0 and their `ai_4Gev`/`ai_6Gev` YAML files. Solenoid is −1.0 in the protected payload. Review the selected GCARD/YAML and their compatibility with the loaded version. The script never edits detector resources.

For maintained uniform output, the default directory and prefix are `Uniform_sample_CHANNEL_ENERGY`, for example `Uniform_sample_enFD_2070MeV`. For physical output, copy the exact directory and prefix reported by creation. Archived LUND files are also accepted: set their actual prefix explicitly. A manifest is useful provenance but is not a submission configuration or a prerequisite for archived input.

Set `JOB_NEVENTS` to the intended per-task limit (normally 25000 for uniform files or 10000 for physical files), and select the actual completed file count with `NUM_OF_JOBS`. A shorter final file remains in the same array with the same limit; processing reaches input EOF. The setup does not claim an exact per-file event count or introduce a separate submission for that file. Confirm EOF handling with the selected GEMC/reconstruction versions during server validation.

## Run on ifarm

Use a csh/tcsh login shell with its module command initialized and reconstruction available:

```tcsh
source run.csh --workflow submit
```

`run.csh` first refreshes the disposable server clone from the remote. **Server edits are discarded; commit and push settings from the local clone first.** Keep completed LUND/output directories on shared storage outside that disposable checkout. See [SSH execution](ssh-workflow.md) for the updater's build exclusions and failure handling.

The sourced setup prints the legacy banners, variables, module-loading messages, path checks, directory counts and final command. It runs `module unload gemc` and `module load gemc/VERSION` when enabled, then validates `GEMC_DATA_DIR`. The login-shell environment and all `setenv` settings reach `sbatch`; `SBATCH_EXPORT=ALL` and `SLURM_EXPORT_ENV=ALL` prevent an inherited restrictive export policy from dropping them. Scheduler resource and log defaults come from the protected payload's existing `#SBATCH` directives.

**Submission removes and recreates `OUTPATH/mchipo` and `OUTPATH/reconhipo`; uniform submission also replaces `OUTPATH/rootfiles`.** It preserves `lundfiles`. Before replacement it validates selected LUND inputs, detector files, the payload, and required executables; unsafe paths and output-directory symlinks are rejected. Optional `CLEAR_FAR_OUT=true` deletes files directly in the configured farm-output directory. It defaults to false.

One `sbatch --array=1-N` call is made per selected sample. Each sample repeats the setup/report, making its environment explicit even when uniform and physical samples are selected together. Setup or submission failure stops later samples, returns a nonzero `$status`, and keeps the sourced shell alive. Already accepted Slurm jobs remain submitted. There is no automatic cancellation or resume.

Paths must be absolute and contain only letters, digits, `/`, `.`, `_`, and `-`, because the protected worker retains its legacy unquoted detector command arguments. Sample prefixes must be single safe filename components. Unsupported beam/channel settings fail explicitly.

## Validation

`submission-legacy-parity` runs relocated temporary copies of both archived setup scripts with fake module and Slurm commands. It compares full stdout byte-for-byte (including color bytes and whitespace), array arguments, module operations and exported settings. Only fixture paths, selected sample settings, and the common payload location are substituted. It also checks maintained channel extensions, multiple samples, input failures, module/submission failures, output replacement and shell survival. It never submits real jobs.

These tests establish the setup/handoff contract. They do not establish detector-level equivalence or prove that the server's installed modules and databases are available. Validate a small array on ifarm before production. The [payload interface](gemc-payload.md) and [legacy comparison](legacy-workflows.md) describe the preserved detector chain.
