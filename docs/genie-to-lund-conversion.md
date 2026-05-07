# GENIE-to-LUND Conversion

## Purpose

This workflow converts GENIE `gst` ROOT trees into LUND text files for later GEMC processing.

Primary entry points:

- `GEMC-samples/GENIE_to_LUND_converter.csh`
- `GEMC-samples/GENIE_to_LUND_converter/GENIE_to_LUND_converter.C`

## How It Is Intended To Run

The wrapper script assumes it is launched from inside `GEMC-samples/`.

```bash
cd GEMC-samples
csh GENIE_to_LUND_converter.csh
```

The wrapper:

- sets the target, GENIE tune, beam energy, and Q2 cut
- locates input `.root` files under a hardcoded truth-sample directory
- invokes the ROOT macro `GENIE_to_LUND_converter.C`

## Inputs

The current wrapper is configured around:

- `BASE_TL_SAMPLE_DIR`
- `TL_SAMPLE_TARGET_NUCLEUS`
- `TL_GENIE_TUNE`
- `TL_SAMPLE_ENERGY`
- `TL_SAMPLE_Q2_CUT`

The macro expects GENIE `gst` trees and writes LUND files in batches of `10000` events per output file.

## Verified Issues In This Checkout

- `GENIE_to_LUND_converter.csh` depends on `BASE_TL_SAMPLE_DIR=/w/hallb-scshelf2102/...`, which does not exist locally.
- `GENIE_to_LUND_converter.C` includes absolute external headers and source files under `/w/hallb-scshelf2102/...`, so it does not compile in this environment.
- `GENIE_to_LUND_converter.C` currently hardcodes the output suffix `_devGEMC_rgm_fall2021_Ar`, which is incorrect for non-Ar targets.

## Practical Consequence

In the current local environment, the wrapper exits on the missing truth-sample directory before conversion begins. If that path issue is bypassed, the ROOT macro still fails to compile because its external dependencies are not present.

## Suggested Fixes

- Move site-specific input and output paths into environment variables.
- Replace absolute framework includes with configurable include paths or local dependencies.
- Derive the output target variation from the selected target and beam energy instead of hardcoding the Ar variation.
