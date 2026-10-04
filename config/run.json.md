# Launcher settings

[`config/run.json`](run.json) contains stable build and execution defaults for [`src/launcher/workflow.py`](../src/launcher/workflow.py). It does not select a workflow, LUND source, sample profile, physical input, or output path; those choices remain visible on the command line.

The file is strict JSON. Unknown keys fail, so explanations live here rather than in invented comment fields.

## Settings

| Key | Checked-in value | CLI override |
| --- | --- | --- |
| `build` | `true` | `--build true\|false` |
| `run` | `true` | `--run true\|false` |
| `build_dir` | `build/release` | `--build-dir DIRECTORY` |
| `build_type` | `Release` | `--build-type Debug\|Release\|RelWithDebInfo\|MinSizeRel` |
| `jobs` | `4` | `--jobs N` with a positive integer |

Resolution is built-in launcher defaults, then the selected JSON file, then explicit launcher options.

Use another file by passing it to [`run.csh`](../run.csh) with `--run-settings`:

```tcsh
source run.csh \
    --run-settings /path/to/run-settings.json \
    --workflow create-lund \
    --source uniform \
    --build true \
    --run false
```

There is no implicit `run.local.json`. An untracked file inside the disposable ifarm checkout could be removed immediately before it was read.

## Launcher and child options

`--workflow create-lund|submit` is always required. For `create-lund`, launcher-owned `--source uniform|physical` selects the LUND application. Submission bypasses this build launcher; its resolver accepts the same `--source` spelling only as truth metadata for input without a completion manifest. Other unrecognized creation options are forwarded unchanged to the selected LUND executable.

Use `--` when forwarding help or any argument that should be unambiguously treated as a child option:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --build false \
    -- \
    --help
```

Submission bypasses [`workflow.py`](../src/launcher/workflow.py) and does not read this JSON. Its settings come from the LUND manifest, an optional submission config, CLI overrides, and submission defaults.

Forwarded application help still follows the disposable-checkout refresh. Only bare launcher help and the limited submission precheck can return before that refresh; see the [checkout model](../docs/submit-simulation/ifarm-environment.md#disposable-checkout). For local development, ask the compiled executable for `--help` directly.

Invalid Booleans, build types, paths, or worker counts fail before CMake or a child application runs. A failed build prevents LUND creation.
