# Quickstart

These examples use small event counts. Replace every `/path/to/...` value before running a command.

## Create uniform LUND files on ifarm

From a csh/tcsh login shell in the repository root:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/output
```

The final run directory is `/path/to/output/Uniform__1e__5986MeV`. The uniform LUND creator prints that resolved path before replacing or writing it.

`run.csh` treats its ifarm checkout as disposable. A normal workflow run removes untracked files except documented build exclusions, discards tracked changes, pulls the configured upstream branch, and updates submodules. Commit and push valuable changes from a development checkout first.

## Run locally during development

After completing the [local build](installation.md#choose-the-right-entry-point), invoke the application directly:

```bash
build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/quickstart
```

To convert existing physical truth:

```bash
build/debug/apps/event-generator-to-lund-converter \
    --config config/samples/physical-lund-creation/genie-gst.conf \
    --input '/path/to/gst*.root' \
    --events 100 \
    --output runs/quickstart
```

Quote the input pattern so ROOT, rather than the shell, receives it. This command converts existing GENIE GST truth; it does not run GENIE.

## Check completion

A successful run contains:

```text
RUN/lundfiles/lund-creation-monitoring/lund-creation-log.json
```

The manifest is written last. Partial LUND files without this manifest do not form a completed run and should not be submitted.

## Preview simulation submission

Make the run available on storage visible from ifarm, then preview its Slurm array:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles \
    --num-jobs 2
```

Preview validates and reports the environment, detector inputs, output actions, and exact `sbatch` command. It does not call `sbatch`. Read the [submission guide](../submit-simulation/guide.md) before adding `--execute`.
