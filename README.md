# CLAS12 sample generator

This project prepares CLAS12[^clas12-spectrometer] simulation samples for the [e4ν collaboration](https://e4nu.org). It connects truth-level particle preparation to the standard CLAS12 detector chain without hiding the scientific or operational choices made along the way.

The project has two separate workflows:

| Workflow | What it does | Main result |
| --- | --- | --- |
| `create-lund` | Creates [LUND](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html) input from random acceptance-test kinematics or converts existing event-generator truth | LUND files and a completion manifest |
| `submit` | Submits those LUND files to ifarm Slurm | GEMC[^gemc-simulation] detector simulation followed by COATJAVA[^coatjava-reconstruction] reconstruction |

The **uniform LUND creator** makes deliberately unphysical samples that cover configured momentum and angle ranges. The **physical LUND converter** preserves supported particles from existing event-generator output; it currently reads [GENIE](https://github.com/GENIE-MC/Generator) GST ROOT trees and does not run GENIE. Both paths use the same target geometry, LUND writer, file splitting, provenance, and completion rules where their meanings agree.

The workflows stop at reconstructed [HIPO](https://github.com/gavalian/hipo) output, which can be analyzed with [CLAS12ROOT](https://github.com/JeffersonLab/clas12root/tree/master). They do not generate physical interactions, calculate detector acceptance, skim reconstructed data, or perform physics analysis.

## Start here

The [project Wiki](../../wiki) is the user and developer manual. Its [documentation home](../../wiki/Home) gives separate reading paths for running the software and extending it.

For a small uniform run on ifarm:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/output
```

`run.csh` is designed for a disposable ifarm checkout: after validating the command, it discards local checkout changes, updates from Git, and builds when needed. Commit and push valuable development work before running it. Local developers should build with CMake and invoke the compiled executables directly.

After LUND creation succeeds, preview simulation submission with the run's `lundfiles/` directory:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/run/lundfiles
```

Preview is the default. Add `--execute` only after reviewing the resolved detector settings and output actions.

Useful entry points:

- [Quickstart](../../wiki/getting-started-quickstart)
- [Create LUND files](../../wiki/create-lund-overview)
- [Submit simulation](../../wiki/submit-simulation-overview)
- [Architecture](../../wiki/concepts-architecture)
- [Contributing](CONTRIBUTING.md)

The longer command collections under [`tutorials/`](tutorials/) demonstrate every public option. Historical implementations are preserved in the `legacy-code-archive` tag and are not part of the current manual.

[^clas12-spectrometer]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^gemc-simulation]: M. Ungaro et al., “The CLAS12 Geant4 simulation,” *Nucl. Instrum. Meth. A* **959**, 163422 (2020). [doi:10.1016/j.nima.2020.163422](https://doi.org/10.1016/j.nima.2020.163422)

[^coatjava-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)
