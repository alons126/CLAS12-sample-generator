# Workflow command examples

The tutorial text files are copyable command collections. They complement the Wiki: the Wiki explains behavior and safety; the tutorials show complete invocations.

## Start with the matching pair

| Work | Create | Submit |
| --- | --- | --- |
| Uniform 1e, epFD, and enFD at the three standard beams | [`lund-creation/uniform-lund-creation.txt`](lund-creation/uniform-lund-creation.txt) | [`slurm-submission/uniform-slurm-submission.txt`](slurm-submission/uniform-slurm-submission.txt) |
| C12 GENIE GST at the three standard beams and the run-15733 exception | [`lund-creation/genie-gst-lund-creation.txt`](lund-creation/genie-gst-lund-creation.txt) | [`slurm-submission/genie-gst-slurm-submission.txt`](slurm-submission/genie-gst-slurm-submission.txt) |

## Complete option demonstrations

- [`launcher-options.txt`](launcher-options.txt) shows every launcher-owned build/run option, workflow selection, help forwarding, and run-settings override.
- [`lund-creation/uniform-all-options.txt`](lund-creation/uniform-all-options.txt) shows every uniform LUND creator option.
- [`lund-creation/physical-all-options.txt`](lund-creation/physical-all-options.txt) shows every physical LUND converter option.
- [`slurm-submission/all-options.txt`](slurm-submission/all-options.txt) shows every public submission option, both manifest-backed and manually described input.

The arbitrary commands in the all-options files are interface demonstrations, not production recommendations. Use checked-in sample profiles for reviewed settings.

## Before running a command

Replace every `/path/to/...` placeholder with a reviewed path. LUND and HIPO data used by Slurm must live on storage visible to the workers. Quote physical-input globs so ROOT receives them unchanged.

Run `run.csh` commands from a csh/tcsh login shell on ifarm. The checkout is disposable: a normal workflow refresh discards tracked edits and removes untracked and ignored files except the checkout's `build/` tree. Commit and push valuable development changes first, and keep production output outside that checkout. For local development, build with CMake and run the compiled LUND executable directly.

Submission previews by default. Some production command lists include `--execute`; remove it to inspect settings and actions without replacing simulation output or calling `sbatch`.

The GENIE examples assume:

```text
SAMPLE_DIRECTORY/
├── input_options.txt
└── master-routine_validation_01-eScattering/
    └── *.root
```

With `tune = auto`, the physical LUND converter reads `TUNE` from `input_options.txt`. The shown nested output paths assume the value `GEM21_11a_00_000`.
