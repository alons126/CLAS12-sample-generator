# External geometry and detector inputs

## Target source and replacement

The authoritative target source is [`src/common/external/targets.h`](../src/common/external/targets.h), initially copied byte-for-byte from `legacy/GEMC-samples/include/targets.h`. Its external origin is [awild7/rgm](https://github.com/awild7/rgm/tree/main). This is the imported snapshot, not a claim that it matches today's upstream branch; the original upstream commit was not recorded.

The target header, unified GEMC submission payload, and detector cards/YAML under `config/detector/` are external snapshots. They are kept in the repository so workflows remain reproducible, while only minimal compatibility changes are applied around them. The maintained code consumes their interfaces without reformatting or rewriting the external content, so reviewed upstream replacements can be adopted with a small, explicit reference update.

Both uniform generation and physical conversion call this header's `randomVertex()` through `TargetGeometry`. The maintained RG-M catalog selects valid map keys and supplies nucleus/GEMC metadata without changing the protected header. The adapter transfers the caller's full vertex RNG state into and out of its `ran` generator under a mutex, preserving independent seeded streams and legacy draw order. Its particle formatter and mass globals are retained but unused. The electron tester uses an explicit fixed `(0,0,-3 cm)` vertex and consumes no vertex random numbers.

To update geometry:

1. Replace only `src/common/external/targets.h` with the reviewed external version. Keep `legacy/` unchanged as the comparison baseline.
2. Preserve the external API: `targets` maps names to nonempty position vectors, `ran` is a `TRandom3`, and `randomVertex(std::string)` returns a `TVector3` in cm. If upstream changes this API, adapt `TargetGeometry.cpp` as well. New target names are discovered from the map; their sampling prescription comes from the replacement function.
3. Build and test with `source run.csh --build true --test true --run false` in tcsh, or the CMake commands in the build guide. CMake detects header changes and recalculates its SHA-256; each generated manifest records `targets_sha256` for the compiled header, including uncommitted replacements.
4. Review changed vertex bounds and any legacy-parity failures. Geometry updates can intentionally invalidate comparisons to the frozen archive; record the reason and revised scientific validation. Update the snapshot table in the configuration guide and choose matching detector geometry and A/Z.
5. Commit the header and documentation, recording the upstream revision or download origin. Rebuild the server checkout before producing samples.

The replacement-geometry test compiles the production adapter with a modified header, checks shifted vertices and a new target name, and checks independent interleaved RNG streams. It demonstrates that there is no second geometry table to maintain.

## LUND format

We produce LUND files following the [GEMC LUND format documentation](https://gemc.jlab.org/gemc/html/documentation/generator/lund.html): an event header followed by fourteen-field particle records, with momentum/energy/mass in GeV and vertices in cm. The [data contract](data-contracts.md) specifies the exact columns and the historical application meanings used in user-defined header fields. In particular, the GENIE process tag is not a physical cross-section weight. The default legacy precision and whitespace preserve archived output; `--lund-format precise` increases numeric precision.

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

The checked-in gcards already contain these scales. The simulation runner also passes field scales on the command line: use `--torus 0.5 --solenoid -1` at 2 GeV and `--torus -1 --solenoid -1` at 4/6 GeV, including when launching through Slurm. These explicit settings must agree with the selected campaign; the runner does not infer them from beam energy or the card filename.

The [unified external GEMC payload](gemc-payload.md) documents `src/common/external/submit_GEMC_sample.sh`, its retained monitoring fields, generator-independent inputs, installation and the boundary with Python coordination.
