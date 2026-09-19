# Unified legacy GEMC payload

`src/slurm-submission/external/submit_GEMC_sample.sh` follows the structure of the external `submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` scripts under `legacy/GEMC-samples/scripts/job_submission_scripts/`. The archived originals remain untouched. The unified script is also protected from routine edits; only minimal integration changes outside the payload should be needed when updating the external source.

## What is generalized

Only the sample monitoring and filename-prefix sections differ:

- `SAMPLE_GENERATOR` identifies uniform, GENIE or any other generator.
- `GENERATOR_TUNE` replaces the GENIE-specific tune variable.
- `SAMPLE_FILE_PREFIX` supplies the complete prefix instead of hardcoding either the uniform or GENIE naming formula.
- Target, Q², beam, GEMC data directory and uniform channel monitoring are retained together. Unset optional labels print empty values, as in the originals.

For uniform samples, a prefix can be `Uniform_en_sample_2070MeV`. For physical samples, it can be `C12_GEM21_11a_00_000_Q2_0_02_2070MeV`. Other generators supply their own prefix without adding branches to the script.

## What remains the same

The Slurm header, field settings, directory assignments, GEMC command and reconstruction command retain the legacy GENIE payload structure. The event-count assignment is generalized to require `JOB_NEVENTS` from the coordinator. There are no command arrays, parser, preview mode, directory creation, input validation or output checks added to this script.

The payload processes `JOB_NEVENTS` events with solenoid -1.0 and `TORUS_FIELD` supplied by the caller. The same count is passed to GEMC and reconstruction. Paths follow:

```text
OUTPATH/lundfiles/SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID.txt
OUTPATH/mchipo/mc_SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID_torusTORUS_FIELD.hipo
OUTPATH/reconhipo/recon_SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID_torusTORUS_FIELD.hipo
```

Create the directories before direct execution. `gemc` and `recon-util` must be available in PATH. Execute with Bash or submit with `sbatch`; the script does not itself submit a job. Direct execution retains legacy failure behavior: without a caller-supplied Bash `-e`, a failed GEMC command does not automatically prevent reconstruction.

## Caller settings

| Variable | Purpose |
| --- | --- |
| `SAMPLE_FILE_PREFIX` | Complete filename prefix, without the `_INDEX.txt` suffix |
| `JOB_NEVENTS` | Validated number of events in this manifest file; required |
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE` | Generator/tune monitoring labels |
| `SAMPLE_TARGET_NUCLEUS`, `Q2_CUT` | Target and Q² monitoring labels |
| `TEMP_BEAM_E`, `TEMP_OUTPATH_PARTICLE`, `GEMC_DATA_DIR` | Beam/channel/environment monitoring |
| `OUTPATH`, `SLURM_ARRAY_TASK_ID` | Run directory and file index |
| `TORUS_FIELD` | +0.5 at 2 GeV; −1 at 4/6 GeV |
| `GCARD_FILE`, `YAML_FILE` | Detector and reconstruction configurations |

The maintained coordinator builds the payload environment from the shared validation implementation in `src/slurm-submission/run.py`. `src/slurm-submission/submit.py` then passes the validated values directly to `sbatch --export` and submits the protected payload as the final command argument. Each manifest file gets a single-element array, so Slurm creates the matching `SLURM_ARRAY_TASK_ID`; `JOB_NEVENTS` comes from that entry's validated `events` value, including a partial final file. Sample labels come from manifest configuration with documented environment overrides; this includes `Q2_CUT`, whose fallback is `config.q2-cut`.

## Integration with the maintained launcher

`run.csh` still selects simulation or submission. Python validates the manifest, prepares a preview, and supplies the variables above. Actual Slurm submission invokes this Bash script directly; `run.py` remains available for local validation/execution and shared planning but is not embedded in the submitted command. `GENIE_TUNE` inherited by the Python coordinator maps to `GENERATOR_TUNE` for compatibility.

The coordinator requires positive per-file manifest counts, matching `lundfiles/PREFIX_INDEX.txt` names, legacy output naming and solenoid -1. It exports each selected file's count as `JOB_NEVENTS`, and also supplies `Q2_CUT` from manifest configuration unless the calling environment overrides it. Because the original reconstruction command leaves paths unquoted, whitespace/glob-containing paths are rejected. Site executable paths must end in `gemc` and `recon-util`; their directories are added to PATH.

The local runner creates output directories, retains per-file locks, invokes Bash with `-e`, checks both outputs and records command/configuration/payload hashes. The Slurm submitter creates required output directories and performs pre-submission conflict checks; Slurm owns the asynchronous payload execution. Those protections live outside the unchanged payload. The script has no `--print-commands` or explicit input/output-path interface.

CMake installs `submit_GEMC_sample.sh` under `bin/` beside the Python runners. `--payload` can select another worker-visible copy. The maintained submitter gets scheduler resources from site JSON; direct sbatch invocation uses the script's original directives unless overridden.
