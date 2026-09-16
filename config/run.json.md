# run.json — checkout launcher settings

## Description and purpose

[run.json](run.json) controls `scripts/workflow.py`, reached through `source run.csh`. It selects the workflow and its build/update/test stages. Particle generation settings belong in `config/samples/*.conf` and are passed to the selected application.

This is strict JSON. Its reader rejects unknown keys, so banners and explanations live in this adjacent file rather than `_comment` or other invented settings.

## Workflow and precedence

1. The driver reads an explicit `--run-settings` path when supplied.
2. Otherwise it selects `config/run.local.json` if present, or `config/run.json`.
3. Built-in defaults are overlaid with that one JSON file and explicit launcher options. The local file replaces the checked-in profile selection; it is not merged on top of `run.json`.
4. The driver optionally updates Git, builds and tests, then dispatches one workflow.
5. Explicit forwarded options replace matching keys in the selected workflow's `arguments` list.

Paths passed through the launcher are interpreted from the checkout root. Server-specific settings can live in Git-ignored `run.local.json`.

## Build and execution controls

| Key | Checked-in value | Meaning / CLI override |
| --- | --- | --- |
| `workflow` | `create-lund` | Create LUND files or submit completed files; `--workflow create-lund|submit` |
| `source` | `uniform` | LUND input mode; `--source uniform|physical` |
| `git_pull` | `false` | Request a clean-checkout `git pull --ff-only`; `--git-pull true` |
| `build` | `true` | Configure and incrementally build both applications; `--build false` reuses binaries |
| `run` | `true` | Execute the selected workflow after preceding stages; `--run false` stops after build/tests |
| `test` | `false` | Enable tests during configuration and run CTest; `--test true` |
| `build_dir` | `build/release` | CMake binary directory; `--build-dir` |
| `build_type` | `Release` | Debug, Release, RelWithDebInfo or MinSizeRel; `--build-type` |
| `jobs` | `4` | Positive build parallelism; `--jobs` |

## Workflow arguments

`arguments` maps workflow names to lists of strings. Each option and value occupy separate list entries; a path containing spaces stays in one string. These lists are not shell programs. Use `--key value`, not `--key=value`; duplicate option keys are rejected.

| Entry | Checked-in behavior |
| --- | --- |
| `arguments.uniform` | Use the electron sample profile and write to `runs/default-uniform` |
| `arguments.physical` | Use `config/samples/genie.conf`; supply the actual event-generator input/output as needed |
| `arguments.submit` | Empty; supply simulation inputs and a Slurm site profile |

For example, from csh/tcsh at the checkout root:

```tcsh
source run.csh --test true --run false
source run.csh --output runs/electron-next
```

## Outputs and failure behavior

The default creates a uniform electron LUND run. A validated existing run directory is removed and recreated, preserving legacy behavior. Failed builds/tests stop execution; a sourced shell remains open and receives a failure status. Submission previews until `--execute` is forwarded. `run.csh` intentionally cleans, resets, and pulls the disposable ifarm clone before dispatch; see [SSH workflow](../docs/ssh-workflow.md).
