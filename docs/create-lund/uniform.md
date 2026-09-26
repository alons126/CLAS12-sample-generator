# Uniform samples

## Generate a sample

```bash
build/debug/apps/uniform-lund-creator \
  --config config/samples/uniform-1e-5986MeV.conf \
  --events 100 --output runs
```

Uniform generation uses `--channel 1e` for one sampled electron, `--channel electron-tester` for the beam-momentum angular scan, and `--channel eh` for a trigger electron followed by the hadron selected with `--hadron proton|neutron|pip|pim`. For `eh`, `--hadron-region FD|CD` chooses the hadron acceptance. The resulting sample labels are `1e`, `electron-tester`, `epFD`, `enFD`, `epipFD`, `epimFD`, `epCD`, `enCD`, `epipCD`, and `epimCD`. A run is written below the supplied parent as `Uniform_sample_<label>_<beam MeV>MeV/`.

For example, a central-detector pi+ sample is:

```bash
build/debug/apps/uniform-lund-creator \
  --channel eh --hadron pip --hadron-region CD \
  --events 100 --output runs
```

`events` is the total run size. `events-per-file` controls splitting and defaults to 25,000. The completed manifest records every file count; submission uses the largest selected file count as the shared GEMC/reconstruction `JOB_NEVENTS` limit.

## Reviewed profiles

Every supported mode has an explicit profile at each established beam energy:

| Sample | 2.07052 GeV | 4.02962 GeV | 5.98636 GeV |
| --- | --- | --- | --- |
| 1e | `uniform-1e-2070MeV.conf` | `uniform-1e-4029MeV.conf` | `uniform-1e-5986MeV.conf` |
| epFD | `uniform-epFD-2070MeV.conf` | `uniform-epFD-4029MeV.conf` | `uniform-epFD-5986MeV.conf` |
| enFD | `uniform-enFD-2070MeV.conf` | `uniform-enFD-4029MeV.conf` | `uniform-enFD-5986MeV.conf` |
| epipFD | `uniform-epipFD-2070MeV.conf` | `uniform-epipFD-4029MeV.conf` | `uniform-epipFD-5986MeV.conf` |
| epimFD | `uniform-epimFD-2070MeV.conf` | `uniform-epimFD-4029MeV.conf` | `uniform-epimFD-5986MeV.conf` |
| epCD | `uniform-epCD-2070MeV.conf` | `uniform-epCD-4029MeV.conf` | `uniform-epCD-5986MeV.conf` |
| enCD | `uniform-enCD-2070MeV.conf` | `uniform-enCD-4029MeV.conf` | `uniform-enCD-5986MeV.conf` |
| epipCD | `uniform-epipCD-2070MeV.conf` | `uniform-epipCD-4029MeV.conf` | `uniform-epipCD-5986MeV.conf` |
| epimCD | `uniform-epimCD-2070MeV.conf` | `uniform-epimCD-4029MeV.conf` | `uniform-epimCD-5986MeV.conf` |
| Electron tester | `electron-tester-2070MeV.conf` | `electron-tester-4029MeV.conf` | `electron-tester-5986MeV.conf` |

Each file contains the relevant scientific definition: beam energy, target identity, event/file counts, seeds, momentum and angular settings, trigger prescription, and monitoring selection. Prefix, LUND layout, masses, target-vertex mode, trigger-electron momentum, and sampled hadron maximum are automatic contracts rather than repeated profile values. Supply only `--output` for the recorded profile as written; command-line options remain available for deliberate studies and override the file. Uniform profiles request 50,000,000 events and tester profiles request 1,000,000, so add a smaller `--events` value for smoke tests. The unvalidated pion/CD profiles also carry an explicit warning in their file headers.

## Production sampling contract

