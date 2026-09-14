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

Add `--execute` to run. `--solenoid` defaults to −1. `--file-index 2` selects the second manifest file; otherwise the runner processes all files sequentially. Paths containing spaces are passed as individual subprocess arguments.

The runner creates `mchipo/mc_INDEX.hipo` and `reconhipo/recon_INDEX.hipo`. It uses the manifest count for both `gemc -N` and `recon-util -n`, including partial files. Reconstruction only starts if GEMC succeeds and produces a nonempty HIPO file. A successful `simulation/INDEX.json` records commands and SHA-256 hashes of both detector configuration files.

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

Edit site JSON for scheduler resources and executable paths. Slurm's normal stdout/stderr defaults apply. Use `--runner /shared/path/run.py` if the default source/install path is not the one workers should use.

## Scope of validation

Local automated tests use fake executables and tiny samples. Actual GEMC and reconstruction execution must be checked in the intended environment. The repository preserves imported gcard/YAML files; it does not silently assign detector versions based on output names.
