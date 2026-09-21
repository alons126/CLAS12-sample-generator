These four reports define the current submission printout for uniform and physical samples in preview and `--execute` modes. Each fixture uses two LUND files, one prior output in each simulation directory, explicit detector inputs, and a custom clas12Tags checkout. The report groups workflow settings, sample settings, output inventory, job settings, and the final sbatch command in that order.

The report fixtures originated from the working C-shell coordinator captured before its Python migration. The original migration comparison also verified Slurm arguments and configured exports. Current fixtures reflect the maintained Python report layout; the protected worker script and detector resources remain unchanged.

Tests remove ANSI escapes, replace the temporary checkout with `{CHECKOUT}`, and use `echo_style=both` to match the Linux/ifarm escape convention. These files are inert test data, not runnable setup scripts.
