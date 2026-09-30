# GEMC and reconstruction worker

`src/workflows/slurm-submission/external/submit_GEMC_sample.sh` is adapted from RG-M job-submission code. Slurm supplies one array index and environment variables identify the input and detector settings. The worker then runs the two-part CLAS12 simulation chain: GEMC transports the LUND particles through the detector and writes simulated HIPO output under `mchipo/`; COATJAVA reconstruction, invoked with `recon-util`, reads that file and writes reconstructed HIPO output under `reconhipo/`.

Python validates and resolves inputs but does not copy the detector commands. When RG-M provides an updated job script, compare it with this payload and carry forward the generator-independent inputs described below. The payload is excluded from routine edits so its origin and update path stay clear.

## What is generalized

Only the sample monitoring and filename-prefix sections differ:

- `SAMPLE_GENERATOR` identifies uniform, GENIE or any other generator.
- `GENERATOR_TUNE` replaces the GENIE-specific tune variable.
- `SAMPLE_FILE_PREFIX` supplies the complete prefix instead of hardcoding either the uniform or GENIE naming formula.
- Target, Q², beam, GEMC data directory and uniform channel monitoring are retained together. Unset optional labels print empty values, as in the originals.

For uniform samples, a prefix can be `Uniform__enFD__2070MeV`. For physical samples with an unknown generator version, it can be `C12__genie-gst__GEM21_11a_00_000__Q2-0.02__2070MeV`; a known version is joined to the generator with a hyphen, as in `C12__genie-gst-3.6.2__GEM21_11a_00_000__Q2-0.02__2070MeV`. Double underscores separate metadata groups; hyphens and decimal points inside one value remain unchanged. Other generators supply their own prefix without adding branches to the script.

## What remains the same

The script contains the Slurm header, field settings, directory assignments, GEMC detector-simulation command, and COATJAVA reconstruction command. It requires `JOB_NEVENTS` from the coordinator. It does not contain command arrays, a parser, preview mode, directory creation, input validation, or output checks.

The payload processes `JOB_NEVENTS` events with solenoid -1.0 and `TORUS_FIELD` supplied by the caller. The same count is passed to GEMC and reconstruction. Paths follow:

```text
OUTPATH/lundfiles/SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID.txt
OUTPATH/mchipo/mc_SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID_torusTORUS_FIELD.hipo
OUTPATH/reconhipo/recon_SAMPLE_FILE_PREFIX_SLURM_ARRAY_TASK_ID_torusTORUS_FIELD.hipo
```

Create the directories before direct execution. `gemc` and COATJAVA's `recon-util` reconstruction command must be available in PATH. Execute with Bash or submit with `sbatch`; the script does not itself submit a job. Without a caller-supplied Bash `-e`, a failed GEMC command does not automatically prevent reconstruction. After Slurm accepts an array, neither this worker nor the coordinator tracks later task failures, retries them, or validates reconstructed HIPO content. Operators must inspect Slurm states and logs and verify at least one `reconhipo/` file with `hipo-utils -dump`; see the [submission guide](guide.md#post-submission-verification).

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

The sourced `src/workflows/slurm-submission/setup_and_submit.csh` invokes `submit.py`, which checks the preloaded environment and inputs and ensures both simulation output directories exist. Preview creates only missing directories; `--execute` recreates both and passes these settings to `sbatch` for one array per sample. Slurm exports the configured environment and supplies the task index. The configured event limit is shared by the array, including a shorter final LUND file; it is not an exact per-file count.

## Integration with the launcher

`source run.csh --workflow submit` refreshes the disposable server checkout and sources the setup script directly in the login shell. The Python coordinator imports `resolve_inputs.py` to resolve the LUND manifest, optional key=value config and CLI. There is no site JSON, local detector runner, lock database or generated wrapper. The payload retains its scheduler directives and detector commands unchanged.

CMake installs only this external payload under `bin/`. The setup workflow runs from the checkout through `run.csh`. Review the [submission guide](guide.md) for settings and output replacement.
