# Local editing and SSH execution

Edit and validate the checkout locally, commit the changes, then transfer them through your normal Git remote or file-copy workflow. On the SSH server, load the site's compiler, ROOT, CMake and Python 3.9+ environment. For detector processing also load GEMC and reconstruction, and use a shared filesystem visible to workers. The launcher inherits this environment; it does not SSH, choose modules, or install software.

## Sourced entry point

In a **csh/tcsh** session on the server, from the repository root:

```tcsh
source run.csh
```

The server checkout is intentionally disposable. Before building, `run.csh` verifies the repository root, removes untracked files except the documented build exclusions, resets tracked changes, and pulls the remote revision. Commit and push every valuable edit from the local VS Code/GitHub clone first. It then reads `config/run.local.json` if present, otherwise `config/run.json`, builds, and dispatches the selected workflow. Existing resolved run directories are recreated as in the legacy generators.

```tcsh
source run.csh --output runs/electron-002
source run.csh --test true --run false
source run.csh --workflow create-lund --source uniform --config config/samples/uniform-neutron-sampled.conf --output runs/en-001
source run.csh --workflow create-lund --source physical --event-generator genie \
  --input '/data/genie/*.root' --output runs/physical
```

GENIE glob patterns must be quoted so they reach ROOT unchanged. Sample options override the selected sample configuration; launcher arguments also override matching defaults in the JSON argument list. All workflow paths are interpreted from the repository root, including when the wrapper is launched elsewhere. This differs from directly invoking the C++ executables, which use the caller's directory.

Use `source run.csh --help` for launcher options. Use `source run.csh --build false -- --help` for the selected executable's help. Bash users can execute `./run.csh` with tcsh installed, or call `python3 scripts/workflow.py`; do not source csh syntax into Bash.

To source from another directory, first set `CLAS12_SAMPLES_DIR` to the absolute checkout path:

```tcsh
setenv CLAS12_SAMPLES_DIR /shared/path/CLAS12-sample-generator
source "$CLAS12_SAMPLES_DIR/run.csh" --output runs/electron-003
```

A failed command stops subsequent stages and returns a nonzero `$status` without exiting the sourced parent shell. `CLAS12_SAMPLE_STATUS` also retains the wrapper's result. Read `$status` immediately because the next shell command replaces it. `CLAS12_SKIP_SERVER_SYNC=1` is reserved for local tests and launcher development; routine ifarm use must keep synchronization enabled.

## Run settings and build controls

Copy `config/run.json` to the Git-ignored `config/run.local.json` for server-specific defaults. Alternatively pass `--run-settings path/to/settings.json`. Keep each option and its value as separate strings in `arguments.uniform`, `arguments.physical`, or `arguments.submit`; values containing spaces remain one string. These are argv lists, not shell commands. Use `--key value` syntax, not `--key=value`.

| JSON key | Default | CLI override and purpose |
| --- | --- | --- |
| `workflow` | `create-lund` | `--workflow create-lund|submit` |
| `source` | `uniform` | `--source uniform|physical` for LUND creation |
| `git_pull` | `false` | `--git-pull true`: require a clean checkout, then `git pull --ff-only` |
| `build` | `true` | `--build false`: reuse existing binaries or skip compilation for simulation |
| `run` | `true` | `--run false`: build/test only |
| `test` | `false` | `--test true`: enable BUILD_TESTING and run CTest before execution |
| `build_dir` | `build/release` | `--build-dir build/debug` or an absolute path |
| `build_type` | `Release` | `--build-type Debug` (also Release, RelWithDebInfo, MinSizeRel) |
| `jobs` | `4` | `--jobs 8`: positive build parallelism |
| `arguments` | Per-workflow lists | Forwarded CLI options replace matching list options |

Building always invokes CMake dependency checking, so replacing an uncommitted `src/common/external/targets.h` is sufficient to trigger rebuilding. With `--test false`, the launcher configures BUILD_TESTING=OFF; use `--build true --test true` to enable tests again. `--build false --test true` requires an already configured test build.

After transferring committed changes to the remote, a server refresh/build/test is:

```tcsh
source run.csh --test true --run false
```

The refresh requires a configured Git upstream and intentionally discards server-side edits and untracked files, retaining the updater's documented build exclusions. It stops before building if cleanup, reset, or pull fails.

## Detector processing and submission

For example, generate a 2 GeV sample, then preview outbending processing:

```tcsh
source run.csh --workflow create-lund --source uniform --beam-energy 2.07052 \
  --prefix Uniform_1e_sample_2070MeV --output runs/electron-2gev
source run.csh --workflow submit --build false \
  --manifest runs/electron-2gev/Uniform_sample_1e_2070MeV/manifest.json \
  --gcard config/detector/Generation_files_2GeV/5.14/rgm_fall2021_Ar_2GeV.gcard \
  --reconstruction config/detector/Generation_files_2GeV/5.14/rgm_fall2021-cv.yaml \
  --site config/sites/local.json --torus 0.5 --solenoid -1
```

Select the actual reconstruction YAML path from your checkout. Submission previews the Slurm array until `--execute` is supplied. The local simulation runner is an internal array-worker/validation component, not a third user-facing workflow. See [simulation and Slurm](gemc-reconstruction-batch-submission.md) for complete options and [external inputs](external-inputs.md) for the required 2/4/6 GeV fields.

## Supporting shell files

`run.csh` owns the intentional disposable-clone refresh and forwards to `scripts/workflow.py`. `scripts/build_and_run.csh` uses the same driver without the refresh. `scripts/code_updater.sh` performs the checked clean/reset/pull sequence in a child shell. `scripts/printers/` supplies project start/success/failure banners.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
