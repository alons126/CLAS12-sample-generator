# Submission examples

Run these commands on ifarm from a csh/tcsh login shell. Preview is the default and is the recommended first step for every sample.

## Preview one completed sample

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/Uniform_sample_1e_5986MeV/lundfiles
```

## Submit only the first five LUND files

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/Uniform_sample_1e_5986MeV/lundfiles \
  --num-jobs 5 \
  --execute
```

`--execute` replaces that run's `mchipo/` and `reconhipo/` directories while preserving `lundfiles/`.

## Override the shared per-task event limit

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/Uniform_sample_enFD_4029MeV/lundfiles \
  --num-jobs 3 \
  --events-per-job 5000
```

Without the override, the largest selected manifest file count becomes the shared GEMC/reconstruction limit.

## Preview several samples in one invocation

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/Uniform_sample_1e_2070MeV/lundfiles \
  --lund-dir /shared/runs/Uniform_sample_epFD_2070MeV/lundfiles \
  --lund-dir /shared/runs/Uniform_sample_enFD_2070MeV/lundfiles \
  --num-jobs 2
```

Each distinct sample becomes a separate Slurm array.

## Use a submission profile with explicit overrides

```tcsh
source run.csh \
  --workflow submit \
  --config config/submission.conf \
  --lund-dir /shared/runs/physical/lundfiles \
  --gemc-version 5.14 \
  --torus -1.0 \
  --job-name C12_GENIE_validation
```

## Select detector and reconstruction resources explicitly

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/physical/lundfiles \
  --gemc-target-variation rgm_fall2021_Ar \
  --gcard config/detector/Generation_files_6GeV/5.14/rgm_fall2021_Ar_6GeV.gcard \
  --yaml config/detector/Generation_files_6GeV/5.14/rgm_fall2021-ai_6Gev.yaml
```

Use paths that exist in the refreshed ifarm checkout. The GCARD controls detector simulation; the YAML controls reconstruction.

## Test a custom clas12Tags checkout

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/physical/lundfiles \
  --clas12tags-dir /shared/development/clas12Tags
```

This changes `GEMC_DATA_DIR` for detector-development testing while retaining the selected GEMC executable checks.

## Optional direct farm-log cleanup

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /shared/runs/physical/lundfiles \
  --clear-farm-out true \
  --farm-out /shared/farm_out \
  --execute
```

Use cleanup only with an exact reviewed directory. It deletes files directly inside that directory once per invocation; it does not broaden simulation-output replacement.

## Verify reconstructed output

The workflow does not monitor array tasks after `sbatch` accepts them. Once the jobs finish, review the Slurm states and logs and test at least one file under the run's `reconhipo/` directory:

```tcsh
hipo-utils -dump /shared/runs/physical/reconhipo/<hipo-file-name>.hipo
```

Confirm that `hipo-utils` opens the file and displays CLAS12 data banks. Also check the remaining task states and output inventory; one valid HIPO file does not guarantee that the complete array succeeded.

The repository also contains a matched [uniform submission command list](../../tutorials/uniform-samples/uniform-slurm-submission.txt) for 1e, enFD, and epFD at the three established beam energies.
