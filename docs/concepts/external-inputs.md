# External geometry and detector inputs

## Target source and replacement

The project aims to remain modestly modular at two RG-M-derived boundaries: target geometry enters through [`src/workflows/lund-creation/external/targets.h`](../../src/workflows/lund-creation/external/targets.h), and per-array-task detector execution enters through [`src/workflows/slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh). Maintained adapters and coordinators surround these files instead of duplicating their geometry or detector commands. If RG-M publishes an update, the corresponding external file can therefore be reviewed and replaced or readapted without redesigning the currently implemented workflows or future peer workflows.

The checked-in `targets.h` is an exact copy of the RG-M target source containing the latest RG-M target implementations available with GEMC 5.14 when this snapshot was adopted. Its external origin is [awild7/rgm](https://github.com/awild7/rgm/tree/main), and the target implementations are documented in CLAS12 Note 2026-001[^sportes-2026-rgm]. The file is consumed byte-for-byte through `TargetGeometry`; project-specific validation and RNG isolation remain outside it.

The unified GEMC submission payload was also obtained from RG-M code, but unlike `targets.h`, it has been modified for this project. It preserves the RG-M script's overall structure and usage pattern while accepting the unified generator-independent settings documented in the [worker reference](../submit-simulation/worker-reference.md). Future RG-M payload revisions should be compared with this adaptation so upstream detector-command changes can be incorporated without discarding the maintained interface.

The two RG-M files and detector cards/YAML under `config/detector/` are external inputs kept in the repository so workflows remain reproducible. The maintained code consumes their interfaces through narrow boundaries, allowing reviewed upstream replacements to be adopted with an explicit update and validation pass.

Both uniform generation and physical conversion call this header's `randomVertex()` through `TargetGeometry`. The maintained RG-M catalog selects valid map keys and supplies nucleus/GEMC metadata without changing the external header. The adapter transfers the caller's full vertex RNG state into and out of its `ran` generator under a mutex, preserving independent seeded streams and legacy draw order. Its particle formatter and mass globals are retained but unused. Every maintained mode, including the electron tester, samples its selected target geometry.

To update geometry:

1. Replace only `src/workflows/lund-creation/external/targets.h` with the reviewed RG-M version, preserving it as an exact copy. Keep `legacy/` unchanged as the comparison baseline.
2. Preserve the external API: `targets` maps names to nonempty position vectors, `ran` is a `TRandom3`, and `randomVertex(std::string)` returns a `TVector3` in cm. If upstream changes this API, adapt `TargetGeometry.cpp` as well. New target names are discovered from the map; their sampling prescription comes from the replacement function.
3. Build with `source run.csh --workflow create-lund --source uniform --build true --run false` in tcsh, or the CMake commands in the build guide. CMake detects header changes and recalculates its SHA-256; each generated manifest records `targets_sha256` for the compiled header, including uncommitted replacements.
4. Review changed vertex bounds. Geometry updates can intentionally change results relative to the frozen archive; record the reason and revised scientific validation. Update the snapshot table in the configuration guide and choose matching detector geometry and A/Z.
5. Commit the header and documentation, recording the upstream revision or download origin. Rebuild the server checkout before producing samples.

The maintained adapter compiles directly against the selected header, so there is no second geometry table to maintain.

## LUND format

We produce LUND files following the [GEMC LUND format documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html): an event header followed by fourteen-field particle records, with momentum in GeV/c, energy in GeV, mass in GeV/c², and vertices in cm. The [data contract](lund-data-contract.md) specifies the exact columns and the historical application meanings used in user-defined header fields. In particular, the GENIE process tag is not a physical cross-section weight. The maintained writer always uses the established precision, whitespace, and numbering conventions.

## Gcard provenance and field settings

We use gcards from [JeffersonLab/clas12-config, gemc directory](https://github.com/JeffersonLab/clas12-config/tree/main/gemc). The files under `config/detector/` are retained campaign/version snapshots imported through the legacy repository; they are not downloaded from the current upstream branch at runtime. Select a matching card and reconstruction YAML explicitly. Retain their versions and the simulation record's hashes for each campaign. Their surrounding integration is intentionally minimal so the snapshots can be updated as a group.

| Nominal electron beam energy | Electron bending | Torus scale | Solenoid scale |
| --- | --- | --- | --- |
| 2 GeV | Outbending | +0.5 | −1 |
| 4 GeV | Inbending | −1 | −1 |
| 6 GeV | Inbending | −1 | −1 |

For 2 GeV:

```xml
<!-- you can scale the fields here. Remember torus -1 means e- INBENDING  -->
<option name="SCALE_FIELD" value="binary_torus, 0.5"/>
<option name="SCALE_FIELD" value="binary_solenoid, -1"/>
```

For 4 and 6 GeV:

```xml
<!-- you can scale the fields here. Remember torus -1 means e- INBENDING  -->
<option name="SCALE_FIELD" value="binary_torus, -1"/>
<option name="SCALE_FIELD" value="binary_solenoid, -1"/>
```

The checked-in gcards contain these scales. The sourced submission settings select torus `0.5` at 2 GeV and `-1.0` at 4/6 GeV; the external payload applies the chosen torus scale and fixed solenoid `-1.0` on the GEMC command line. Review these explicit settings together with the selected card, YAML and GEMC module. No detector settings are inferred from a LUND filename.

The [unified external GEMC payload](../submit-simulation/worker-reference.md) documents `src/workflows/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python setup and its sourced shell bridge.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
