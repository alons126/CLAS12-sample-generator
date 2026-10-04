# Workflow command examples

The tutorial text files are copyable command collections. They complement the Wiki: the Wiki explains behavior and safety; the tutorials show complete invocations.

For the explanation before the commands, follow the [getting-started reading order](../docs/getting-started/index.md). The shared [path notation](../docs/getting-started/outputs.md#path-notation) distinguishes the creation output parent from the run directory passed to submission.

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

The all-options files use deliberately arbitrary values to show how to spell and combine options. Do not treat those values as recommended physics settings. Start with the sample profiles included in the repository.

## Before running a command

Replace every `/path/to/...` placeholder with a reviewed path. LUND and HIPO data used by Slurm must live on storage visible to the workers. Quote physical-input globs so ROOT receives them unchanged.

Run [`run.csh`](../run.csh) from a csh/tcsh login shell on ifarm. Before starting the workflow, it discards uncommitted edits and deletes untracked and ignored files except `build/`. Commit and push development changes from your local copy first. Keep generated samples outside the ifarm repository directory so cleanup cannot delete them. For local development, build with CMake and run the compiled LUND application directly.

Submission previews by default. Some production command lists include `--execute`; remove it to inspect settings and actions without replacing simulation output or calling `sbatch`.

The GENIE examples using `tune = auto` assume samples generated with [`eAScatteringGridSubmitter.py`](https://github.com/GENIE-MC/Generator/blob/3a50ba6d0918f62b023194eb2d6b5267b2815868/src/scripts/production/python/eAScatteringGridSubmitter.py) using `--store-comitinfo` and the default production directory name. That GENIE-only option writes `input_options.txt`; it is not an option of this repository's launcher or physical LUND converter. Keep the saved metadata beside the event directory:

```text
SAMPLE_DIRECTORY/
├── input_options.txt
└── master-routine_validation_01-eScattering/
    └── *.root
```

With `tune = auto`, the physical LUND converter reads `TUNE` from that file or records `unknown` when lookup fails. The shown nested output paths assume the value `GEM21_11a_00_000`. For samples produced another way or stored in another layout, pass `--tune NAME` explicitly; see [automatic GENIE tune lookup](../docs/create-lund/physical.md#automatic-genie-tune-lookup).
