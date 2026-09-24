# Concepts and data contracts

This section explains why the workflows behave as documented. It is reference material, not a prerequisite for the quickstart.

- [Architecture](architecture.md): components, targets, and call chains.
- [Sampling models](sampling-models.md): uniform distributions, correlations, and random streams.
- [LUND data contract](lund-data-contract.md): header/particle fields, precision, splitting, and manifests.
- [External inputs](external-inputs.md): protected target geometry and detector resources.
- [Scientific scope](scientific-scope.md): assumptions, boundaries, and technical-note assembly.

The central boundary is LUND: creation produces truth-level particle records; submission sends those records through GEMC/Geant4 detector transport and CLAS12 reconstruction. Acceptance extraction and physics analysis remain downstream.
