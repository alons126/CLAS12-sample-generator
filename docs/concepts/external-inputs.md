# External geometry and detector inputs

Some files are versioned in this repository but owned by an external scientific or operational source. They are protected from routine refactoring because changing them can change generated vertices, detector response, reconstruction, or batch behavior.

## Protected inputs

| Path | Origin and role | Project boundary |
| --- | --- | --- |
| `src/workflows/lund-creation/external/targets.h` | RG-M target geometry and particle masses | Read through `TargetGeometry` |
| `src/workflows/slurm-submission/external/submit_GEMC_sample.sh` | RG-M-derived Slurm worker | Called by the submission coordinator through environment variables |
| `config/detector/` | Campaign GCARD and reconstruction YAML snapshots | Selected and hashed during submission |

Do not edit these as part of a general cleanup. Treat a change as a reviewed source replacement with recorded origin and production validation.

## Target geometry adapter

The checked-in `targets.h` is kept as an exact source copy from the [RG-M repository](https://github.com/awild7/rgm). That repository also contains RG-M analysis and utility code that may be useful for downstream work, but it is not a runtime dependency of this project. `TargetGeometry.cpp` is the only project source that includes the header. The adapter:

- validates a resolved geometry name;
- samples one position in centimeters;
- isolates the external global RNG behind caller-owned state and a mutex; and
- exposes the external electron, proton, neutron, and charged-pion masses.

Project code does not copy the geometry table or mass constants. The target catalog maps user-facing target variations to geometry keys. Every manifest records a SHA-256 hash of the header compiled into the executable.

To update the header:

1. Record the upstream revision or download source.
2. Replace the file without project-specific edits.
3. If its API changed, update only the adapter boundary needed to consume it.
4. Rebuild so the new hash enters generated provenance.
5. Review target mappings, vertex bounds, monitoring ranges, and affected documentation.
6. Validate generated distributions and the matching detector configuration before production use.

The target implementations and RG-M variations are described in CLAS12 Note 2026-001[^sportes-2026-rgm].

## Detector resources

Files under `config/detector/` are fixed campaign snapshots. GCARD files define GEMC detector geometry and configuration; YAML files define COATJAVA reconstruction settings. The submission record hashes both selected files.

For each beam group (`2GeV`, `4GeV`, or `6GeV`), `GEMC_GCARDs_<beam-group>/<GEMC-version>/` holds GCARDs, while `COATJAVA_YAML_configs_<beam-group>/<COATJAVA-version>/` holds reconstruction YAMLs. The checked-in YAML release is 10.0.7. Submission selects each directory using its corresponding software-version option. Reconstruction configurations are shared across GEMC versions rather than duplicated inside their directories.

GCARD configurations come from the [`gemc` directory in `JeffersonLab/clas12-config`](https://github.com/JeffersonLab/clas12-config/tree/main/gemc). Reconstruction YAML configurations are available in its [`coatjava` directory](https://github.com/JeffersonLab/clas12-config/tree/main/coatjava). The current checked-in YAML files were obtained through the [RG-M repository](https://github.com/awild7/rgm). These snapshots are read locally during submission rather than downloaded from upstream at runtime.

Standard field policy is:

| Nominal beam | Torus field configuration | Torus | Solenoid |
| --- | --- | ---: | ---: |
| $2\,\mathrm{GeV}$ | outbending | $+0.5$ | $-1.0$ |
| $4\,\mathrm{GeV}$ | inbending | $-1.0$ | $-1.0$ |
| $6\,\mathrm{GeV}$ | inbending | $-1.0$ | $-1.0$ |

For a simulation campaign matched to data, both torus and solenoid scales must match the values recorded in the data. The worker passes these scales on the GEMC command line, and the selected GCARD must declare the same values. The [submission guide](../submit-simulation/guide.md#magnetic-field-consistency) explains how to inspect the data's `RUN::config` bank and gives the required GCARD entries. Scientific settings are never inferred from a LUND filename.

## Worker payload

Unlike the exact target-header copy, the RG-M worker was adapted to accept generator-independent variables and a complete file prefix. Its concrete GEMC and `recon-util` commands remain localized there. When an upstream worker changes, compare the command and scheduler behavior and preserve the documented interface unless the coordinator, tutorials, and validation are updated together.

See the [worker reference](../submit-simulation/worker-reference.md) for the environment contract.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
