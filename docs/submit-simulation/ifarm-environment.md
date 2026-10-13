# Ifarm environment and checkout model

Edit, build, and review changes in a development checkout. Commit and push them through the collaboration's normal Git workflow, then run production commands from an ifarm checkout on storage visible to Slurm workers. The launcher does not SSH to ifarm or install site software.

Obtain an ifarm login and permission to submit under the `clas12` Slurm account before production use. Follow Jefferson Lab’s [farm batch instructions](https://scicomp.jlab.org/docs/farm_slurm_batch) for site access and scheduler support. The account and partition required by this worker are listed [below](#scheduler-resources).

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

### Check commands before launching

In the configured csh/tcsh login shell, confirm that the programs are available:

```tcsh
which \
    git \
    tcsh \
    python3 \
    cmake \
    root-config \
    modulecmd \
    sbatch
module list
module show gemc/5.14
module show coatjava/10.0.7
```

Use the [version checks](../getting-started/installation.md#verify-prerequisites) for Python, CMake, and ROOT before LUND creation. Submission needs `modulecmd` in `PATH`, the directories searched for executables, because the coordinator calls that program from Python. An interactive `module` alias or function alone is insufficient. If a command or release is missing, repair the login setup or consult site support before invoking the destructive launcher. These checks do not submit jobs or clean the checkout.

### Scheduler resources

The [worker script](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh) supplies these Slurm directives for every task:

| Resource | Worker setting |
| --- | --- |
| Account | `clas12` |
| Partition | `production` |
| Nodes | 1 |
| Tasks | 1 |
| Memory per CPU | `--mem-per-cpu=2000` (Slurm's default unit is megabytes) |
| Time limit | `20:00:00` (20 hours) |

The public submission interface sets the job name and array size, but exposes no account, partition, memory, or time-limit options. Review any inherited `SBATCH_*` environment overrides as part of your site setup; do not assume they are recorded as resolved resource allocations in the Slurm submission log. Use Slurm's job information to inspect the accepted allocation.

If Slurm rejects the account or partition, contact site support or the collaboration's allocation administrator. For memory or time-limit failures, retain the task logs and consult the workflow maintainer about resource requirements or a smaller per-file workload. A worker resource change needs review; changing `--num-jobs` alone does not reduce the events in each selected file. See [troubleshooting](../getting-started/troubleshooting.md).

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

Bash users must first ensure the required environment variables and command paths are present in the child tcsh environment. They may execute [`./run.csh`](../../run.csh) when tcsh is installed, but must not source csh syntax into Bash.

## Run from another directory

When the shell is not in the repository root, point the launcher to the checkout:

```tcsh
unset CLAS12_SAMPLES_DIR
unsetenv CLAS12_SAMPLES_DIR
setenv CLAS12_SAMPLES_DIR /path/to/CLAS12-sample-tools
source "$CLAS12_SAMPLES_DIR/run.csh" \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

`CLAS12_SAMPLES_DIR` is a user-owned override and remains in that shell until removed with `unsetenv CLAS12_SAMPLES_DIR`.

## Exit status

Using `source` runs the script in your current shell. If the script handles a failure, it returns a nonzero `$status` instead of closing that shell. Zero means success; a nonzero value means failure. Read `$status` immediately, because the next command replaces it. `CLAS12_SAMPLE_STATUS` also stores the script's result.

`CLAS12_SKIP_SERVER_SYNC=1` exists only for launcher development. Routine ifarm operation must leave synchronization enabled.
