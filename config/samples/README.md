# Sample profiles

A profile is a text file of `key = value` settings for a LUND sample. The files under [`config/samples/`](.) specify particles, beam energy, target, and sampling or conversion settings. They do not choose a workflow, control the build, or submit detector jobs.

These examples use [`run.csh`](../../run.csh) on ifarm. Before running, it discards uncommitted edits and deletes untracked and ignored files except `build/`. Commit and push development changes from your local copy first, and keep generated samples outside the ifarm repository directory. During local development, run the compiled application directly to avoid cleanup. Select a profile with `--config`:

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --output /path/to/output
```

The example selects [`uniform-1e-5986MeV.conf`](uniform-lund-creation/uniform-1e-5986MeV.conf). The application applies built-in defaults, reads the profile, then applies command-line overrides. Profiles use plain `key = value` lines with blank lines and full-line comments. Unknown or repeated keys fail.

Supply `--output` when running the command so profiles do not need paths specific to your machine. After creation succeeds, the application records every final setting in a JSON log called the completion manifest, including defaults and command-line overrides.

## Uniform profiles

Each supported uniform mode has a complete Ar40 profile for the three RG-M beam energies in [`uniform-lund-creation/`](uniform-lund-creation/):

| Mode | Profile pattern |
| --- | --- |
| electron tester | `electron-tester-{2070,4029,5986}MeV.conf` |
| 1e | `uniform-1e-{2070,4029,5986}MeV.conf` |
| epFD/enFD | `uniform-{epFD,enFD}-{2070,4029,5986}MeV.conf` |
| epipFD/epimFD | `uniform-{epipFD,epimFD}-{2070,4029,5986}MeV.conf` |
| epCD/enCD | `uniform-{epCD,enCD}-{2070,4029,5986}MeV.conf` |
| epipCD/epimCD | `uniform-{epipCD,epimCD}-{2070,4029,5986}MeV.conf` |

There are no uniform kaon profiles because kaon yields in the physical data are low. The uniform LUND creator supports protons, neutrons, and charged pions as hadrons.

Tester profiles request 1,000,000 events and production profiles request 50,000,000, with 25,000 events per file. Use `--events 100` for a small check before a long run.

The electron-tester, 1e, epFD, and enFD profiles are the production-tested modes. FD pion and all CD profiles are clearly marked as not yet production-validated. Their settings describe implemented behavior, not a validation claim.

## Physical profile

[`physical-lund-creation/genie-gst.conf`](physical-lund-creation/genie-gst.conf) is the current GENIE GST example. Supply `--input` and `--output` at runtime. Its `tune = auto` setting expects GENIE samples generated with [`eAScatteringGridSubmitter.py`](https://github.com/GENIE-MC/Generator/blob/3a50ba6d0918f62b023194eb2d6b5267b2815868/src/scripts/production/python/eAScatteringGridSubmitter.py) using `--store-comitinfo`, which saves `TUNE` in `input_options.txt`. The [physical guide](../../docs/create-lund/physical.md#automatic-genie-tune-lookup) gives the exact directory layout. If lookup fails, conversion records `unknown`. For samples generated another way or stored elsewhere, provide `--tune NAME` explicitly.

The profile does not run GENIE. It configures conversion of existing GST truth.

## Where option definitions live

The [LUND configuration reference](../../docs/create-lund/configuration.md) is authoritative for types, defaults, ranges, automatic target resolution, output layouts, and replacement behavior. The [uniform guide](../../docs/create-lund/uniform.md) defines production sampling, and the [physical guide](../../docs/create-lund/physical.md) defines the GST selection and cutoff.

Use the selected executable's `--help` for the current accepted CLI. The [all-options tutorials](../../tutorials/README.md) demonstrate the exact spelling of every public setting with deliberately arbitrary values.
