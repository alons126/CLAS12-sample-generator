# GEMC, reconstruction and Slurm

Generation and conversion produce the same manifest schema, so these commands work with either workflow.

## Preview one run

After generating `runs/first-electron` using the README example:

```bash
python3 scripts/simulation/run.py \
  --manifest runs/first-electron/manifest.json \
  --gcard config/detector/Generation_files_6GeV/5.14/rgm_fall2021_Ar_6GeV.gcard \
  --reconstruction config/detector/Generation_files_6GeV/5.14/rgm_fall2021-ai_6Gev.yaml \
  --site config/sites/local.json \
  --torus -1
```

This is a dry run. It validates the manifest totals, file paths and configuration-file existence and prints the exact GEMC/reconstruction commands. It creates no simulation output and does not require installed GEMC binaries. It does not inspect the physical compatibility of detector-card contents; select cards, energy, geometry and field settings consistently.

Add `--execute` to run. `--solenoid` defaults to −1. `--file-index 2` selects the second manifest file; otherwise the runner processes all files sequentially. The preserved unquoted legacy command paths require paths without whitespace or glob characters.

By default the runner preserves legacy names: `mchipo/mc_LUNDSTEM_torusSCALE.hipo` and `reconhipo/recon_LUNDSTEM_torusSCALE.hipo`. The restored payload fixes 10000 events per file and solenoid -1; the coordinator rejects partial files, indexed naming and whitespace/glob-containing paths. Bash -e stops the coordinator-launched payload on command failure; the coordinator then checks both outputs. A successful `simulation/INDEX.json` records commands and SHA-256 hashes of both detector configuration files and `payload_sha256` for the executed Bash payload.

Existing outputs are rejected. A per-file lock prevents two processes from executing the same task concurrently. Failed jobs retain their locks/partial output for inspection; there is no automatic cleanup or resume. Use a fresh run directory, or deliberately resolve the failed file's state before retrying. A single run directory supports one simulation configuration.

## Slurm array

Run from a configured cluster login environment with files on shared storage:

```bash
python3 scripts/slurm/submit.py \
  --manifest runs/first-electron/manifest.json \
  --gcard config/detector/Generation_files_6GeV/5.14/rgm_fall2021_Ar_6GeV.gcard \
  --reconstruction config/detector/Generation_files_6GeV/5.14/rgm_fall2021-ai_6Gev.yaml \
  --site config/sites/jlab.json \
  --torus -1
```

This previews `sbatch`. Add `--execute` to submit. One array task is created per manifest file; each invokes the same runner with `$SLURM_ARRAY_TASK_ID`. The worker environment must provide Python 3.9+, GEMC, reconstruction and access to the script/config/input paths. The submitter does not install software or source environment scripts.

Edit site JSON for scheduler resources and executable paths. Optional site `slurm.output` and `slurm.error` select log paths. The JLab example preserves the archived `/farm_out/%u/%x-%j-%N` convention and uses 2000M memory for the single-task job. Use `--runner /shared/path/run.py` if the default source/install path is not the one workers should use.

## Scope of validation

Local automated tests use fake executables and tiny samples. Actual GEMC and reconstruction execution must be checked in the intended environment. The repository preserves imported gcard/YAML files; it does not silently assign detector versions based on output names.

The [legacy launch-chain mapping](legacy-workflows.md) traces `setup_and_submit_jobs.csh` and its manual workflow selection. [Command parity tests](validation.md) execute the archived Bash payloads with fake binaries and compare their argument lists to the new runner. They never submit jobs.

The supported checkout entry point is `source run.csh` in csh/tcsh; see [SSH execution](ssh-workflow.md). Geometry source, LUND format, gcard provenance and the required energy-dependent field settings are documented in [external inputs](external-inputs.md).

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
