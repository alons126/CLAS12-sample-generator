# CLAS12 sample generator

This code prepares CLAS12[^clas12-spectrometer] simulation samples for the [e4ν collaboration](https://e4nu.org). First, it writes particles' momenta and vertex coordinates (the event's position in the target) to [LUND text files](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html). These are truth-level particles: the particles before detector simulation. In a separate step, it submits jobs that run GEMC[^gemc-simulation] detector simulation and COATJAVA[^coatjava-reconstruction] reconstruction. GEMC simulates the detector response; reconstruction uses that response to determine the measured particles.

| Workflow | What it does | Main result |
| --- | --- | --- |
| `create-lund` | Writes LUND input from randomly sampled particles or existing event-generator output | LUND files and a JSON log listing their settings and event counts; this log marks successful creation |
| `submit` | Submits simulation and reconstruction jobs to Jefferson Lab’s computing farm (ifarm), using the Slurm job scheduler | A job ID; a JSON log; the submitted jobs later write simulated and reconstructed HIPO files |

The **uniform LUND creator** randomly chooses particle momenta and angles within your configured ranges. Its samples are deliberately unphysical and are used to study which particles the detector can detect and reconstruct. The **physical LUND converter** copies supported particles from existing event-generator output; it currently reads [GENIE](https://github.com/GENIE-MC/Generator) production output in the GST format stored in [ROOT](https://github.com/root-project/root) files. Both applications use the same code to sample one vertex position in the target per event and assign it to every particle, write and split LUND files, and record the settings used.

The submitted jobs produce reconstructed [HIPO](https://github.com/gavalian/hipo) files, which can be analyzed with [CLAS12ROOT](https://github.com/JeffersonLab/clas12root/tree/master). This code does not run a physical event generator, calculate detector acceptance, select events from reconstructed files, or perform physics analysis. Creating LUND files never submits jobs automatically.

## Start here

The [code Wiki](../../wiki) is the user and developer manual. Its [home page](../../wiki/Home) gives separate reading paths for running the software and extending it.

For a small uniform run on ifarm, use the profile [`uniform-1e-5986MeV.conf`](config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf). A profile is a text file containing the sample settings:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/quickstart-output
```

Use [`run.csh`](run.csh) in an ifarm clone of the repository used for running code, not editing it. Development is advised to be done locally and synced on the ifarm via Git. Before running the selected workflow, the script discards uncommitted edits to files tracked by Git and deletes files Git does not track, including ignored files. It preserves the `build/` directory, then updates the code from Git and builds when needed. It checks the full sample settings only afterward, so an invalid command can still clean the checkout before failing.

Commit and push development changes from your local clone first. Store generated samples outside the ifarm repository directory so the next cleanup cannot delete them. For local development, build with CMake and run the compiled applications directly; those commands do not clean or update the checkout.

After LUND creation succeeds, preview simulation submission with the run's `lundfiles/` directory:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/quickstart-output/Uniform__1e__5986MeV/lundfiles
```

Without `--execute`, the command checks and prints the settings but does not submit jobs. Review the detector settings and the directories it would delete before adding `--execute`.

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
