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
| `JOB_NEVENTS` | Configured per-task event limit shared by the array; required |
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE` | Generator/tune monitoring labels |
| `SAMPLE_TARGET_NUCLEUS`, `Q2_CUT` | Target and Q² monitoring labels |
| `BEAM_ENERGY_LABEL`, `DETECTOR_ENERGY_GROUP`, `UNIFORM_SAMPLE_CHANNEL`, `GEMC_DATA_DIR` | Beam/resource-group/channel/environment monitoring |
| `OUTPATH`, `SLURM_ARRAY_TASK_ID` | Run directory and file index |
| `TORUS_FIELD` | +0.5 at 2 GeV; −1 at 4/6 GeV |
| `GCARD_FILE`, `YAML_FILE` | Detector and reconstruction configurations |

The sourced `src/slurm-submission/setup_and_submit.csh` invokes `submit.py`, which passes these settings to `sbatch`, checks the preloaded environment and inputs, recreates simulation output directories with `--execute`, and submits one array per sample. Slurm exports the configured environment and supplies the task index. The configured event limit is shared by the array, including a shorter final LUND file; it is not an exact per-file count.

## Integration with the maintained launcher

`source run.csh --workflow submit` refreshes the disposable server checkout and sources the setup script directly in the login shell. The Python coordinator imports `resolve_inputs.py` to resolve the LUND manifest, optional key=value config and CLI. There is no site JSON, local detector runner, lock database or generated wrapper. The payload retains its scheduler directives and detector commands unchanged.

CMake installs only this protected payload under `bin/`. The setup workflow runs from the checkout through `run.csh`. Review the [submission guide](gemc-reconstruction-batch-submission.md) for settings, output replacement and tests.
