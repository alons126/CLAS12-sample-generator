# LUND-creation examples

These examples show supported option combinations beyond the minimal quickstart. Commands use small counts unless explicitly marked as production-style. Existing resolved run directories are replaced after a warning.

## Build and test without creating output

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --build true \
  --test true \
  --run false \
  --build-type Debug \
  --jobs 4
```

## Uniform 1e from a reviewed profile

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --config config/samples/uniform-1e-4029MeV.conf \
  --events 10000 \
  --events-per-file 2500 \
  --seed 67890 \
  --vertex-seed 12345 \
  --output runs/uniform
```

## Explicit electron–proton FD study

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --channel eh \
  --hadron proton \
  --hadron-region FD \
  --beam-energy 5.98636 \
  --rgm-target Ar40 \
  --hadron-momentum mixed \
  --hadron-p-min 0.3 \
  --hadron-theta-min 5 \
  --hadron-theta-max 45 \
  --trigger-theta 25 \
  --events 1000 \
  --output runs/uniform
```

## Fixed-momentum neutron study

Fixed momentum is intentionally neutron-only:

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --channel eh \
  --hadron neutron \
  --hadron-region CD \
  --hadron-momentum fixed \
  --hadron-p 1 \
  --events 1000 \
  --output runs/uniform
```

## Electron angular tester

```tcsh
source run.csh \
  --workflow create-lund \
  --source uniform \
  --config config/samples/electron-tester-5986MeV.conf \
  --events 5000 \
  --prefix tester-5986 \
  --output runs/tester
```

The tester remains at beam momentum and scans 5–40° with full azimuth while sampling the configured target geometry.

## GENIE conversion with explicit provenance

```tcsh
source run.csh \
  --workflow create-lund \
  --source physical \
  --event-generator genie-gst \
  --event-generator-version 3.2.2 \
  --tune GEM21_11a_00_000 \
  --q2-cut Q2_0_40 \
  --gemc-version 5.14 \
  --rgm-target C12-small \
  --beam-energy 5.98636 \
  --input '/shared/truth/C12/GEM21_11a_00_000/*.root' \
  --events 50000 \
  --events-per-file 10000 \
  --output runs/physical
```

The converter copies supported GST truth, samples only the vertex, and records the unsanitized provenance in the manifest. `q2-cut` labels the upstream selection; the converter does not apply a Q² cut.

## Direct executable help and execution

After building, bypass the launcher when debugging application options locally:

```bash
build/debug/apps/uniform-lund-generator --help
build/debug/apps/event-generator-to-lund-converter --help

build/debug/apps/uniform-lund-generator \
  --config config/samples/uniform-enFD-2070MeV.conf \
  --events 100 \
  --output runs/direct
```

Direct executable paths are interpreted from the caller's current directory. The `run.csh` workflow instead anchors paths to the repository root and supplies the ifarm synchronization/build stages.

For the full production matrix, see the checked-in [uniform generation command list](../../tutorials/uniform-samples/uniform-lund-generation.txt).
