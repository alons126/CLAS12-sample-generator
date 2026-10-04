# Concepts and contracts

These pages explain the design that the user guides rely on:

- [Architecture](architecture.md): workflow boundaries, control flow, and extension rules.
- [LUND data contract](lund-data-contract.md): in-memory records, serialized fields, splitting, and manifests.
- [Sampling models](sampling-models.md): exact uniform distributions, correlations, and random streams.
- [External inputs](external-inputs.md): protected geometry, detector resources, and worker payload.

LUND is the central interface. Creation owns truth preparation; submission owns the handoff to GEMC and COATJAVA. Downstream analysis is outside both workflows.
