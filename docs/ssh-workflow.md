# Local editing and SSH execution

Edit and validate the checkout locally, commit the changes, then transfer them through your normal Git remote or file-copy workflow. On the SSH server, load the site's compiler, ROOT, CMake and Python 3.9+ environment. For detector processing also load GEMC and reconstruction, and use a shared filesystem visible to workers. The launcher inherits this environment; it does not SSH, choose modules, or install software.

## Sourced entry point

In a **csh/tcsh** session on the server, from the repository root:

```tcsh
source run.csh
```

This reads `config/run.local.json` if present, otherwise `config/run.json`; configures and incrementally builds Release in `build/release`; then generates the default 1000-event electron sample in `runs/default-uniform`. Git updates and tests are off by default. The output directory must be new, so choose another output for subsequent runs:

```tcsh
source run.csh --output runs/electron-002
source run.csh --test true --run false
source run.csh --workflow uniform --config config/samples/uniform-neutron-sampled.conf --output runs/en-001
source run.csh --workflow genie --input '/data/genie/*.root' --output runs/genie-001
```

GENIE glob patterns must be quoted so they reach ROOT unchanged. Sample options override the selected sample configuration; launcher arguments also override matching defaults in the JSON argument list. All workflow paths are interpreted from the repository root, including when the wrapper is launched elsewhere. This differs from directly invoking the C++ executables, which use the caller's directory.

Use `source run.csh --help` for launcher options. Use `source run.csh --build false -- --help` for the selected executable's help. Bash users can execute `./run.csh` with tcsh installed, or call `python3 scripts/workflow.py`; do not source csh syntax into Bash.

To source from another directory, first set `CLAS12_SAMPLES_DIR` to the absolute checkout path:

```tcsh
setenv CLAS12_SAMPLES_DIR /shared/path/CLAS12-sample-generator
source "$CLAS12_SAMPLES_DIR/run.csh" --output runs/electron-003
```

A failed command stops subsequent stages and returns a nonzero `$status` without exiting the sourced parent shell. `CLAS12_SAMPLE_STATUS` also retains the wrapper's result. Read `$status` immediately because the next shell command replaces it. Interrupted child execution reports failure too. Existing sample outputs are preserved for inspection.

## Run settings and build controls

Copy `config/run.json` to the Git-ignored `config/run.local.json` for server-specific defaults. Alternatively pass `--run-settings path/to/settings.json`. Keep each option and its value as separate strings in `arguments.uniform`, `arguments.genie`, `arguments.simulate`, or `arguments.submit`; values containing spaces remain one string. These are argv lists, not shell commands. Use `--key value` syntax, not `--key=value`.

| JSON key | Default | CLI override and purpose |
| --- | --- | --- |
| `workflow` | `uniform` | `--workflow uniform|genie|simulate|submit` |
| `git_pull` | `false` | `--git-pull true`: require a clean checkout, then `git pull --ff-only` |
| `build` | `true` | `--build false`: reuse existing binaries or skip compilation for simulation |
| `run` | `true` | `--run false`: build/test only |
| `test` | `false` | `--test true`: enable BUILD_TESTING and run CTest before execution |
| `build_dir` | `build/release` | `--build-dir build/debug` or an absolute path |
| `build_type` | `Release` | `--build-type Debug` (also Release, RelWithDebInfo, MinSizeRel) |
| `jobs` | `4` | `--jobs 8`: positive build parallelism |
| `arguments` | Per-workflow lists | Forwarded CLI options replace matching list options |

Building always invokes CMake dependency checking, so replacing an uncommitted `src/common/external/targets.h` is sufficient to trigger rebuilding. With `--test false`, the launcher configures BUILD_TESTING=OFF; use `--build true --test true` to enable tests again. `--build false --test true` requires an already configured test build.

After transferring committed changes to the remote, an optional server update/build/test is:

```tcsh
source run.csh --git-pull true --test true --run false
```

The update requires a configured Git upstream and refuses tracked or untracked local changes. It never resets or cleans files, and divergent history fails the fast-forward operation. Ignored local settings and generated output remain in place. With the default `--git-pull false`, locally edited code builds directly.

## Detector processing and submission

For example, generate a 2 GeV sample, then preview outbending processing:

```tcsh
source run.csh --workflow uniform --beam-energy 2.07052 \
  --prefix Uniform_1e_sample_2070MeV --output runs/electron-2gev
source run.csh --workflow simulate --build false \
  --manifest runs/electron-2gev/manifest.json \
  --gcard config/detector/Generation_files_2GeV/5.14/rgm_fall2021_Ar_2GeV.gcard \
  --reconstruction config/detector/Generation_files_2GeV/5.14/rgm_fall2021-cv.yaml \
  --site config/sites/local.json --torus 0.5 --solenoid -1
```

Select the actual reconstruction YAML path from your checkout. Use `--workflow submit` and the server's site JSON to preview a Slurm array. Both workflows remain dry runs until `--execute` is supplied. See [simulation and Slurm](gemc-reconstruction-batch-submission.md) for complete options and [external inputs](external-inputs.md) for the required 2/4/6 GeV fields.

## Supporting shell files

`run.csh` forwards to `scripts/workflow.py`. `scripts/build_and_run.csh` uses the same driver with Git pulling disabled. `update_only.sh` and `scripts/code_updater.sh` are csh/tcsh wrappers despite the `.sh` suffix; sourcing either performs only the checked fast-forward update. `scripts/printers/` supplies project start/success/failure banners. These replace the copied analyzer paths, destructive Git cleanup and unrelated environment setup. They are source-checkout helpers, not installed commands.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
