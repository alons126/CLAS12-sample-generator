# Local editing and SSH execution

Edit and validate the checkout locally, commit the changes, then transfer them through your normal Git remote or file-copy workflow. On the SSH server, load the site's compiler, ROOT, CMake and Python 3.9+ environment. For submission, initialize the login shell’s module command and reconstruction environment and use shared storage visible to workers. The sourced setup script loads the configured GEMC version. The launcher does not SSH or install software.

## Sourced entry point

In a **csh/tcsh** session on the server, select the workflow, source, sample profile and output explicitly:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf --output runs/electron-001
```

The server checkout is intentionally disposable. Before building, `run.csh` verifies the repository root, removes untracked files except the documented build exclusions, resets tracked changes, pulls the remote revision, and runs `git submodule sync --recursive` followed by `git submodule update --init --recursive`. The latter checks out `legacy/Uniform-sample-generator` at the exact revision pinned by the main repository. Commit and push every valuable edit from the local VS Code/GitHub clone first. It then reads build/test defaults from `config/run.json` and dispatches the action written in the command. Existing resolved run directories are recreated as in the legacy generators.

```tcsh
source run.csh --workflow create-lund --source uniform --config config/samples/uniform-enFD-5986MeV.conf --output runs/en-001
source run.csh --workflow create-lund --source physical --config config/samples/genie.conf \
  --input '/data/genie/*.root' --output runs/physical
source run.csh --workflow create-lund --source uniform --build true --test true --run false
```

GENIE glob patterns must be quoted so they reach ROOT unchanged. Options after `--config` override matching sample-profile values. `workflow.py` forwards child options exactly as written and does not inject a hidden sample profile or output path. All workflow paths are interpreted from the repository root, including when the wrapper is launched elsewhere. This differs from directly invoking the C++ executables, which use the caller's directory.

Use `source run.csh --help` for launcher options. Use `source run.csh --workflow create-lund --source uniform --build false -- --help` for the selected executable's help. Bash users can execute `./run.csh` with tcsh installed, or call `python3 src/launcher/workflow.py`; do not source csh syntax into Bash.

An empty `source run.csh` prints uniform, physical, submission, and build/test examples and returns status 2. Both the empty-command guidance and `source run.csh --help` run before the disposable-clone synchronization, so asking for usage does not clean, reset, pull, build, create output, or submit jobs. A nonempty command that omits `--workflow` receives the same examples from `workflow.py`.

To source from another directory, first set `CLAS12_SAMPLES_DIR` to the absolute checkout path:

```tcsh
setenv CLAS12_SAMPLES_DIR /shared/path/CLAS12-sample-generator
source "$CLAS12_SAMPLES_DIR/run.csh" --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf --output runs/electron-003
```

`CLAS12_SAMPLES_DIR` is an optional user-defined environment variable; the project does not create it. It overrides automatic checkout discovery so the launcher can be sourced from any working directory. When the shell is already in the repository root, it is unnecessary:

```tcsh
cd /shared/path/CLAS12-sample-generator
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf --output runs/electron-001
```

Because `setenv` stores the value in the current shell, it remains available for later commands and sessions descended from that shell. Remove it when it should no longer override checkout discovery:

```tcsh
unsetenv CLAS12_SAMPLES_DIR
```

A failed command stops subsequent stages and returns a nonzero `$status` without exiting the sourced parent shell. `CLAS12_SAMPLE_STATUS` also retains the wrapper's result. Read `$status` immediately because the next shell command replaces it. `CLAS12_SKIP_SERVER_SYNC=1` is reserved for local tests and launcher development; routine ifarm use must keep synchronization enabled.

## Run settings and build controls

`config/run.json` contains only LUND build/test controls; submission does not read it. Use `--run-settings path/to/settings.json` to select another strict JSON build profile explicitly. Workflow, source, sample configuration, input and output remain visible on the command line. Use `--key value` syntax, not `--key=value`.

There is no automatic `config/run.local.json`. The normal ifarm refresh removes untracked files, so an implicit local profile could disappear immediately before execution. Keep a reusable alternative profile in a deliberate location and select it with `--run-settings FILE`.

| JSON key | Default | CLI override and purpose |
| --- | --- | --- |
| `build` | `true` for `create-lund` | `--build true` or `--build false`: explicitly select compilation behavior |
| `run` | `true` | `--run false`: build/test only |
| `test` | `false` | `--test true`: enable BUILD_TESTING and run CTest before execution |
| `build_dir` | `build/release` | `--build-dir build/debug` or an absolute path |
| `build_type` | `Release` | `--build-type Debug` (also Release, RelWithDebInfo, MinSizeRel) |
| `jobs` | `4` | `--jobs 8`: positive build parallelism |

`--workflow create-lund|submit` is required. `--source uniform|physical` is required for `create-lund` and invalid for `submit`.

`workflow.py` separates build options from LUND application options and preserves argument boundaries. Submission instead sources `src/slurm-submission/setup_and_submit.csh` directly; its helper accepts `--lund-dir`, `--config` and CLI overrides without invoking the LUND build driver. See the [submission guide](gemc-reconstruction-batch-submission.md).

Building always invokes CMake dependency checking, so replacing an uncommitted `src/lund-generation/external/targets.h` is sufficient to trigger rebuilding. With `--test false`, the launcher configures BUILD_TESTING=OFF; use `--build true --test true` to enable tests again. `--build false --test true` requires an already configured test build.

After transferring committed changes to the remote, a server refresh/build/test is:

```tcsh
source run.csh --workflow create-lund --source uniform --test true --run false
```

The refresh requires a configured Git upstream and network access to any not-yet-initialized submodule. It intentionally discards server-side edits and untracked files, retaining the updater's documented build exclusions. It stops before building if cleanup, reset, pull, submodule synchronization, or submodule checkout fails.

## Detector processing and submission

First create the LUND files. Pass the completed LUND directory; use optional config/CLI overrides for detector settings. Commit and push any in-checkout configuration changes locally first. Then, from a csh/tcsh login shell on ifarm:

```tcsh
source run.csh --workflow submit --lund-dir /shared/sample/lundfiles
```

This previews setup and the Slurm command. Add `--execute` to submit the selected arrays and replace simulation output directories, preserving LUND input. Preview still performs the documented server-checkout refresh and environment loading, but preserves sample outputs and farm logs. The setup checks inputs and prints the legacy report before calling `sbatch`. Tests intercept that final call in temporary fixtures. See the [submission guide](gemc-reconstruction-batch-submission.md) for settings and failure behavior.

## Supporting shell files

`run.csh` owns the disposable-clone refresh, then sources submission or calls the LUND Python driver. `src/launcher/build_and_run.csh` is a LUND build helper without refresh. `src/launcher/code_updater.csh` performs checked Git operations in a child shell. Submission is sourced so login-shell module aliases and environment updates remain available to `sbatch`.
