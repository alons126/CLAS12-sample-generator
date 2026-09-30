# CLAS12 sample generator

This project prepares simulation samples for the [e4ν collaboration](https://e4nu.org)’s electron-scattering studies with the CLAS12 spectrometer[^clas12-spectrometer] at Jefferson Lab. It provides two separate user-facing workflows:

| Workflow | Purpose | Result |
| --- | --- | --- |
| `create-lund` | Create [LUND files](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html), the truth-level input to GEMC[^gemc-simulation], from uniform acceptance kinematics or existing physical event-generator output | LUND files, a completion manifest, and uniform-only monitoring |
| `submit` | Validate LUND output and other input requirements, then submit Slurm job arrays that run GEMC detector simulation followed by CLAS12 reconstruction with COATJAVA[^coatjava-reconstruction] on Jefferson Lab's ifarm | An ifarm Slurm array and a submission log |

Uniform samples provide deliberately unphysical detector-acceptance coverage, while physical samples preserve the particle content of existing event-generator output. The physical LUND converter currently supports [GENIE](https://github.com/GENIE-MC/Generator) GST input; its adapter boundary allows other event-generator formats to be added. The project does not run a physical event generator, calculate acceptance maps, or perform physics analysis. LUND creation and detector simulation remain separate steps; the [scientific scope](../../wiki/concepts-scientific-scope) defines these boundaries in detail.

**Full documentation:** [Read the project Wiki](../../wiki).

**For legacy code implementation:** See the `legacy-code-archive` tag.

## Quick start

Building requires CMake 3.20 or later, a C++ compiler compatible with the selected ROOT installation, ROOT, and Python 3.9 or later. The workflow launcher also requires csh or tcsh. Replace `REPOSITORY_URL` with the HTTPS or SSH clone URL of the collaboration's fork.

```tcsh
git clone REPOSITORY_URL
cd CLAS12-sample-generator
```

### LUND file creation

From a csh or tcsh login shell on ifarm, create a 100-event uniform electron sample. `run.csh` treats its checkout as disposable: before running the workflow, it removes untracked files, discards tracked changes, pulls the configured remote branch, and updates submodules while preserving the `build/` directory. Commit and push valuable changes from a development checkout before using it. For local LUND creation, use the direct-executable instructions in the Wiki instead.

The launcher configures and builds the applications before running the selected workflow:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output runs/first-electron
```

The completed run is written to `runs/first-electron/Uniform__1e__5986MeV/`. Before writing, the uniform LUND creator reports this fully resolved run directory. If it already exists, the creator warns, removes it, and recreates it. Review the reported path before using a production output location. See the Wiki's [installation guide](../../wiki/getting-started-installation) for manual CMake commands and direct executable use. Continue with the [full quickstart](../../wiki/getting-started-quickstart) for physical conversion and an ifarm submission preview.

Every successful LUND run has this common boundary:

```text
RUN/
├── lundfiles/
│   ├── PREFIX_1.txt
│   ├── ...                         # additional split files when needed
│   └── lund-creation-monitoring/
│       ├── lund-creation-log.json
│       ├── PREFIX__monitoring_plots.root  # uniform only
│       └── MonitoringPlotsPath/           # uniform only
│           ├── PREFIX__plots.pdf
│           └── INDEX_HISTOGRAM.png
├── mchipo/
└── reconhipo/
```

The completion manifest records the resolved configuration, event counts, source provenance, and exact LUND file inventory. Uniform creation also writes ROOT, PDF, and PNG monitoring histograms, while physical conversion does not. The Wiki's [output guide](../../wiki/getting-started-outputs) explains the complete directory layout, the [LUND data contract](../../wiki/concepts-lund-data-contract) defines the serialized records and manifest fields, and [external inputs](../../wiki/concepts-external-inputs) records the imported geometry, detector, and worker sources.

### Slurm job submission on the ifarm

Submission consumes a successful run's `lundfiles/` directory. Its completion manifest normally supplies the exact file inventory and event counts. The workflow previews the job array's requirements without submitting them with `sbatch`, by default:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /absolute/path/to/lund-run/lundfiles \
    --num-jobs 2
```

The preview validates the LUND run, detector inputs, software environment, and resulting `sbatch` command without submitting jobs. It preserves existing simulation output. Add `--execute` only after reviewing the report; execution replaces the selected run's `mchipo/` and `reconhipo/` contents while preserving `lundfiles/`.

Submission responsibility ends when `sbatch` accepts the array. The project does not monitor later task failures or certify reconstructed output. Read the Wiki's [ifarm environment guide](../../wiki/submit-simulation-ifarm-environment) before using this workflow, then use the [submission guide](../../wiki/submit-simulation-guide) for software-version defaults, input rules, output replacement, and post-submission checks.

## Documentation

The [project Wiki](../../wiki) is the complete user and developer manual. Start at its documentation home for the recommended reading order, or choose a subject directly:

| Subject | Use it for |
| --- | --- |
| [Getting started](../../wiki/getting-started-overview) | Install, build, run a small sample, and understand outputs |
| [Create LUND files](../../wiki/create-lund-overview) | Uniform LUND creation, physical LUND conversion, configuration, examples, and monitoring |
| [Submit simulation](../../wiki/submit-simulation-overview) | Preview and submit ifarm GEMC/reconstruction jobs |
| [Concepts and contracts](../../wiki/concepts-overview) | Architecture, sampling, LUND records, provenance, and scientific scope |
| [Development](../../wiki/development-overview) | Contribute, validate changes, publish the Wiki, or add an input adapter |

For a first pass, read **Getting started**, then the guide for the workflow you intend to run. The Wiki's [workflow examples](../../wiki/Workflow-Examples) contain longer command lists and complete option demonstrations. The [sample-profile inventory](../../wiki/Sample-Profiles) identifies reviewed and experimental configurations.

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before changing code or documentation. It lists the build and validation path, documentation expectations, protected external inputs, and the wiki publication flow.

[^clas12-spectrometer]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^gemc-simulation]: M. Ungaro et al., “The CLAS12 Geant4 simulation,” *Nucl. Instrum. Meth. A* **959**, 163422 (2020). [doi:10.1016/j.nima.2020.163422](https://doi.org/10.1016/j.nima.2020.163422)

[^coatjava-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)
