# CLAS12 sample generation: technical-note outline

**Document status:** maintained software/methods reference, structured for later assembly into a technical note. The implementation and archived reference sources are the authority for the conventions described here. Local validation establishes generator and command compatibility; it does not establish detector-level acceptance equivalence.

## Abstract

The software prepares CLAS12 simulation inputs through uniform particle sampling or conversion of existing GENIE GST events. Both routes use common event records, target geometry, LUND serialization, and run manifests. A shared runner executes GEMC and reconstruction, with optional Slurm arrays. Legacy-compatible output is tested against archived implementations. Additional non-fixed nucleon prescriptions provide neutron isotropy inside the historical angular bounds and a balanced proton momentum/inverse-momentum mixture.

## Reading and assembly order

| Section | Material | Purpose |
| --- | --- | --- |
| 1. Scope and terminology | [Newcomer guide](index.md) | Physical samples, acceptance samples, GST, LUND, HIPO |
| 2. Provenance and legacy workflows | [Launch-chain reference](legacy-workflows.md) | Exact old entry points, selections, configuration mappings |
| 3. Software architecture | [Architecture](architecture.md), [source reference](code-reference.md) | Modules, data flow, APIs, ownership, build dependencies |
| 4. Generation methods | [Sampling models](sampling-models.md), [uniform guide](uniform-samples.md) | Equations, support, RNG streams, trigger electron |
| 5. Physical-event conversion | [GENIE guide](genie-to-lund-conversion.md) | GST schema, process/species selection, splitting |
| 6. Geometry and configuration | [Configuration reference](configuration.md) | Target positions, beam/metadata choices, validation |
| 7. Data products and provenance | [Data contracts](data-contracts.md), [diagnostics](diagnostics.md) | Field definitions, mass conventions, histograms, manifests |
| 8. Detector processing | [Simulation and Slurm](gemc-reconstruction-batch-submission.md) | Runtime environment, filenames, arguments, failure handling |
| 9. Verification and limitations | [Validation matrix](validation.md) | Byte/bin/command comparisons, distribution checks, known differences |
| Appendices | [Build guide](building.md), [migration](migration.md) | Reproducible build/run recipes and compatibility options |

## Boundaries and assumptions

- The repository converts GENIE output; it does not produce GENIE interactions or compute cross sections.
- Uniform electron–nucleon events are artificial acceptance probes. The trigger electron and nucleon need not satisfy exclusive scattering energy/momentum conservation.
- “Isotropic en” means uniform solid angle inside the legacy forward angular window, not full-sphere emission.
- GENIE header field 10 retains a process code, not a physical event weight.
- Compatibility requires matching beam energy, geometry, A/Z, seeds, selected mode and file settings. The archived launch scripts contain independent manual selections; they must not be assumed to describe one consistent campaign.
- Legacy LUND precision, historical pion constants and the retained short-input correction are explicit in the data/validation chapters.

## Material still needed for a publication

Record the production source revision, ROOT/compiler/GEMC/reconstruction versions, loaded modules, detector-card and reconstruction-YAML hashes, geometry databases, simulation RNG settings, campaign manifest and input dataset provenance. Add reconstructed acceptance plots and statistical comparisons from the intended server environment. The local tests in this repository do not supply those detector results.

The [diagnostics chapter](diagnostics.md) describes available plots and their quantities. For exported publication figures, choose axis ranges and labels appropriate to the campaign and retain the underlying ROOT histograms.
