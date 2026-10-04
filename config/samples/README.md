# Sample profiles

Profiles under `config/samples/` describe a LUND sample. They do not choose a workflow, control CMake, or submit detector jobs.

The examples use `run.csh` on a disposable ifarm checkout. Its refresh discards tracked edits and removes untracked and ignored files except `build/`. Commit and push valuable changes first; for local development, invoke the compiled application directly. Keep output outside the ifarm checkout. Select a profile explicitly:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --output /path/to/output
```

The application applies built-in defaults, reads the profile, then applies command-line overrides. Profiles use plain `key = value` lines with blank lines and full-line comments. Unknown or repeated keys fail.

`output` is normally supplied at runtime so a checked-in profile contains no machine-specific path. Every resolved value, including defaults and CLI overrides, is recorded in the completion manifest.

## Uniform profiles

Each supported uniform mode has a complete Ar40 profile for the three RG-M beam energies:

| Mode | Profile pattern |
| --- | --- |
| 1e | `uniform-1e-{2070,4029,5986}MeV.conf` |
| epFD / enFD | `uniform-{epFD,enFD}-{2070,4029,5986}MeV.conf` |
| epipFD / epimFD | `uniform-{epipFD,epimFD}-{2070,4029,5986}MeV.conf` |
| epCD / enCD | `uniform-{epCD,enCD}-{2070,4029,5986}MeV.conf` |
| epipCD / epimCD | `uniform-{epipCD,epimCD}-{2070,4029,5986}MeV.conf` |
| electron tester | `electron-tester-{2070,4029,5986}MeV.conf` |

Production profiles request 50,000,000 events and tester profiles request 1,000,000, with 25,000 events per file. Override `--events` for a smoke test.

The 1e, epFD, enFD, and electron-tester profiles are the production-tested modes. FD pion and all CD profiles are clearly marked as not yet production-validated. Their settings describe implemented behavior, not a validation claim.

## Physical profile

`physical-lund-creation/genie-gst.conf` is the current GENIE GST example. Supply `--input` and `--output` at runtime. Its `tune = auto` setting reads the exact `TUNE` value from the standard `input_options.txt` layout and records `unknown` when discovery is not possible.

The profile does not run GENIE. It configures conversion of existing GST truth.

## Where option definitions live

The [LUND configuration reference](../../docs/create-lund/configuration.md) is authoritative for types, defaults, ranges, automatic target resolution, output layouts, and replacement behavior. The [uniform guide](../../docs/create-lund/uniform.md) defines production sampling, and the [physical guide](../../docs/create-lund/physical.md) defines the GST selection and cutoff.

Use the selected executable's `--help` for the current accepted CLI. The [all-options tutorials](../../tutorials/README.md) demonstrate the exact spelling of every public setting with deliberately arbitrary values.
