# Local editing and SSH execution

Edit and validate the checkout locally, commit the changes, then transfer them through your normal Git remote or file-copy workflow. On the SSH server, load the site's compiler, ROOT, CMake and Python 3.9+ environment. For detector processing also load GEMC and reconstruction, and use a shared filesystem visible to workers. The launcher inherits this environment; it does not SSH, choose modules, or install software.

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

Use `source run.csh --help` for launcher options. Use `source run.csh --workflow create-lund --source uniform --build false -- --help` for the selected executable's help. Bash users can execute `./run.csh` with tcsh installed, or call `python3 scripts/workflow.py`; do not source csh syntax into Bash.

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

`config/run.json` contains only stable build/test controls. Use `--run-settings path/to/settings.json` to select another strict JSON build profile explicitly. Workflow, source, sample configuration, input and output remain visible on the command line. Use `--key value` syntax, not `--key=value`.

There is no automatic `config/run.local.json`. The normal ifarm refresh removes untracked files, so an implicit local profile could disappear immediately before execution. Keep a reusable alternative profile in a deliberate location and select it with `--run-settings FILE`.

| JSON key | Default | CLI override and purpose |
| --- | --- | --- |
| `build` | `true` | `--build false`: reuse existing binaries or skip compilation for simulation |
| `run` | `true` | `--run false`: build/test only |
| `test` | `false` | `--test true`: enable BUILD_TESTING and run CTest before execution |
| `build_dir` | `build/release` | `--build-dir build/debug` or an absolute path |
| `build_type` | `Release` | `--build-type Debug` (also Release, RelWithDebInfo, MinSizeRel) |
| `jobs` | `4` | `--jobs 8`: positive build parallelism |

`--workflow create-lund|submit` is required. `--source uniform|physical` is required for `create-lund` and invalid for `submit`.

`workflow.py` separates its options from child options with `parse_known_args()`. It consumes the workflow, source, run-profile, and build/test flags. It forwards every other token unchanged to the selected executable or submitter; a single bare `--` may mark the boundary and is removed before forwarding. The dispatch is:

| Selection | Child command |
| --- | --- |
| `--workflow create-lund --source uniform` | `BUILD/apps/clas12-uniform` |
| `--workflow create-lund --source physical` | `BUILD/apps/clas12-generator-to-lund` |
| `--workflow submit` | `python3 scripts/slurm/submit.py` |

Run-profile precedence is built-in defaults, then the selected strict JSON, then explicit launcher options. Sample-profile values have their own C++ precedence: application defaults, then `--config FILE`, then explicit sample options. Site JSON, the completed manifest, GCARD, and reconstruction YAML are submission inputs rather than launcher settings. This keeps build policy, sample physics, completed output inventory, server resources, and detector configuration independently reviewable.

Building always invokes CMake dependency checking, so replacing an uncommitted `src/common/external/targets.h` is sufficient to trigger rebuilding. With `--test false`, the launcher configures BUILD_TESTING=OFF; use `--build true --test true` to enable tests again. `--build false --test true` requires an already configured test build.

After transferring committed changes to the remote, a server refresh/build/test is:

```tcsh
source run.csh --workflow create-lund --source uniform --test true --run false
```

The refresh requires a configured Git upstream and network access to any not-yet-initialized submodule. It intentionally discards server-side edits and untracked files, retaining the updater's documented build exclusions. It stops before building if cleanup, reset, pull, submodule synchronization, or submodule checkout fails.

## Detector processing and submission

For example, generate a 2 GeV sample, then preview outbending processing:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-2070MeV.conf --output runs/electron-2gev
source run.csh --workflow submit --build false \
  --manifest runs/electron-2gev/Uniform_sample_1e_2070MeV/lundfiles/lund-gen-monitoring/lund-gen-log.json \
  --gcard config/detector/Generation_files_2GeV/5.14/rgm_fall2021_Ar_2GeV.gcard \
  --reconstruction config/detector/Generation_files_2GeV/5.14/rgm_fall2021-cv.yaml \
  --site config/sites/local.json --torus 0.5 --solenoid -1
```

Select the actual reconstruction YAML path from your checkout. Submission previews the Slurm array until `--execute` is supplied. The local simulation runner is an internal array-worker/validation component, not a third user-facing workflow. See [simulation and Slurm](gemc-reconstruction-batch-submission.md) for complete options and [external inputs](external-inputs.md) for the required 2/4/6 GeV fields.

## Supporting shell files

`run.csh` owns the intentional disposable-clone refresh and forwards to `scripts/workflow.py`. `scripts/build_and_run.csh` uses the same driver without the refresh. `scripts/code_updater.sh` performs the checked clean/reset/pull/submodule-update sequence in a child shell. `scripts/printers/` supplies project start/success/failure banners.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
