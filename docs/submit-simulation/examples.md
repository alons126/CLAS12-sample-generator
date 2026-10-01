# Submission examples

Run these commands on ifarm. Preview is the default and should be the first action for every sample.

## Preview a completed run

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles
```

## Submit the first five files

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles \
    --num-jobs 5 \
    --execute
```

Execution replaces this run's `mchipo/` and `reconhipo/`, but preserves `lundfiles/`.

## Override detector inputs

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles \
    --gemc-version 5.14 \
    --coatjava-version 10.0.7 \
    --gemc-target-variation rgm_fall2021_Ar \
    --gcard config/detector/GEMC_GCARDs_6GeV/5.14/rgm_fall2021_Ar_6GeV.gcard \
    --yaml config/detector/COATJAVA_YAML_configs_6GeV/10.0.7/rgm_fall2021-ai_6Gev.yaml \
    --torus -1.0
```

The GCARD controls GEMC detector simulation. The YAML controls COATJAVA reconstruction.

## Preview two independent arrays

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/run-a/lundfiles \
    --lund-dir /shared/run-b/lundfiles \
    --num-jobs 2
```

## Test a custom clas12Tags checkout

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles \
    --clas12tags-dir /shared/development/clas12Tags
```

Use this only for a reviewed detector-development study. It changes the GEMC data directory passed to Slurm.

The [submission all-options tutorial](../../tutorials/slurm-submission/all-options.txt) demonstrates every public option, including manual metadata and farm-log cleanup. The production command lists pair the uniform and physical creation examples with their corresponding submissions.
