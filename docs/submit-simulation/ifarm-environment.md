# Ifarm environment and checkout model

Edit, build, and review changes in a development checkout. Commit and push them through the collaboration's normal Git workflow, then run production commands from an ifarm checkout on storage visible to Slurm workers. The launcher does not SSH to ifarm or install site software.

## Login environment

Keep this setup in `~/environment.csh`, a file in your ifarm home directory. The `module` commands select installed software versions and set the paths and environment variables needed to run them:

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

Use the ifarm repository copy to run code, not to keep development edits. Before a normal workflow starts, [`run.csh`](../../run.csh) checks the repository location, deletes untracked and ignored files except `build/`, discards uncommitted edits to tracked files, and pulls the configured Git branch. It updates external repositories, called submodules, only when `.gitmodules` declares them. The published application files do not require that file or any submodule checkout.

For example, an untracked profile you saved inside the ifarm checkout can be deleted before the workflow tries to read it. A custom build directory is also deleted unless it is inside `build/`. Keep the only copy of valuable code, configuration, and output elsewhere.

Place LUND and HIPO data on shared storage outside the checkout. Commit and push valuable repository changes before invoking [`run.csh`](../../run.csh).

The cleanup can happen even when the requested workflow later fails its checks. Only a few checks run first: bare [`source run.csh --help`](../../run.csh) returns without cleanup or updating, and submission help and argument-syntax checks run early when `--workflow submit` (or `--workflow=submit`) is the first option. Full input and sample-setting checks run after cleanup and updating. Forwarded LUND help (`-- --help`) also runs afterward. In a development checkout, use the compiled application's `--help` directly.

Use a csh/tcsh login shell:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

Bash users may execute [`./run.csh`](../../run.csh) when tcsh is installed, but must not source csh syntax into Bash.

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

Using `source` runs the script in your current shell. If the script handles a failure, it returns a nonzero `$status` instead of closing that shell. Zero means success; a nonzero value means failure. Read `$status` immediately, because the next command replaces it. `CLAS12_SAMPLE_STATUS` also stores the script's result.

`CLAS12_SKIP_SERVER_SYNC=1` exists only for launcher development. Routine ifarm operation must leave synchronization enabled.