| Hadron/region | θ range | p minimum | Momentum distribution |
| --- | ---: | ---: | --- |
| proton FD | 5–45° | 0.3 GeV/c | 50/50 uniform-p and uniform-1/p |
| neutron FD | 5–35° | 0 | uniform-p |
| pip or pim FD | 5–45° | 0.2 GeV/c | 50/50 uniform-p and uniform-1/p |
| proton CD | 35–145° | 0.2 GeV/c | 50/50 uniform-p and uniform-1/p |
| neutron CD | 35–145° | 0 | uniform-p |
| pip or pim CD | 35–140° | 0.1 GeV/c | 50/50 uniform-p and uniform-1/p |

Every maximum momentum defaults to the beam energy. θ is uniform in theta and φ is uniform from −180° to 180°. These deliberately unphysical samples map acceptance; they do not enforce exclusive energy or momentum conservation.

> **Validation status:** uniform FD pion samples (`epipFD` and `epimFD`) and every uniform CD particle sample (`epCD`, `enCD`, `epipCD`, and `epimCD`) have not yet been tested as production samples. Automated integration checks exercise their labels, particle IDs, configured bounds, and output structure, but their complete generated distributions and detector workflow have not been validated. Do not treat them as validated production modes until those checks are complete. The established physics and legacy validation currently covers `1e`, `epFD`, `enFD`, and the electron tester.

The `1e` electron has θ 5–40°, full φ, and a 50/50 uniform-p/uniform-1/p mixture from 0.7 GeV/c to beam momentum. In `eh`, the trigger electron has beam momentum and θ=25°. Its φ is the CLAS12 sector center closest to the direction opposite the hadron, plus the beam-dependent offset: 16° at 2.07052 GeV, 7° at 4.02962 GeV, 5° at 5.98636 GeV, and 0° otherwise. This opposite-sector constraint is not required for a CD hadron, but it is retained deliberately to be sure the trigger electron is separated in the established way.

The optional `--hadron-momentum fixed --hadron-p 1` study is accepted only for a neutron, in either FD or CD. Hadron theta and phi are always sampled uniformly inside the configured detector ranges so equal-width angular bins receive comparable generated statistics for acceptance mapping.

## Electron tester

```bash
build/debug/apps/uniform-lund-creator \
  --config config/samples/electron-tester-5986MeV.conf \
  --output runs
```

The tester always scans electron θ from 5–40° and all φ at beam momentum, and samples the selected target geometry. It provides a rough estimate of where the trigger electron in electron–hadron samples should be thrown. The maintained 25° trigger prescription was selected from this scan.

## Targets, reproducibility, and masses

`--rgm-target` first resolves LUND A/Z metadata, the external [`targets.h`](../../src/lund-generation/external/targets.h) geometry key, and the default GEMC target variation. Production profiles therefore contain only `rgm-target = Ar40`; the resolved manifest contains `target = Ar`, `A = 40`, `Z = 18`, and `gemc-target-variation = rgm_fall2021_Ar`. Explicit field values remain available as later overrides for controlled studies.

`seed` controls kinematics and `vertex-seed` controls geometry. Defaults 67890 and 12345 are repeatable. `TRandom3(0)` asks ROOT to choose an automatic seed; a manifest that records zero therefore cannot reproduce the event sequence. The streams are separate so geometry draws do not shift kinematics.

Electron, proton, neutron, and charged-pion masses come directly from the external `src/lund-generation/external/targets.h` source through `particleMass()`. The photon mass is exactly zero. LUND serialization still writes every mass and derived energy to five decimal places.

## Output and diagnostics

`lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root` contains the complete monitoring set. Its legacy-style definitions cover 1e, the electron tester, and every proton, neutron, pip, and pim FD/CD channel with region-bearing hadron labels. It is the only monitoring ROOT file. Every uniform run also fills `MonitoringPlotsPath/` with PDF/PNG views of those same histograms. Empty `mchipo/` and `reconhipo/` directories preserve the downstream layout for later simulation and reconstruction. See [diagnostics](monitoring.md) and [sampling equations](../concepts/sampling-models.md).
