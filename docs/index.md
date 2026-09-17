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
5. [Simulation and Slurm](gemc-reconstruction-batch-submission.md): preview commands before execution.
6. [Migration](migration.md): old-to-new entry points and deliberate behavioral changes.

For the scientific and implementation reference, start at the [technical-note outline](technical-note.md). It links the sampling equations, data contracts, source inventory, legacy launch-chain mapping, and validation evidence.

## Where the maintained code lives

| Directory | What to look for there |
| --- | --- |
| `src/config/` | Run-option parsing, validation, and RG-M target metadata |
| `src/lund/` | Event records, particle masses, LUND writing, file splitting, and manifests |
| `src/geometry/` | The maintained adapter around external target geometry |
| `src/monitoring/` | ROOT histograms and legacy-compatible monitoring artifacts |
| `src/support/` | Shared constants, terminal colors, and compiled provenance template |
| `src/clas12-uniform/` | Uniform kinematic generation |
| `src/clas12-generator-to-lund/` | Physical-source dispatch, with generator adapters such as `genie/` nested below it |
| `src/common/external/` | Protected imported files; do not treat this as maintained common code |

The first five directories build together as `LundCore`. They remain separate folders so configuration, data format, geometry, diagnostics, and low-level definitions are easy to find without adding unnecessary runtime abstractions. The two source-specific directories match their executable names, and physical generator adapters are subordinate to `clas12-generator-to-lund`. The detailed call chain is in the [architecture walkthrough](architecture.md).

## A run directory

```text
runs/example/
    manifest.json       # Published only after successful generation/conversion
    lundfiles/
        PREFIX_1.txt
        PREFIX_2.txt
    monitoring.root     # Per-particle diagnostic histograms
    legacy_histograms.root # Stable copy of original named diagnostics
    PREFIX_plots.root   # Archived uniform histogram filename (uniform only)
    MonitoringPlotsPath/ # Archived PDF/numbered PNG layout (uniform default)
    mchipo/             # Prepared by uniform creation; filled by simulation
    reconhipo/          # Prepared by uniform creation; filled by reconstruction
    rootfiles/          # Prepared legacy uniform downstream directory
    simulation/         # Command records and detector-config hashes
```

A failed generation may leave partial files but no completed manifest. On rerun, the resolved run directory is validated, reported, recursively removed, and recreated, preserving the legacy generator lifecycle.

Uniform LUND files default to 25,000 events per file; physical conversion defaults to 10,000. Submission reads each file's exact event count from the completed manifest and passes it to both GEMC and reconstruction.

All documented shell examples start at the repository root. Executables and scripts also work from other directories when supplied appropriate paths; relative sample configuration paths are interpreted from the caller's working directory.

For local editing and server execution via `source run.csh`, read the [SSH workflow](ssh-workflow.md). Target-header replacement, LUND format and gcard/field provenance are covered in [external inputs](external-inputs.md).

Every LUND-creation command explicitly selects `--workflow create-lund`, `--source uniform|physical`, and either a reviewed [sample profile](../config/samples/README.md) or the complete set of sample options. `config/run.json` supplies build/test defaults only.

See [source documentation conventions](source-documentation.md) for the banners, region markers and explanations embedded in maintained code. External and archived source files are excluded and protected from edits.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
