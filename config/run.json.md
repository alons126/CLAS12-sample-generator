# run.json - build and execution defaults

## Purpose

[run.json](run.json) supplies stable build/test controls to `scripts/workflow.py`. It deliberately does not select a user-facing workflow, a LUND source, a sample profile, input data, or output location. Those choices remain visible in every `source run.csh` command.

This is strict JSON. Its consumer rejects unknown keys, so explanations live in this adjacent Markdown file rather than comment properties inside the JSON object.

## Required command selections

Create a uniform LUND sample by naming both the source and its sample profile:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT
```

Convert physical generator output by naming the physical source, profile, input and output:

```tcsh
source run.csh --workflow create-lund --source physical \
  --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT
```

Submit an existing completed LUND run with the submission inputs:

```tcsh
source run.csh --workflow submit --manifest RUN/manifest.json [submission options]
```

`--workflow` is always required. `--source uniform|physical` is required for `create-lund` and is rejected for `submit`. Child options are forwarded exactly as written; the launcher no longer injects a hidden sample profile or output path.

## Available keys

| JSON key | Checked-in value | CLI override and purpose |
| --- | --- | --- |
| `build` | `true` | `--build false` reuses existing binaries instead of configuring/building |
| `run` | `true` | `--run false` stops after the requested build/test stages |
| `test` | `false` | `--test true` enables tests in CMake and requires CTest to pass before dispatch |
| `build_dir` | `build/release` | `--build-dir PATH` selects the CMake binary directory |
| `build_type` | `Release` | `--build-type Debug|Release|RelWithDebInfo|MinSizeRel` |
| `jobs` | `4` | `--jobs N` selects positive parallel build-worker count |

Precedence is:

```text
workflow.py built-in build defaults
    -> selected run JSON
    -> explicit launcher options
```

Use `--run-settings FILE` to select a different strict JSON build profile explicitly. There is no automatic `config/run.local.json`: normal ifarm synchronization removes untracked files, so an implicit local profile would be unreliable.

## Why this file remains

The file keeps stable operational defaults out of scientific sample profiles and avoids repeating build controls in every command. It does not hide the action being performed. A reader can determine the selected workflow, source, sample definition, input and output directly from the command line.

Sample physics and generation settings belong in [samples](samples/). Site-specific Slurm settings belong in [sites](sites/). Protected GCARD and reconstruction resources belong in [detector](detector/).

## Failure behavior

Unknown keys, non-Boolean stage controls, unsupported build types, empty build paths, and nonpositive job counts fail before CMake or a child workflow runs. A failed checked build or test prevents LUND creation and submission.
