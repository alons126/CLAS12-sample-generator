# CLAS12 sample generator

This project prepares samples for the $e4\nu$ collaboration’s electron-scattering studies with the CLAS12 spectrometer[^clas12-spectrometer] at Jefferson Lab. The project provides two separate user-facing workflows:

| Workflow | Purpose | Result |
| --- | --- | --- |
| `lund-creation` | Create [LUND files](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html), the truth-level input to GEMC[^gemc-simulation], the CLAS12 Geant4 simulation, for either unphysical uniform acceptance samples or physical samples baised on existing event generator output | LUND files, a completion manifest, and uniform-only monitoring |
| `slurm-submission` | Automatic validation of LUND output and submission of Slurm jobs for running the CLAS12 simulation on Jefferson Lab’s ifarm, in which the LUND files are passed through GEMC followed by by the CLAS12 reconstruction code, COATJAVA[^coatjava-reconstruction] | An ifarm Slurm array and a submission log |

The uniform LUND creator deliberately samples unphysical acceptance coverage. The physical LUND converter copies supported truth-level content from existing event-generator output; it does not run GENIE or invent missing kinematics. Creating LUND files never submits simulation.



It writes [LUND files](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html), the truth-level input to GEMC[^gemc-simulation], the CLAS12 Geant4 simulation, and can submit completed samples to Jefferson Lab’s ifarm, where GEMC simulates the detector response and COATJAVA[^coatjava-reconstruction] reconstructs the resulting events.

It supports deliberately unphysical samples for detector-acceptance studies and physical samples converted from existing event-generator output. Both paths share configuration, target geometry, file naming, provenance, and the handoff from event preparation to detector processing. The project does not run GENIE, perform physics analysis, or calculate final acceptance maps.

## Quick start

Building requires CMake 3.20 or later, a C++ compiler compatible with the selected ROOT installation, ROOT, and Python 3.9 or later. The sourced workflow launcher also requires csh or tcsh.

Replace `REPOSITORY_URL` with the HTTPS or SSH clone URL of the collaboration's fork.

```bash
git clone \
    REPOSITORY_URL
cd CLAS12-sample-generator
cmake \
    -S . \
    -B build/debug \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug
cmake \
    --build build/debug \
    --parallel 4
```

Create a 100-event uniform electron sample:

```bash
build/debug/apps/uniform-lund-creator \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/first-electron
```

The completed run is written to `runs/first-electron/Uniform__1e__5986MeV/`. If that resolved run directory already exists, creation warns, recursively removes that exact directory, and recreates it. Review the reported path before using a production output location.

The same operation through the supported checkout launcher is:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/first-electron
```

See the [quickstart](docs/getting-started/quickstart.md) for physical GENIE GST conversion and an ifarm submission preview.

## Output and provenance

Every successful LUND run has this common boundary:

```text
RUN/
├── lundfiles/
│   ├── PREFIX_1.txt
│   └── lund-creation-monitoring/
│       └── lund-creation-log.json
├── mchipo/
└── reconhipo/
```

The completion manifest records resolved settings, output counts, source provenance, and the exact LUND file inventory. Uniform creation also writes ROOT, PDF, and PNG monitoring products. Physical conversion creates no monitoring histograms.

LUND event headers use the shared ten-field format documented in the [LUND data contract](docs/concepts/lund-data-contract.md). Particle momentum, energy, mass, and vertex coordinates are written with five digits after the decimal point. Electron, proton, neutron, and charged-pion masses come from the imported target source; photons are massless.

## Submit simulation on ifarm

Submission consumes an already completed `lundfiles/` directory. It previews by default:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /absolute/path/to/completed-run/lundfiles \
    --num-jobs 2
```

The preview validates the manifest, detector inputs, software environment, and resulting `sbatch` command. It preserves existing simulation output. Add `--execute` only after reviewing the report; execution replaces the selected run's `mchipo/` and `reconhipo/` contents while preserving `lundfiles/`.

On ifarm, the login environment must provide COATJAVA 10.0.7 as described in the [ifarm environment guide](docs/submit-simulation/ifarm-environment.md). GEMC defaults to 5.14 because that release contains the RG-M Ar target and corrected one-foil C12 implementations used here.[^sportes-2026-rgm] GEMC 6.x with COATJAVA 11 still requires detector-level validation before replacing these defaults.

Submission responsibility ends when `sbatch` accepts the array. The project does not monitor later task failures or certify reconstructed output.

## Documentation

Start with the [documentation home](docs/index.md) or choose a task directly:

| Subject | Use it for |
| --- | --- |
| [Getting started](docs/getting-started/index.md) | Install, build, run a small sample, and understand outputs |
| [Create LUND files](docs/create-lund/index.md) | Uniform LUND creation, physical LUND conversion, configuration, examples, and monitoring |
| [Submit simulation](docs/submit-simulation/index.md) | Preview and submit ifarm GEMC/reconstruction jobs |
| [Concepts and contracts](docs/concepts/index.md) | Architecture, sampling, LUND records, provenance, and scientific scope |
| [Development](docs/development/index.md) | Contribute, validate changes, publish the wiki, or add an input adapter |

The [workflow examples](tutorials/README.md) contain longer production command lists. The [sample-profile inventory](config/samples/README.md) identifies reviewed and experimental configurations.

## Project boundaries

This repository prepares detector-simulation input and submits detector processing. It does not run a physical event generator, calculate acceptance maps, perform physics analysis, monitor completed Slurm jobs, or validate a production campaign's detector-level physics.

Two imported RG-M sources have narrow update boundaries: `src/workflows/lund-creation/external/targets.h` supplies target geometry and particle masses, while `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` contains the worker payload adapted for this project. Their provenance and replacement rules are documented under [external inputs](docs/concepts/external-inputs.md).

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before changing code or documentation. It lists the build and validation path, documentation expectations, protected external inputs, and the wiki publication flow.

[^clas12-spectrometer]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^gemc-simulation]: M. Ungaro et al., “The CLAS12 Geant4 simulation,” *Nucl. Instrum. Meth. A* **959**, 163422 (2020). [doi:10.1016/j.nima.2020.163422](https://doi.org/10.1016/j.nima.2020.163422)

[^coatjava-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
