# Quickstart

These examples use small event counts. Replace every `/path/to/...` value before running a command.

## Create uniform LUND files on ifarm

From a csh/tcsh login shell in the repository root, select the profile [`uniform-1e-5986MeV.conf`](../../config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf):

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/quickstart-output
```

The final run directory is `/path/to/quickstart-output/Uniform__1e__5986MeV`. The uniform LUND creator prints that resolved path before replacing or writing it.

Before running the workflow, [`run.csh`](../../run.csh) discards uncommitted edits to tracked files and deletes untracked and ignored files, except `build/`. It then pulls the configured Git branch and updates submodules (external repositories included in this project). Commit and push development changes from your local copy first. Keep generated samples outside the ifarm repository directory so cleanup cannot delete them.

## Run locally during development

After completing the [local build](installation.md#choose-the-right-entry-point), invoke the application directly:

```bash
build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/quickstart
```

To copy particles from existing GENIE GST output, select the profile [`genie-gst.conf`](../../config/samples/physical-lund-creation/genie-gst.conf):

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

This JSON file is the completion manifest: it lists the settings, LUND filenames, and event counts. The application writes it only after all required output succeeds. If it is missing, creation did not finish successfully, even if some LUND files exist. Do not submit those partial files.

## Preview simulation submission

The 100-event uniform example creates one LUND file. Make the run available on storage visible from ifarm, then preview submission of all its files:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/quickstart-output/Uniform__1e__5986MeV/lundfiles
```

Preview validates and reports the environment, detector inputs, output actions, and exact `sbatch` command. It does not call `sbatch`. Read the [submission guide](../submit-simulation/guide.md) before adding `--execute`.

Next, read [outputs and path notation](outputs.md), then follow the guide for your chosen LUND source from the [getting-started reading order](index.md).
