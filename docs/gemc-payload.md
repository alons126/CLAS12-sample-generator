# Unified legacy GEMC payload

`src/common/external/submit_GEMC_sample.sh` follows the structure of the external `submit_GEMC_GENIE_sample.sh` and `submit_GEMC_uniform_sample.sh` scripts under `legacy/GEMC-samples/scripts/job_submission_scripts/`. The archived originals remain untouched. The unified script is also protected from routine edits; only minimal integration changes outside the payload should be needed when updating the external source.

## What is generalized

Only the sample monitoring and filename-prefix sections differ:

- `SAMPLE_GENERATOR` identifies uniform, GENIE or any other generator.
- `GENERATOR_TUNE` replaces the GENIE-specific tune variable.
- `SAMPLE_FILE_PREFIX` supplies the complete prefix instead of hardcoding either the uniform or GENIE naming formula.
- Target, Q², beam, GEMC data directory and uniform channel monitoring are retained together. Unset optional labels print empty values, as in the originals.

For uniform samples, a prefix can be `Uniform_en_sample_2070MeV`. For physical samples, it can be `C12_GEM21_11a_00_000_Q2_0_02_2070MeV`. Other generators supply their own prefix without adding branches to the script.

## What remains the same

The Slurm header and the entire block beginning with `NEVENTS=10000` are copied unchanged from the legacy GENIE payload: field settings, directory assignments, GEMC command and reconstruction command. There are no command arrays, parser, preview mode, directory creation, input validation or output checks added to this script.

The payload processes 10000 events with solenoid -1.0 and `TORUS_FIELD` supplied by the caller. Paths follow:

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
| `SAMPLE_GENERATOR`, `GENERATOR_TUNE` | Generator/tune monitoring labels |
| `SAMPLE_TARGET_NUCLEUS`, `Q2_CUT` | Target and Q² monitoring labels |
| `TEMP_BEAM_E`, `TEMP_OUTPATH_PARTICLE`, `GEMC_DATA_DIR` | Beam/channel/environment monitoring |
| `OUTPATH`, `SLURM_ARRAY_TASK_ID` | Run directory and file index |
| `TORUS_FIELD` | +0.5 at 2 GeV; −1 at 4/6 GeV |
| `GCARD_FILE`, `YAML_FILE` | Detector and reconstruction configurations |

## Integration with the maintained launcher

`run.csh` still selects simulation or submission. Python validates the manifest, prepares a preview, and supplies the variables above. Actual detector execution invokes this Bash script. Slurm workers use the same coordinator and payload. `GENIE_TUNE` inherited by the Python coordinator maps to `GENERATOR_TUNE` for compatibility.

The coordinator requires complete 10000-event files, matching `lundfiles/PREFIX_INDEX.txt` names, legacy output naming and solenoid -1. It rejects incompatible requests instead of silently processing them with different settings. Because the original reconstruction command leaves paths unquoted, whitespace/glob-containing paths are rejected. Site executable paths must end in `gemc` and `recon-util`; their directories are added to PATH.

The coordinator creates output directories, retains per-file locks, invokes Bash with `-e`, checks both outputs and records command/configuration/payload hashes. Those protections live outside the unchanged payload. The script has no `--print-commands` or explicit input/output-path interface.

CMake installs `submit_GEMC_sample.sh` under `bin/` beside the Python runners. `--payload` can select another worker-visible copy. The maintained submitter gets scheduler resources from site JSON; direct sbatch invocation uses the script's original directives unless overridden.
