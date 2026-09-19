# Submit completed uniform LUND files

Create the LUND files first. Edit the samples and their exact directories, filename prefixes,
beam/channel labels and task counts in `src/slurm-submission/setup_and_submit.csh`. Copy a
sample case for each desired beam/channel combination and select the cases in `samples`.
Commit and push from the local checkout, then on ifarm in csh/tcsh:

```tcsh
source run.csh --workflow submit
```

This performs setup and submits one array per selected sample. It replaces simulation output
directories while preserving LUND inputs. See the [submission guide](../../docs/gemc-reconstruction-batch-submission.md).
