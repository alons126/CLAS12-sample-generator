# LUND-creation examples

These examples show supported option combinations beyond the minimal quickstart. Commands use small counts unless explicitly marked as production-style. Existing resolved run directories are replaced after a warning.

Commands using `run.csh` are for a disposable ifarm checkout. Before a workflow runs, the launcher removes untracked files except the reusable `build/` tree, discards tracked changes, pulls the configured remote branch, and updates submodules. Commit and push valuable changes from a development checkout first. For local development, build with CMake and use the direct executables shown below.

## Build without creating output

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --build true \
    --run false \
    --build-type Debug \
    --jobs 4
```

## Uniform samples

### Uniform 1e from a reviewed profile

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-4029MeV.conf \
    --events 10000 \
    --events-per-file 2500 \
    --seed 67890 \
    --vertex-seed 12345 \
    --output runs/uniform
```

### Explicit electron–proton FD study

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --channel eh \
    --hadron proton \
    --hadron-region FD \
    --beam-energy 5.98636 \
    --target Ar40 \
    --hadron-momentum mixed \
    --hadron-p-min 0.3 \
    --hadron-theta-min 5 \
    --hadron-theta-max 45 \
    --trigger-theta 25 \
    --events 1000 \
    --output runs/uniform
```

### Fixed-momentum neutron study

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

### Electron angular tester

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/electron-tester-5986MeV.conf \
    --events 5000 \
    --prefix tester-5986 \
    --output runs/tester
```

The tester remains at beam momentum and scans 5–40° with full azimuth while sampling the configured target geometry.

## Physical samples

### GENIE conversion with explicit provenance

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
    --input '/shared/truth/C12/GEM21_11a_00_000/*.root' \
    --events 50000 \
    --events-per-file 10000 \
    --output runs/physical
```

The physical LUND converter copies supported GST truth, selects only the vertex position, and records the unsanitized provenance in the manifest. `q2-cut` labels the upstream selection; the physical LUND converter does not apply a Q² cut. This 6 GeV carbon example uses the four-foil target; the small and large one-foil targets belong to 2 and 4 GeV running, with run 15733 as the small-foil 4 GeV exception[^sportes-2026-rgm][^rgm-analysis-note].

## Direct executable help and execution

After building, bypass the launcher when debugging application options locally:

```bash
build/debug/apps/uniform-lund-creator --help
build/debug/apps/event-generator-to-lund-converter --help

build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-enFD-2070MeV.conf \
    --events 100 \
    --output runs/direct
```

Direct executable paths are interpreted from the caller's current directory. The `run.csh` workflow instead anchors paths to the repository root and supplies the ifarm synchronization/build stages.

For the full production matrix, see the checked-in [uniform creation command list](../../tutorials/lund-creation/uniform-lund-creation.txt). The separate [uniform all-options tutorial](../../tutorials/lund-creation/uniform-all-options.txt) and [physical all-options tutorial](../../tutorials/lund-creation/physical-all-options.txt) reproduce the documented examples and demonstrate every public creation option with additional arbitrary studies.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
