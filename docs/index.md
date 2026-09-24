# CLAS12 sample generator documentation

This project has exactly two user-facing workflows. First create completed LUND files from uniform acceptance sampling or existing physical event-generator truth. Later, and only as a separate action, submit those LUND files to ifarm Slurm for GEMC detector simulation followed by CLAS12 reconstruction.

```mermaid
flowchart TB
    subgraph CREATE["1. Create LUND files"]
        direction LR
        U["Uniform acceptance<br/>sampling"] --> C["Create LUND files"]
        P["Existing physical<br/>generator truth"] --> C
        C --> L["Split LUND files and<br/>completion manifest"]
    end

    subgraph SIMULATE["2. Submit and simulate"]
        direction RL
        S["Submit ifarm<br/>Slurm array"] --> G["CLAS12 simulation<br/>(GEMC)"]
        G --> R["CLAS12 reconstruction<br/>(COATJAVA)"]
        R --> H["Reconstructed HIPO"]
    end

    CREATE --> SIMULATE
```

The project does not run a physical event generator, derive acceptance maps, or perform physics analysis.

## Choose where to start

| Goal | Start here |
| --- | --- |
| Build the project and make a small sample | [Getting started](getting-started/index.md) |
| Create uniform or physical LUND files | [Create LUND files](create-lund/index.md) |
| Submit completed LUND files to ifarm | [Submit simulation](submit-simulation/index.md) |
| Understand sampling, records, architecture, or provenance | [Concepts and data contracts](concepts/index.md) |
| Modify, test, or extend the software | [Development guide](development/index.md) |
| Understand compatibility and archived behavior | [History and migration](history/index.md) |

## Common reader paths

- **New user:** [install and test](getting-started/installation.md) → [quickstart](getting-started/quickstart.md) → [output layout](getting-started/outputs.md).
- **Uniform-sample user:** [creation overview](create-lund/index.md) → [uniform sampling](create-lund/uniform.md) → [examples](create-lund/examples.md).
- **Physical-sample user:** [creation overview](create-lund/index.md) → [physical conversion](create-lund/physical.md) → [examples](create-lund/examples.md).
- **ifarm operator:** [submission overview](submit-simulation/index.md) → [examples](submit-simulation/examples.md) → [full operational guide](submit-simulation/guide.md).
- **Adapter developer:** [architecture](concepts/architecture.md) → [adding an event generator](development/adding-event-generator.md) → [validation](development/validation.md).

## Safety and provenance

LUND creation reports the fully resolved run directory, then recursively replaces that exact directory when it already exists. Submission previews by default; `--execute` replaces simulation output directories while preserving LUND input. The ifarm checkout is intentionally disposable and is refreshed from Git before a real workflow. Read the relevant workflow page before using production paths.

Every successful LUND run publishes `lundfiles/lund-gen-monitoring/lund-gen-log.json`. That manifest is the handoff from creation to submission and records resolved settings, software provenance, scanned/written counts, and the exact LUND file inventory.
