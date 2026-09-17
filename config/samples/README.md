# Sample configuration profiles

These files describe the LUND sample being created. They do not select the user-facing workflow, configure CMake, or submit Slurm jobs. Select a profile explicitly in every LUND-creation command:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986.conf --output OUTPUT_PARENT
```

```tcsh
source run.csh --workflow create-lund --source physical \
  --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT
```

The executable installs built-in defaults, reads the named profile, then applies explicit `--key value` overrides. Unknown and repeated keys fail. Blank lines and lines beginning with `#` are ignored; inline comments, sections, quoting, and environment expansion are unsupported.

## Uniform production matrix

Every supported uniform mode has one complete profile for each established RG-M beam energy. Select the file matching the sample and beam rather than overriding a generic profile. The profiles use Ar40 geometry and A=40/Z=18, repeatable seeds, legacy-derived angular conventions, current PDG masses, and 25,000 events per file. Uniform profiles request 50,000,000 events, following the legacy production scale; tester profiles request 1,000,000 events, following its server default. Override `--events` for smaller studies.

| Sample | 2.07052 GeV | 4.02962 GeV | 5.98636 GeV |
| --- | --- | --- | --- |
| Uniform 1e | `uniform-1e-2070.conf` | `uniform-1e-4029.conf` | `uniform-1e-5986.conf` |
| Uniform epFD | `uniform-epfd-2070.conf` | `uniform-epfd-4029.conf` | `uniform-epfd-5986.conf` |
| Uniform enFD | `uniform-enfd-2070.conf` | `uniform-enfd-4029.conf` | `uniform-enfd-5986.conf` |
| Uniform epipFD | `uniform-epipfd-2070.conf` | `uniform-epipfd-4029.conf` | `uniform-epipfd-5986.conf` |
| Uniform epimFD | `uniform-epimfd-2070.conf` | `uniform-epimfd-4029.conf` | `uniform-epimfd-5986.conf` |
| Uniform epCD | `uniform-epcd-2070.conf` | `uniform-epcd-4029.conf` | `uniform-epcd-5986.conf` |
| Uniform enCD | `uniform-encd-2070.conf` | `uniform-encd-4029.conf` | `uniform-encd-5986.conf` |
| Uniform epipCD | `uniform-epipcd-2070.conf` | `uniform-epipcd-4029.conf` | `uniform-epipcd-5986.conf` |
| Uniform epimCD | `uniform-epimcd-2070.conf` | `uniform-epimcd-4029.conf` | `uniform-epimcd-5986.conf` |
| Electron tester | `electron-tester-2070.conf` | `electron-tester-4029.conf` | `electron-tester-5986.conf` |

The 1e profiles use the updated 0.7 GeV/c minimum and 50/50 uniform-p/uniform-1/p mixture. The epFD profiles use a beam-momentum 25° trigger electron and a proton mixture from 0.3 GeV/c to the beam momentum. The enFD profiles use the same trigger prescription and uniform neutron momentum from zero to the beam momentum. All retain flat legacy theta and full phi coverage. The beam-specific trigger offsets are written explicitly as 16°, 7°, and 5°.

FD pion and all CD profiles are marked experimental inside the files. They encode the documented updated conventions but have not yet been tested as production samples; see [uniform generation](../../docs/uniform-samples.md) and [validation](../../docs/validation.md).

The electron tester profiles use a fixed `(0,0,-3 cm)` vertex and scan electron theta from 5° to 40° and full phi at beam momentum. They provide the rough estimate from which the 25° trigger-electron prescription was selected.

## Compatibility and physical-input profiles

| Profile | Purpose |
| --- | --- |
| `genie.conf` | Physical GENIE GST conversion example |
| `legacy-coderun.conf` | Archived uniform `CodeRun.cpp` compatibility settings |
| `legacy-genie-wrapper.conf` | Archived GENIE wrapper compatibility settings |

## Available common options

`output`, `beam-energy`, `rgm-target`, `target`, `A`, `Z`, `gemc-target-variation`, `vertex-mode`, `vertex-x`, `vertex-y`, `vertex-z`, `events`, `events-per-file`, `seed`, `vertex-seed`, `prefix`, `lund-format`, `mass-convention`, and `render-plots` are common configuration keys. `events-per-file` defaults to 25,000 for uniform and 10,000 for physical input. `output` is normally supplied at runtime so a committed profile does not embed a machine-specific path.

## Available uniform options

Uniform profiles may set `channel`, `hadron`, `hadron-region`, `electron-theta-min`, `electron-theta-max`, `electron-momentum`, `electron-p-min`, `electron-p-max`, `hadron-theta-min`, `hadron-theta-max`, `hadron-momentum`, `hadron-angle`, `hadron-p`, `hadron-p-min`, `hadron-p-max`, `trigger-theta`, and `trigger-phi-offset`.

## Available physical options

Physical profiles may set `input`, `event-generator`, `event-generator-version`, `tune`, `q2-cut`, and `gemc-version` in addition to the common options. `input` and `output` are normally supplied at runtime. The implemented physical adapter is `genie`.

The full types, units, allowed values, automatic resolutions, RG-M target catalog, and failure behavior are documented in [configuration.md](../../docs/configuration.md). The selected executable also prints its current interface:

```tcsh
source run.csh --workflow create-lund --source uniform --build false -- --help
```

```tcsh
source run.csh --workflow create-lund --source physical --build false -- --help
```
