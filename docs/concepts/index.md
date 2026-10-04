# Concepts and contracts

These pages explain the design that the user guides rely on:

- [Architecture](architecture.md): workflow boundaries, control flow, and extension rules.
- [LUND data contract](lund-data-contract.md): in-memory records, serialized fields, splitting, and manifests.
- [Sampling models](sampling-models.md): exact uniform distributions, correlations, and random streams.
- [External inputs](external-inputs.md): imported target geometry, detector files, and the Slurm worker script.

LUND files connect the two workflows: creation writes them, and the submitted GEMC jobs read them before COATJAVA reconstructs the detector output. Analysis of reconstructed events is outside both workflows.
