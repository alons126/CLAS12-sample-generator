# Scientific validation boundaries

Software behavior and detector-level scientific validation are separate responsibilities. The maintained implementation defines record formats, supported particle content, sampling prescriptions, file splitting, configuration validation, provenance, and submission handoff. A production campaign must additionally establish that its detector and reconstruction settings are suitable for the intended analysis.

## Intentional maintained behavior

- **Physical-input cutoff:** before starting a follow-up file, conversion requires at least `events-per-file` inclusive input entries beginning with the current accepted entry. The first file is allowed through input exhaustion, and an exact final block is never interrupted after it starts. This is an input-entry cutoff, not an accepted-event calculation.
- **Random state:** archived `TRandom3(0)` runs cannot be reconstructed from a seed that was never recorded. Use explicit nonzero seeds when repeatability is required.
- **Mass source:** electron, proton, neutron, and charged-pion masses come from external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); photons use exact zero.
- **Neutral pions:** the maintained converter requires neutral pions to be decayed during upstream GENIE production and consumes the resulting photons. Residual PDG 111 entries are skipped because the converter does not invent missing decay kinematics.
- **Production sampling:** the 1e and charged-hadron mixtures use the configured uniform-p/uniform-1/p prescription. Neutrons use uniform momentum, including the configured zero-to-beam range.
- **Diagnostics:** monitoring is uniform-only and is stored once in `<prefix>_monitoring_plots.root`. Physical conversion creates no monitoring histograms.
- **Metadata:** checked-in Ar profiles use A=40 and Z=18. Geometry, A, and Z remain independently configurable for unusual studies.

## Production validation

Use matched LUND samples, identical GEMC and reconstruction versions, identical GCARD/YAML/database resources, and explicit detector RNG control when the production environment supports it. Compare event counts, generated banks, reconstructed particle yields, and acceptance distributions. Retain logs, manifests, software versions, resource hashes, tolerances, and statistical uncertainties with the campaign record.

Uniform FD pion modes and every uniform CD mode remain marked as unvalidated for production. Their configuration files and documentation retain that warning until detector-level evidence supports changing it.

Server software, geometry databases, reconstruction versions, field settings, and simulation RNG state are required before claiming equivalent HIPO output or acceptance. A successful local build or a readable output file does not establish detector-level scientific validity.
