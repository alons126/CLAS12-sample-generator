# External geometry and detector inputs

## Target source and replacement

The project has two small boundaries for code obtained from RG-M: target geometry enters through [`src/workflows/lund-creation/external/targets.h`](../../src/workflows/lund-creation/external/targets.h), and each Slurm array task runs through [`src/workflows/slurm-submission/external/submit_GEMC_sample.sh`](../../src/workflows/slurm-submission/external/submit_GEMC_sample.sh). Adapters around these files avoid copying their geometry or detector commands. If RG-M publishes an update, the corresponding file can be reviewed and updated without redesigning the workflows.

The checked-in `targets.h` is an exact copy of the RG-M target source containing the latest RG-M target implementations available with GEMC 5.14 when this snapshot was adopted. Its external origin is [awild7/rgm](https://github.com/awild7/rgm/tree/main), and the target implementations are documented in CLAS12 Note 2026-001[^sportes-2026-rgm]. The file is consumed byte-for-byte through `TargetGeometry`; project-specific validation and RNG isolation remain outside it.

The GEMC submission payload was also obtained from RG-M code, but unlike `targets.h`, it has been modified for this project. It keeps the RG-M script's overall structure while accepting the generator-independent settings documented in the [worker reference](../submit-simulation/worker-reference.md). Future RG-M payload revisions should be compared with this file so detector-command changes can be incorporated without changing its project interface.

The two RG-M files and detector cards/YAML under `config/detector/` are external inputs kept in the repository so workflows remain reproducible. Project code uses them through small interfaces so an upstream replacement can be reviewed and validated in one place.

Both uniform generation and physical conversion call this header's `randomVertex()` through `TargetGeometry`. The target catalog selects valid map keys and supplies nucleus and GEMC metadata without changing the external header. The adapter transfers the caller's complete vertex RNG state into and out of the header's `ran` generator under a mutex. This keeps the vertex and kinematic random streams separate without changing the order of random draws. The header's particle formatter and mass globals are not used. Every mode, including the electron tester, samples its selected target geometry.

To update geometry:

1. Replace only `src/workflows/lund-creation/external/targets.h` with the reviewed RG-M version, preserving it as an exact copy.
2. Preserve the external API: `targets` maps names to nonempty position vectors, `ran` is a `TRandom3`, and `randomVertex(std::string)` returns a `TVector3` in cm. If upstream changes this API, adapt `TargetGeometry.cpp` as well. New target names are discovered from the map; their sampling prescription comes from the replacement function.
3. Build with `source run.csh --workflow create-lund --source uniform --build true --run false` in tcsh, or the CMake commands in the build guide. CMake detects header changes and recalculates its SHA-256; each generated manifest records `targets_sha256` for the compiled header, including uncommitted replacements.
4. Review changed vertex bounds. Geometry updates can intentionally change results relative to the frozen archive; record the reason and revised scientific validation. Update the snapshot table in the configuration guide and choose matching detector geometry and A/Z.
5. Commit the header and documentation, recording the upstream revision or download origin. Rebuild the server checkout before producing samples.

The adapter compiles directly against the selected header, so the project does not copy its geometry table.

## LUND format

We produce LUND files following the [GEMC LUND format documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html): an event header followed by fourteen-field particle records, with momentum in GeV/c, energy in GeV, mass in GeV/c², and vertices in cm. The [data contract](lund-data-contract.md) specifies every column and the project-specific meanings of user-defined header fields. In particular, the GENIE process tag is not a physical cross-section weight. The writer uses the precision, spacing, and numbering rules stated in that contract.

## Gcard provenance and field settings

We use gcards from [JeffersonLab/clas12-config, gemc directory](https://github.com/JeffersonLab/clas12-config/tree/main/gemc). The files under `config/detector/` are fixed campaign/version snapshots; they are not downloaded from the current upstream branch at runtime. Select a matching card and reconstruction YAML explicitly. Keep their versions and the simulation record's hashes for each campaign. Their small integration boundary allows the snapshots to be updated as a group.

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
