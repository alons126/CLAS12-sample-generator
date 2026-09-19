# Newcomer guide

This project has two user-facing workflows: create LUND files, and submit completed LUND files to ifarm Slurm for GEMC plus reconstruction. LUND creation accepts uniform or physical input sources.

## The stages

```text
Uniform sampling -------------------+
                                    +--> LUND --> GEMC (Slurm) --> COATJAVA reconstruction (Slurm) --> reconstructed HIPO files for analyses
Existing GENIE GST --> conversion --+
```

**Uniform sampling** places particles across chosen momentum and angular ranges so an analysis can measure detector acceptance. The electron–hadron modes use an artificial trigger electron; they are not models of exclusive scattering kinematics.

**GENIE conversion** reads already generated physical events from a ROOT tree named `gst`, selects supported final-state particle species, and writes their momenta to LUND. It does not generate new GENIE interactions.

**LUND** is the text boundary between event preparation and detector simulation. Each event has one header followed by one line per particle. **GEMC** simulates the detector response. **Reconstruction** processes the simulated HIPO into reconstructed HIPO. Acceptance-map extraction belongs to downstream analysis.

## Recommended reading order

1. [Build and test](building.md): dependencies, commands and troubleshooting.
2. [Architecture](architecture.md): source layout and a run through the code.
3. Choose [uniform generation](uniform-samples.md) or [physical event-generator conversion](genie-to-lund-conversion.md).
4. [Configuration](configuration.md): units, defaults, seeds and target settings.
5. [Simulation and Slurm](gemc-reconstruction-batch-submission.md): configure the sourced setup and submit one array per sample.
6. [Migration](migration.md): old-to-new entry points and deliberate behavioral changes.

For the scientific and implementation reference, start at the [technical-note outline](technical-note.md). It links the sampling equations, data contracts, source inventory, legacy launch-chain mapping, and validation evidence.

## Where the maintained code lives

| Directory | What to look for there |
| --- | --- |
| `src/lund-generation/` | Complete LUND-creation workflow, applications, external geometry, and tests |
| `src/slurm-submission/` | Sourced setup/submission, external GEMC payload, and tests |
| `src/launcher/` | Shared dispatcher, ifarm checkout helpers, terminal presentation, and launcher test |
| `src/lund-generation/core/config/` | Run-option parsing, validation, and RG-M target metadata |
| `src/lund-generation/core/lund/` | Event records, particle masses, LUND writing, file splitting, and manifests |
| `src/lund-generation/core/geometry/` | The maintained adapter around external target geometry |
| `src/lund-generation/core/support/` | Shared constants, terminal colors, and compiled provenance template |
| `src/lund-generation/clas12-uniform/` | Uniform kinematic generation |
| `src/lund-generation/clas12-generator-to-lund/` | Physical-source dispatch, with generator adapters such as `genie/` nested below it |
| `src/lund-generation/external/` | Protected imported target geometry used during LUND creation |
| `src/slurm-submission/external/` | Protected GEMC/reconstruction worker payload |

The shared configuration, LUND, geometry, and support directories build together as `LundCore`. Uniform sampling and monitoring build as `UniformGeneration`. The source-specific directories match their executable names, and physical generator adapters are subordinate to `clas12-generator-to-lund`. The detailed call chain is in the [architecture walkthrough](architecture.md).

## A run directory

```text
runs/example/
    lundfiles/
        PREFIX_1.txt
        PREFIX_2.txt
        lund-gen-monitoring/
            lund-gen-log.json       # Published only after successful generation/conversion
            PREFIX_monitoring_plots.root # All uniform histograms; absent for physical runs
            MonitoringPlotsPath/    # Required PDF/numbered PNG layout for every uniform run
    mchipo/             # Prepared by uniform creation; filled by simulation
    reconhipo/          # Prepared by uniform creation; filled by reconstruction
        simulation/     # Command records, locks and detector-config hashes
```

A failed generation may leave partial files but no completed `lund-gen-log.json`. On rerun, the resolved run directory is validated, reported, recursively removed, and recreated, preserving the legacy generator lifecycle.

Uniform LUND files default to 25,000 events per file; physical conversion defaults to 10,000. Submission reads each file's exact event count from the completed manifest and passes it to both GEMC and reconstruction.

All documented shell examples start at the repository root. Executables and scripts also work from other directories when supplied appropriate paths; relative sample configuration paths are interpreted from the caller's working directory.

For local editing and server execution via `source run.csh`, read the [SSH workflow](ssh-workflow.md). Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](external-inputs.md).

Every LUND-creation command explicitly selects `--workflow create-lund`, `--source uniform|physical`, and either a reviewed [sample profile](../config/samples/README.md) or the complete set of sample options. `config/run.json` supplies build/test defaults only.

See [source documentation conventions](source-documentation.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded and protected from edits.

The [unified external GEMC payload](gemc-payload.md) documents `src/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with sourced-shell setup.
