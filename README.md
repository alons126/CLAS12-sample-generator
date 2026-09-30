# CLAS12 sample generator

This project prepares simulation samples for the [e4ν collaboration](https://e4nu.org)’s electron-scattering studies with the [CLAS12 spectrometer](https://doi.org/10.1016/j.nima.2020.163419)[^clas12-spectrometer] at Jefferson Lab. It provides two separate user-facing workflows:

| Workflow | Purpose | Result |
| --- | --- | --- |
| `create-lund` | Create [LUND files](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html), the truth-level input to GEMC[^gemc-simulation], from uniform acceptance kinematics or existing physical event-generator output | LUND files, a completion manifest, and uniform-only monitoring |
| `submit` | Validate LUND output and other input requirements, then submit Slurm job arrays that run GEMC detector simulation followed by CLAS12 reconstruction with COATJAVA[^coatjava-reconstruction] on Jefferson Lab's ifarm | An ifarm Slurm array and a submission log |

Uniform samples provide deliberately unphysical detector-acceptance coverage, while physical samples preserve the particle content of existing event-generator output. LUND creation and detector simulation remain separate steps.

## Quick start

Building requires CMake 3.20 or later, a C++ compiler compatible with the selected ROOT installation, ROOT, and Python 3.9 or later. The workflow launcher also requires csh or tcsh. Replace `REPOSITORY_URL` with the HTTPS or SSH clone URL of the collaboration's fork.

```bash
git clone REPOSITORY_URL
cd CLAS12-sample-generator
```

### LUND file creation

From the ifarm ssh (or a csh or tcsh shell), create a 100-event uniform electron sample. The launcher configures and builds the applications before running the selected workflow:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/first-electron
```

The completed run is written below `runs/first-electron/`. Before writing, the uniform LUND creator reports the fully resolved run directory. If that directory already exists, it warns, removes it, and recreates it. Review the reported path before using a production output location. See the [installation guide](docs/getting-started/installation.md) for manual CMake commands and direct executable use. Continue with the [full quickstart](docs/getting-started/quickstart.md) for physical conversion and an ifarm submission preview.

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

The completion manifest records the resolved configuration, event counts, source provenance, and exact LUND file inventory. Uniform creation also writes ROOT, PDF, and PNG monitoring histograms, while physical conversion does not. The [output guide](docs/getting-started/outputs.md) explains the complete directory layout, and the [LUND data contract](docs/concepts/lund-data-contract.md) defines the serialized records and manifest fields.

### Slurm job submission on the ifarm

Submission consumes an already completed `lundfiles/` directory. The workflow previews the job array's requirements without submitting them with `sbatch`, by default:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /absolute/path/to/completed-run/lundfiles \
    --num-jobs 2
```

The preview validates the completed run, detector inputs, software environment, and resulting `sbatch` command without submitting jobs. It preserves existing simulation output. Add `--execute` only after reviewing the report; execution replaces the selected run's `mchipo/` and `reconhipo/` contents while preserving `lundfiles/`.

Submission responsibility ends when `sbatch` accepts the array. The project does not monitor later task failures or certify reconstructed output. Read the [ifarm environment guide](docs/submit-simulation/ifarm-environment.md) before using this workflow, then use the [submission guide](docs/submit-simulation/guide.md) for software-version defaults, input rules, output replacement, and post-submission checks.

## Documentation

Start with the [documentation home](docs/index.md) or choose a task directly:

| Subject | Use it for |
| --- | --- |
| [Getting started](docs/getting-started/index.md) | Install, build, run a small sample, and understand outputs |
| [Create LUND files](docs/create-lund/index.md) | Uniform LUND creation, physical LUND conversion, configuration, examples, and monitoring |
| [Submit simulation](docs/submit-simulation/index.md) | Preview and submit ifarm GEMC/reconstruction jobs |
| [Concepts and contracts](docs/concepts/index.md) | Architecture, sampling, LUND records, provenance, and scientific scope |
| [Development](docs/development/index.md) | Contribute, validate changes, publish the wiki, or add an input adapter |

For a first pass, read **Getting started**, then the guide for the workflow you intend to run. The [workflow examples](tutorials/README.md) contain longer command lists and complete option demonstrations. The [sample-profile inventory](config/samples/README.md) identifies reviewed and experimental configurations.

## Project boundaries

This repository prepares detector-simulation input and submits detector processing. It does not run a physical event generator, calculate acceptance maps, perform physics analysis, monitor completed Slurm jobs, or by itself validate the detector-level physics of a production campaign. The [scientific scope](docs/concepts/scientific-scope.md) defines these boundaries in detail, and [external inputs](docs/concepts/external-inputs.md) records the project’s imported geometry, detector, and worker sources.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before changing code or documentation. It lists the build and validation path, documentation expectations, protected external inputs, and the wiki publication flow.

[^clas12-spectrometer]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^gemc-simulation]: M. Ungaro et al., “The CLAS12 Geant4 simulation,” *Nucl. Instrum. Meth. A* **959**, 163422 (2020). [doi:10.1016/j.nima.2020.163422](https://doi.org/10.1016/j.nima.2020.163422)

[^coatjava-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)
