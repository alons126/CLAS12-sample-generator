# Ifarm environment and checkout model

Edit, build, and review changes in a development checkout. Commit and push them through the collaboration's normal Git workflow, then run production commands from an ifarm checkout on storage visible to Slurm workers. The launcher does not SSH to ifarm or install site software.

## Login environment

Keep this setup in `~/environment.csh`:

```tcsh
#!/bin/csh
module use /scigroup/cvmfs/hallb/clas12/sw/modulefiles
module purge
module load sqlite/dev
module load clas12
module switch coatjava/10.0.7
```

See [RG-M repository](https://github.com/awild7/rgm) for any updates for `~/environment.csh`. Source it from `~/.cshrc` so every ifarm login receives the expected CLAS12 environment:

```tcsh
source ~/environment.csh
```

This prepares the login environment. Submission then explicitly unloads and loads its requested GEMC module, switches COATJAVA with `module switch coatjava/<coatjava-version>`, and verifies both selections in a private child environment, defaulting to GEMC 5.14 and COATJAVA 10.0.7. Use `--gemc-version` and `--coatjava-version`, or their configuration-file keys, to change those selections. A different login-shell release does not override submission defaults, and submission does not alter the interactive shell. See the [COATJAVA repository](https://github.com/JeffersonLab/coatjava) for its source and other releases. Changing a release requires compatible configuration files and campaign validation.

## Disposable checkout

For a normal workflow command, `run.csh` verifies the checkout, removes untracked and ignored files except the checkout's `build/` tree, discards tracked changes, pulls the configured upstream branch, and synchronizes submodules. This is intentional. Never keep the only copy of code, configuration, or output inside the ifarm checkout. A custom build directory outside `build/` is not protected by that exclusion.

Place LUND and HIPO data on shared storage outside the checkout. Commit and push valuable repository changes before invoking `run.csh`.

Only limited checks happen before synchronization. A bare `source run.csh --help` returns without updating. Submission help and argument-syntax checks also run early when `--workflow submit` (or `--workflow=submit`) is the first option. Full submission input checks and LUND-creation option validation happen after synchronization. Forwarded LUND help (`-- --help`) also runs after synchronization; use the compiled application's `--help` directly in a development checkout.

Use a csh/tcsh login shell:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

Bash users may execute `./run.csh` when tcsh is installed, but must not source csh syntax into Bash.

## Run from another directory

When the shell is not in the repository root, point the launcher to the checkout:

```tcsh
unset CLAS12_SAMPLES_DIR
unsetenv CLAS12_SAMPLES_DIR
setenv CLAS12_SAMPLES_DIR /path/to/CLAS12-sample-generator
source "$CLAS12_SAMPLES_DIR/run.csh" \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

`CLAS12_SAMPLES_DIR` is a user-owned override and remains in that shell until removed with `unsetenv CLAS12_SAMPLES_DIR`.

## Exit status

Because the entry point is sourced, a handled failure returns a nonzero `$status` without closing the login shell. Read `$status` immediately; the next shell command replaces it. `CLAS12_SAMPLE_STATUS` also retains the wrapper result.

`CLAS12_SKIP_SERVER_SYNC=1` exists only for launcher development. Routine ifarm operation must leave synchronization enabled.
