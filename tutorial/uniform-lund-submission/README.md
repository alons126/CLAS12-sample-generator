# Submit completed uniform LUND files

Create the LUND files first. Pass each completed sample with `--lund-dir RUN/lundfiles`; its manifest provides the
beam/channel, prefix and counts. Optional config/CLI overrides select simulation settings.
Commit and push from the local checkout, then on ifarm in csh/tcsh:

```tcsh
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles
```

This performs setup and submits one array per selected sample. It replaces simulation output
directories while preserving LUND inputs. See the [submission guide](../../docs/gemc-reconstruction-batch-submission.md).
