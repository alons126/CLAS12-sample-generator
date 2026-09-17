# Sample configuration profiles

These files describe the LUND sample being created. They do not select the user-facing workflow, configure CMake, or submit Slurm jobs. Select a profile explicitly in every LUND-creation command:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-1e-5986MeV.conf --output OUTPUT_PARENT
```

```tcsh
source run.csh --workflow create-lund --source physical \
  --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT
```

The executable installs built-in defaults, reads the named profile, then applies explicit `--key value` overrides. Unknown and repeated keys fail. Blank lines and lines beginning with `#` are ignored; inline comments, sections, quoting, and environment expansion are unsupported. Every checked-in profile groups its values under comment-only explanation sections covering their purpose, consumers, units, derived behavior, output contract, and relevant validation limits.

Profiles normally specify only `rgm-target`. The maintained target catalog first resolves its vertex geometry, A/Z metadata, and default GEMC target variation. Explicit `target`, `A`, `Z`, or `gemc-target-variation` settings are optional overrides applied afterward and should appear only when the run intentionally departs from the selected identity. All resolved values are recorded in `manifest.json` even when they are absent from the profile.

## Uniform production matrix

Every supported uniform mode has one complete profile for each established RG-M beam energy. Select the file matching the sample and beam rather than overriding a generic profile. The profiles use Ar40 geometry and A=40/Z=18, repeatable seeds, legacy-derived angular conventions, rounded PDG-based masses, and 25,000 events per file. Uniform profiles request 50,000,000 events, following the legacy production scale; tester profiles request 1,000,000 events, following its server default. Override `--events` for smaller studies.

| Sample | 2.07052 GeV | 4.02962 GeV | 5.98636 GeV |
| --- | --- | --- | --- |
| Uniform 1e | `uniform-1e-2070MeV.conf` | `uniform-1e-4029MeV.conf` | `uniform-1e-5986MeV.conf` |
| Uniform epFD | `uniform-epFD-2070MeV.conf` | `uniform-epFD-4029MeV.conf` | `uniform-epFD-5986MeV.conf` |
| Uniform enFD | `uniform-enFD-2070MeV.conf` | `uniform-enFD-4029MeV.conf` | `uniform-enFD-5986MeV.conf` |
| Uniform epipFD | `uniform-epipFD-2070MeV.conf` | `uniform-epipFD-4029MeV.conf` | `uniform-epipFD-5986MeV.conf` |
| Uniform epimFD | `uniform-epimFD-2070MeV.conf` | `uniform-epimFD-4029MeV.conf` | `uniform-epimFD-5986MeV.conf` |
| Uniform epCD | `uniform-epCD-2070MeV.conf` | `uniform-epCD-4029MeV.conf` | `uniform-epCD-5986MeV.conf` |
| Uniform enCD | `uniform-enCD-2070MeV.conf` | `uniform-enCD-4029MeV.conf` | `uniform-enCD-5986MeV.conf` |
| Uniform epipCD | `uniform-epipCD-2070MeV.conf` | `uniform-epipCD-4029MeV.conf` | `uniform-epipCD-5986MeV.conf` |
| Uniform epimCD | `uniform-epimCD-2070MeV.conf` | `uniform-epimCD-4029MeV.conf` | `uniform-epimCD-5986MeV.conf` |
| Electron tester | `electron-tester-2070MeV.conf` | `electron-tester-4029MeV.conf` | `electron-tester-5986MeV.conf` |

The 1e profiles use the updated 0.7 GeV/c minimum and 50/50 uniform-p/uniform-1/p mixture. The epFD profiles use a beam-momentum 25° trigger electron and a proton mixture from 0.3 GeV/c to the beam momentum. The enFD profiles use the same trigger prescription and uniform neutron momentum from zero to the beam momentum. All retain flat legacy theta and full phi coverage. The beam-specific trigger offsets are written explicitly as 16°, 7°, and 5°.

FD pion and all CD profiles are marked experimental inside the files. They encode the documented updated conventions but have not yet been tested as production samples; see [uniform generation](../../docs/uniform-samples.md) and [validation](../../docs/validation.md).

The electron tester profiles sample the selected target geometry and scan electron theta from 5° to 40° and full phi at beam momentum. They provide the rough estimate from which the 25° trigger-electron prescription was selected.

## Compatibility and physical-input profiles

| Profile | Purpose |
| --- | --- |
| `genie.conf` | Physical GENIE GST conversion example |
| `legacy-coderun.conf` | Archived uniform `CodeRun.cpp` compatibility settings |
| `legacy-genie-wrapper.conf` | Archived GENIE wrapper compatibility settings |

## Available common options

`output`, `beam-energy`, `rgm-target`, optional target-field overrides (`target`, `A`, `Z`, `gemc-target-variation`), `events`, `events-per-file`, `seed`, `vertex-seed`, `prefix`, and `render-plots` are common configuration keys. Every event samples its selected target geometry. Uniform prefixes are automatic unless `--prefix` explicitly overrides them. `events-per-file` defaults to 25,000 for uniform and 10,000 for physical input. `output` is normally supplied at runtime so a committed profile does not embed a machine-specific path.

## Available uniform options

Uniform profiles may set `channel`, `hadron`, `hadron-region`, `electron-theta-min`, `electron-theta-max`, `electron-momentum`, `electron-p-min`, `electron-p-max`, `hadron-theta-min`, `hadron-theta-max`, `hadron-momentum`, `hadron-p`, `hadron-p-min`, `trigger-theta`, and `trigger-phi-offset`. Sampled hadron momentum always ends at beam energy; the eh trigger electron automatically uses beam momentum.

## Available physical options

Physical profiles may set `input`, `event-generator`, `event-generator-version`, `tune`, `q2-cut`, and `gemc-version` in addition to the common options. `input` and `output` are normally supplied at runtime. The implemented physical adapter is `genie`.

The full types, units, allowed values, automatic resolutions, RG-M target catalog, and failure behavior are documented in [configuration.md](../../docs/configuration.md). The selected executable also prints its current interface:

```tcsh
source run.csh --workflow create-lund --source uniform --build false -- --help
```

```tcsh
source run.csh --workflow create-lund --source physical --build false -- --help
```
