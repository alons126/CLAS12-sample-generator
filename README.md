# CLAS12 sample generator

One compiled project for two sources of CLAS12 simulation input:

- **Uniform samples** for acceptance-map studies: electron, electron–proton and electron–neutron channels.
- **Physical samples**: conversion of existing GENIE `gst` ROOT trees to LUND.

Both write LUND files and a run manifest. The same runner sends those files through GEMC and reconstruction, locally or through Slurm. This repository does not run the GENIE event generator itself or calculate final acceptance maps.

## First build and sample

Requirements: CMake 3.20+, a C++ compiler compatible with your ROOT installation, ROOT with Core/RIO/Hist/Physics/Tree/TreePlayer, and Python 3.9+ for tests and simulation scripts. GEMC and `recon-util` are needed only when executing simulation.

```bash
cmake --preset debug
cmake --build --preset debug --parallel 4
ctest --preset debug

build/debug/apps/clas12-uniform \
  --config config/samples/uniform-electron.conf \
  --events-per-file 100 \
  --output runs/first-electron
```

The output directory must be new. Nothing runs `git clean`, deletes previous samples, or submits jobs as part of building or generating a sample.

Open `runs/first-electron/manifest.json` to see the resolved settings and output counts. LUND text is under `lundfiles/`; diagnostic histograms are in `monitoring.root`.

## Where to start

Read the [newcomer guide](docs/index.md), then [build instructions](docs/building.md) and the [architecture walkthrough](docs/architecture.md).

- [Uniform generation](docs/uniform-samples.md)
- [GENIE conversion](docs/genie-to-lund-conversion.md)
- [GEMC, reconstruction and Slurm](docs/gemc-reconstruction-batch-submission.md)
- [Configuration reference](docs/configuration.md)
- [Migration from the imported repositories](docs/migration.md)

The original source trees are retained in `legacy/` for comparison. They are retired and excluded from the build; use the commands documented above. Detector cards and reconstruction YAML are retained in `config/detector/`.
