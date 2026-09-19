# External geometry and detector inputs

## Target source and replacement

The authoritative target source is [`src/lund-generation/external/targets.h`](../src/lund-generation/external/targets.h), initially copied byte-for-byte from `legacy/GEMC-samples/include/targets.h`. Its external origin is [awild7/rgm](https://github.com/awild7/rgm/tree/main). This is the imported snapshot, not a claim that it matches today's upstream branch; the original upstream commit was not recorded.

The target header, unified GEMC submission payload, and detector cards/YAML under `config/detector/` are external snapshots. They are kept in the repository so workflows remain reproducible, while only minimal compatibility changes are applied around them. The maintained code consumes their interfaces without reformatting or rewriting the external content, so reviewed upstream replacements can be adopted with a small, explicit reference update.

Both uniform generation and physical conversion call this header's `randomVertex()` through `TargetGeometry`. The maintained RG-M catalog selects valid map keys and supplies nucleus/GEMC metadata without changing the protected header. The adapter transfers the caller's full vertex RNG state into and out of its `ran` generator under a mutex, preserving independent seeded streams and legacy draw order. Its particle formatter and mass globals are retained but unused. Every maintained mode, including the electron tester, samples its selected target geometry.

To update geometry:

1. Replace only `src/lund-generation/external/targets.h` with the reviewed external version. Keep `legacy/` unchanged as the comparison baseline.
2. Preserve the external API: `targets` maps names to nonempty position vectors, `ran` is a `TRandom3`, and `randomVertex(std::string)` returns a `TVector3` in cm. If upstream changes this API, adapt `TargetGeometry.cpp` as well. New target names are discovered from the map; their sampling prescription comes from the replacement function.
3. Build and test with `source run.csh --workflow create-lund --source uniform --build true --test true --run false` in tcsh, or the CMake commands in the build guide. CMake detects header changes and recalculates its SHA-256; each generated manifest records `targets_sha256` for the compiled header, including uncommitted replacements.
4. Review changed vertex bounds and any legacy-parity failures. Geometry updates can intentionally invalidate comparisons to the frozen archive; record the reason and revised scientific validation. Update the snapshot table in the configuration guide and choose matching detector geometry and A/Z.
5. Commit the header and documentation, recording the upstream revision or download origin. Rebuild the server checkout before producing samples.

The replacement-geometry test compiles the production adapter with a modified header, checks shifted vertices and a new target name, and checks independent interleaved RNG streams. It demonstrates that there is no second geometry table to maintain.

## LUND format

We produce LUND files following the [GEMC LUND format documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html): an event header followed by fourteen-field particle records, with momentum in GeV/c, energy in GeV, mass in GeV/c², and vertices in cm. The [data contract](data-contracts.md) specifies the exact columns and the historical application meanings used in user-defined header fields. In particular, the GENIE process tag is not a physical cross-section weight. The maintained writer always uses the established precision, whitespace, and numbering conventions.

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

The checked-in gcards contain these scales. The sourced submission settings select torus `0.5` at 2 GeV and `-1.0` at 4/6 GeV; the protected payload applies the chosen torus scale and fixed solenoid `-1.0` on the GEMC command line. Review these explicit settings together with the selected card, YAML and GEMC module. No detector settings are inferred from a LUND filename.

The [unified external GEMC payload](gemc-payload.md) documents `src/slurm-submission/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with sourced-shell setup.
