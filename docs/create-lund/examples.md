# LUND-creation examples

Use these as patterns. Checked-in profiles are the starting point for reviewed settings; command-line values intentionally override a profile for a particular run.

The [`run.csh`](../../run.csh) examples are for a disposable ifarm checkout: refresh discards tracked edits and removes untracked and ignored files except `build/`. Commit and push valuable changes first, and keep output outside that checkout. During local development, use the compiled application directly.

## Small uniform run on ifarm

This uses the [`uniform-enFD-2070MeV.conf`](../../config/samples/uniform-lund-creation/uniform-enFD-2070MeV.conf):

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-enFD-2070MeV.conf \
    --events 1000 \
    --output /path/to/uniform-output
```

## Explicit central-detector pion study

This mode is implemented but is not yet production-validated:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --channel eh \
    --hadron pip \
    --hadron-region CD \
    --beam-energy 5.98636 \
    --target Ar40 \
    --events 1000 \
    --output /path/to/uniform-output
```

## Physical conversion with explicit provenance

```tcsh
source run.csh \
    --workflow create-lund \
    --source physical \
    --event-generator genie-gst \
    --event-generator-version 3.6.2 \
    --tune GEM21_11a_00_000 \
    --q2-cut Q2-0.40 \
    --target C12 \
    --beam-energy 5.98636 \
    --input '/path/to/C12/GEM21_11a_00_000/*.root' \
    --events 50000 \
    --events-per-file 10000 \
    --output /path/to/physical-output
```

## Build without running

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --build true \
    --run false
```

For local debugging, call the built program directly. Relative paths then start from the caller's current directory; [`run.csh`](../../run.csh) anchors workflow paths to the repository root.

The [tutorial index](../../tutorials/README.md) links to the production command matrices and three all-options files. Those files demonstrate every public launcher, creation, and submission option, including arbitrary non-production values chosen only to show syntax.
