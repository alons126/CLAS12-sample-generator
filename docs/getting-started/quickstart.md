# Quickstart

All commands below start at the repository root. `run.csh` is a tcsh/csh workflow entry point; source it from a compatible shell. Examples intentionally use small event counts.

## Build and test

```bash
cmake -S . -B build/debug -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure
```

## Create a small uniform sample

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf \
  --events 100 \
  --output runs/quickstart
```

The completed run is `runs/quickstart/Uniform_sample_1e_5986MeV/`. Generation replaces that resolved run directory if it already exists.

## Convert existing physical truth

```tcsh
source run.csh \
  --workflow create-lund \
  --source physical \
  --config config/samples/genie-gst.conf \
  --input '/path/to/gst*.root' \
  --events 100 \
  --output runs/quickstart
```

Quote a glob so ROOT receives it unchanged. This command converts existing GENIE GST truth; it does not run GENIE.

## Preview ifarm submission

After either creation command completes, preview the detector job without submitting it:

```tcsh
source run.csh \
  --workflow submit \
  --lund-dir /absolute/path/to/completed-run/lundfiles \
  --num-jobs 2
```

Preview validates the manifest, detector inputs, software environment, and resulting `sbatch` command. Add `--execute` only after reviewing the resolved report. See the [submission examples](../submit-simulation/examples.md).
