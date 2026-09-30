# Workflow command examples

The command lists are grouped by user-facing workflow:

- [`launcher-options.txt`](launcher-options.txt) demonstrates every launcher-owned build/run option, help routing, child-option forwarding, and submission selection.
- [`uniform-lund-creation.txt`](lund-creation/uniform-lund-creation.txt) creates nine uniform LUND samples: `1e`, `enFD`, and `epFD` at 2070, 4029, and 5986 MeV.
- [`uniform-all-options.txt`](lund-creation/uniform-all-options.txt) demonstrates every uniform LUND creator option through valid arbitrary studies and the examples from the LUND-creation documentation.
- [`uniform-slurm-submission.txt`](slurm-submission/uniform-slurm-submission.txt) consumes those completed LUND directories and submits the corresponding GEMC and reconstruction jobs.
- [`genie-gst-lund-creation.txt`](lund-creation/genie-gst-lund-creation.txt) converts physical C12 GENIE GST samples at 2070, 4029, and 5986 MeV, plus the run-15733 target-variation exception.
- [`physical-all-options.txt`](lund-creation/physical-all-options.txt) demonstrates every physical LUND converter option, an arbitrary metadata-layout study, and the documented provenance examples.
- [`genie-gst-slurm-submission.txt`](slurm-submission/genie-gst-slurm-submission.txt) consumes those completed physical LUND directories and submits the corresponding GEMC and reconstruction jobs.
- [`all-options.txt`](slurm-submission/all-options.txt) demonstrates every public submission option for manifest-backed and manually described uniform and physical inputs.

Run the commands from the repository root in a csh/tcsh shell. LUND creation and
simulation submission remain separate workflows. Each submission example selects
only the first five completed LUND files, producing a five-task Slurm array.

Replace every `/path/to/...` value with a reviewed location that is visible in the
environment where the command will run. Keep LUND and simulation output on shared
storage when Slurm workers must read or write it.

The `all-options` files are interface inventories, not recommended production
profiles. Their arbitrary values are chosen to exercise validation and show syntax.
Use the checked-in sample profiles for reviewed production settings. A public option
is considered covered when at least one tutorial command shows its exact CLI spelling;
the tutorials do not attempt every possible value of each option.

Submission previews by default. The checked-in submission examples include
`--execute`; remove that flag to validate the resolved settings and print the
`sbatch` command without replacing simulation outputs or submitting jobs.

The submission workflow reads sample metadata, filename prefix, completed file
counts, and the default event limit from the LUND manifest. GEMC defaults to 5.14.
See the [submission guide](../docs/submit-simulation/guide.md) for
configuration overrides, validation, output replacement, and ifarm synchronization.

The GENIE examples assume each GST directory has this layout:

```text
SAMPLE_DIRECTORY/
├── input_options.txt
└── master-routine_validation_01-eScattering/
    └── *.root
```

With `tune = auto`, LUND conversion reads the exact `TUNE` entry from
`input_options.txt`. The shown resolved output paths assume that the metadata contains
`TUNE GEM21_11a_00_000` and that the default nested output layout is used.
