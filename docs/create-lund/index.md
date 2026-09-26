# Create LUND files

The `create-lund` workflow has one common output contract and two event sources.

```mermaid
flowchart TD
    BUILD["run.csh --workflow create-lund<br/>workflow.py builds the application<br/>RunConfig validates profile and CLI"]
    U["--source uniform<br/>uniform-lund-creator<br/>Sample configured acceptance kinematics"]
    P["--source physical<br/>event-generator-to-lund-converter<br/>Read and select existing GENIE GST truth"]
    SHARED["Shared target geometry, Event, Particle, and LundWriter<br/>Assign one vertex per event, serialize, and split"]
    DONE["Completed LUND files and manifest"]
    MONITORING["Uniform only<br/>ROOT, PDF, and PNG monitoring plots"]
    LOCAL["Creation can run locally<br/>It does not submit simulation jobs"]

    BUILD --> U
    BUILD --> P
    U --> SHARED
    P --> SHARED
    SHARED --> DONE
    U -.-> MONITORING
    SHARED -.-> LOCAL

    classDef endpoint fill:#183247,color:#ffffff,stroke:#183247,stroke-width:2px;
    classDef stage fill:#e8f1ef,color:#183247,stroke:#0f8492,stroke-width:2px;
    classDef note fill:#ffffff,color:#536879,stroke:#a6b4bd,stroke-dasharray:5 5;
    class BUILD,DONE endpoint;
    class U,P,SHARED stage;
    class MONITORING,LOCAL note;
```

Code shown in the diagram: [`run.csh`](../../run.csh), [`workflow.py`](../../src/launcher/workflow.py), [`RunConfig.h`](../../src/lund-creation/core/config/RunConfig.h), and [`LundWriter.h`](../../src/lund-creation/core/lund/LundWriter.h).

## Choose a source

| Source | Use it for | Guide |
| --- | --- | --- |
| `uniform` | Deliberately unphysical acceptance samples | [Uniform samples](uniform.md) |
| `physical` | Conversion of existing event-generator truth | [Physical conversion](physical.md) |

Both sources share validated configuration, target sampling, particle records, LUND serialization, output naming, file splitting, provenance, and completion behavior where their semantics agree. They do not share event-content rules: uniform mode creates particles, while a physical adapter copies supported truth and must not invent missing kinematics.

## Pages in this section

- [Configuration reference](configuration.md): all shared and source-specific settings.
- [Command examples](examples.md): profiles, overrides, build controls, and physical provenance.
- [Uniform monitoring](monitoring.md): ROOT objects and rendered products.
- [Sample profile inventory](../../config/samples/README.md): reviewed beam/channel configurations.
- [LUND data contract](../concepts/lund-data-contract.md): serialized fields, splitting, and manifest schema.
- [Targets, seeds, and sampling](../concepts/sampling-models.md): scientific definitions and reproducibility.

Creating LUND files never submits simulation. Continue with [Submit simulation](../submit-simulation/index.md) only after a run has a completed manifest.
