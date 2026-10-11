# Quickstart

Start with the [environment setup and prerequisite checks](installation.md). For ifarm, also complete the [login environment](../submit-simulation/ifarm-environment.md#login-environment). These examples use small event counts. Replace every `/path/to/...` value before running a command. Follow the uniform example first; the physical example requires a separately prepared GST dataset.

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

Before running the workflow, [`run.csh`](../../run.csh) discards uncommitted edits to tracked files and deletes untracked and ignored files, except `build/`. It then pulls the configured Git branch. If `.gitmodules` exists, it also updates the external repositories declared there, called submodules. A checkout without that file skips this step and uses its own application files. Commit and push development changes from your local copy first. Keep generated samples outside the ifarm repository directory so cleanup cannot delete them.

## Run locally during development

After completing the [local build](installation.md#choose-the-right-entry-point), invoke the application directly:

```bash
build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/quickstart
```

For physical input, use electron-scattering GST with upstream neutral-pion decays, as described in the [physical-input contract](../create-lund/physical.md#selection-and-translation). The command below describes $\mathrm{Ar}^{40}$ at 5.98636 GeV with an upstream minimum $Q^2$ of 0.40 GeV². Use it only for input matching those values, or change all three to match your dataset. The physical LUND converter does not verify those assertions against GST truth. Changing only the beam energy does not change an explicit profile `q2-cut`.

To copy those particles, select the profile [`genie-gst.conf`](../../config/samples/physical-lund-creation/genie-gst.conf):

```bash
build/debug/apps/event-generator-to-lund-converter \
    --config config/samples/physical-lund-creation/genie-gst.conf \
    --input '/path/to/gst*.root' \
    --target Ar40 \
    --beam-energy 5.98636 \
    --q2-cut Q2-0.40 \
    --events 100 \
    --output runs/quickstart
```

Quote the input pattern so ROOT, rather than the shell, receives it. This command converts existing GENIE GST truth; it does not run GENIE.

## Check completion

A successful run contains:

```text
RUN/lundfiles/lund-creation-monitoring/lund-creation-log.json
```

This JSON file is the LUND creation log: it lists the settings, LUND filenames, and event counts. The application writes it after its checked creation stages finish. For uniform samples, also open the monitoring ROOT file and PDF and check the PNG inventory: the program attempts rendering but does not independently verify PDF/PNG export success. If the LUND creation log is missing, creation did not finish successfully, even if some LUND files exist. Do not submit those partial files.

## Preview simulation submission

The 100-event uniform example creates one LUND file ending in `_1.txt`; its LUND creation log should report `written_events = 100` and one file with `events = 100`. Uniform monitoring should contain the ROOT file, PDF, and PNGs shown in the [output layout](outputs.md).

Make the complete run directory, including its LUND creation log under `lundfiles/`, available on storage visible from ifarm. When moving local output, copy the whole run, not only the numbered text files. Submission derives its location from `--lund-dir`, so the original absolute `config.output` stored in the LUND creation log does not need editing. On ifarm, preview all files:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/quickstart-output/Uniform__1e__5986MeV/lundfiles
```

Preview validates and reports the environment, detector inputs, output actions, and exact `sbatch` command. It does not call `sbatch`. Read the [submission guide](../submit-simulation/guide.md) before adding `--execute`.

Read [outputs and path notation](outputs.md) before continuing. After this first uniform run, follow the guide for your chosen LUND source from the [getting-started reading order](index.md).

## Submit and verify the first uniform run

Read the [submission guide](../submit-simulation/guide.md) and review the preview first. With execution, the command below deletes and recreates this run's entire `mchipo/` and `reconhipo/` directories. Use a new run for this exercise and do not submit into a run with active tasks.

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/quickstart-output/Uniform__1e__5986MeV/lundfiles \
    --execute
```

The accepted job ID means submission succeeded, not that simulation finished. For this one-file run, expect one GEMC HIPO file and one reconstructed HIPO file with index 1 after the task completes. Follow [job and HIPO verification](../submit-simulation/verification.md) using that job ID. If an earlier step fails, use [troubleshooting](troubleshooting.md); do not repeatedly run the destructive launcher to diagnose a dependency problem.
