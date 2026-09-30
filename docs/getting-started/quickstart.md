# Quickstart

All commands below start at the repository root and intentionally use small event counts. The build and LUND-creation commands run locally through the compiled executables. The submission preview uses `run.csh` from a csh/tcsh login shell on ifarm.

`run.csh` treats its ifarm checkout as disposable: before a workflow runs, it removes untracked files except the reusable `build/` tree, discards tracked changes, pulls the configured remote branch, and updates submodules. Commit and push valuable changes from a development checkout before using it.

## Build

```bash
cmake \
    -S . \
    -B build/debug \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug
cmake \
    --build build/debug \
    --parallel 4
```

## Create a small uniform sample

```bash
build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/quickstart
```

The completed run is `runs/quickstart/Uniform__1e__5986MeV/`. Generation replaces that resolved run directory if it already exists.

## Convert existing physical truth

```bash
build/debug/apps/event-generator-to-lund-converter \
    --config config/samples/physical-lund-creation/genie-gst.conf \
    --input '/path/to/gst*.root' \
    --events 100 \
    --output runs/quickstart
```

Quote a glob so ROOT receives it unchanged. This command converts existing GENIE GST truth; it does not run GENIE.

## Preview ifarm submission

After either creation command succeeds and the run is available on storage visible from ifarm, preview the detector job without submitting it:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /absolute/path/to/lund-run/lundfiles \
    --num-jobs 2
```

Preview validates the manifest, detector inputs, software environment, and resulting `sbatch` command. Add `--execute` only after reviewing the resolved report. See the [submission examples](../submit-simulation/examples.md).
