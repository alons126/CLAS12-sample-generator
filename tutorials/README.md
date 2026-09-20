# Workflow command examples

The example commands for both user-facing workflows live together in
[`example-commands/`](example-commands/):

- [`uniform-lund-generation.txt`](example-commands/uniform-lund-generation.txt) creates nine uniform LUND samples: `1e`, `enFD`, and `epFD` at 2070, 4029, and 5986 MeV.
- [`uniform-slurm-submission.txt`](example-commands/uniform-slurm-submission.txt) consumes those completed LUND directories and submits the corresponding GEMC and reconstruction jobs.

Run the commands from the repository root in a csh/tcsh shell. LUND creation and
simulation submission remain separate workflows. Each submission example selects
only the first five completed LUND files, producing a five-task Slurm array.

Submission previews by default. The checked-in submission examples include
`--execute`; remove that flag to validate the resolved settings and print the
`sbatch` command without replacing simulation outputs or submitting jobs.

The submission workflow reads sample metadata, filename prefix, completed file
counts, and the default event limit from the LUND manifest. GEMC defaults to 5.14.
See the [submission guide](../docs/gemc-reconstruction-batch-submission.md) for
configuration overrides, validation, output replacement, and ifarm synchronization.
