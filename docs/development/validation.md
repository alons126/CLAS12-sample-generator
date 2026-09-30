# Scientific validation boundaries

Software behavior and detector-level scientific validation are separate responsibilities. The implementation defines record formats, supported particle content, sampling prescriptions, file splitting, configuration validation, provenance, and submission handoff. A production campaign must also establish that its detector and reconstruction settings are suitable for the intended analysis.

## Defined software behavior

- **Physical-input cutoff:** before starting a follow-up file, conversion requires at least `events-per-file` inclusive input entries beginning with the current accepted entry. The first file is allowed through input exhaustion, and an exact final block is never interrupted after it starts. This is an input-entry cutoff, not an accepted-event calculation.
- **Random state:** ROOT treats `TRandom3(0)` as automatic seeding, so a run cannot be reconstructed from the configured zero alone. Use explicit nonzero seeds when repeatability is required.
- **Mass source:** electron, proton, neutron, and charged-pion masses come from external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); photons use exact zero.
- **Neutral pions:** the physical LUND converter requires neutral pions to be decayed during upstream GENIE production and consumes the resulting photons. Residual PDG 111 entries are skipped because the physical LUND converter does not invent missing decay kinematics.
- **Production sampling:** the 1e and charged-hadron mixtures use the configured uniform-p/uniform-1/p prescription. Neutrons use uniform momentum, including the configured zero-to-beam range.
- **Diagnostics:** monitoring is uniform-only and is stored once in `<prefix>__monitoring_plots.root`; its combined PDF is `<prefix>__plots.pdf`. Physical conversion creates no monitoring histograms.
- **Metadata:** checked-in Ar profiles use A=40 and Z=18. Geometry, A, and Z remain independently configurable for unusual studies.
- **Reproduction boundary:** match the beam energy, target variation and geometry, A/Z metadata, both configured seeds, source/channel settings, physical input, software versions, and file settings. These inputs can be selected independently, so individually valid values do not necessarily describe one consistent campaign.

## Production validation

Use matched LUND samples, identical GEMC and reconstruction versions, identical GCARD/YAML/database resources, and explicit detector RNG control when the production environment supports it. Compare event counts, generated banks, reconstructed particle yields, and acceptance distributions. Retain the source revision, input-dataset provenance, campaign manifest, ROOT/compiler/GEMC/reconstruction versions, loaded modules, geometry databases, detector-card and reconstruction-YAML hashes, simulation RNG settings, logs, tolerances, and statistical uncertainties with the campaign record.

Uniform FD pion modes and every uniform CD mode remain marked as unvalidated for production. Their configuration files and documentation retain that warning until detector-level evidence supports changing it.

Server software, geometry databases, reconstruction versions, field settings, and simulation RNG state are required before claiming equivalent HIPO output or acceptance. A successful local build or a readable output file does not establish detector-level scientific validity.

Publication-quality acceptance results require reconstructed acceptance plots and statistical comparisons from the intended server environment. Local software checks and uniform-generation monitoring cannot supply that detector-level evidence. Retain the underlying ROOT histograms used to produce exported figures.
