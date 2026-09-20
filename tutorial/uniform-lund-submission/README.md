# Submit completed uniform LUND files

Create the LUND files first. Pass each completed sample with `--lund-dir RUN/lundfiles`; its manifest provides the
beam/channel, prefix and counts. Optional config/CLI overrides select simulation settings.
Commit and push from the local checkout, then on ifarm in csh/tcsh:

```tcsh
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles
```

This previews setup and the command. Add `--execute` to submit one array per selected sample
and replace simulation output directories while preserving LUND inputs. See the [submission guide](../../docs/gemc-reconstruction-batch-submission.md).
