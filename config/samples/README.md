# Sample configuration profiles

These files describe the LUND sample being created. They do not select the user-facing workflow, configure CMake, or submit Slurm jobs. Select a profile explicitly in every LUND-creation command:

```tcsh
source run.csh --workflow create-lund --source uniform \
  --config config/samples/uniform-electron.conf --output OUTPUT_PARENT
```

```tcsh
source run.csh --workflow create-lund --source physical \
  --config config/samples/genie.conf --input 'GST_GLOB' --output OUTPUT_PARENT
```

The executable installs built-in defaults, reads the named profile, then applies explicit `--key value` overrides. Unknown and repeated keys fail. Blank lines and lines beginning with `#` are ignored; inline comments, sections, quoting, and environment expansion are unsupported.

## Profile inventory

| Profile | Purpose |
| --- | --- |
| `uniform-electron.conf` | Production uniform 1e acceptance sample |
| `uniform-proton.conf` | Production epFD 50/50 uniform-p and uniform-1/p sample |
| `uniform-neutron.conf` | Production enFD uniform-p sample from zero to beam momentum |
| `uniform-proton-sampled.conf` | Explicit alias profile for the production epFD mixture |
| `uniform-neutron-sampled.conf` | Explicit enFD uniform-momentum profile with flat legacy theta and zero lower bound |
| `electron-tester.conf` | Beam-momentum electron with fixed `(0,0,-3 cm)` vertex |
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
