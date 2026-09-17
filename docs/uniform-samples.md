# Uniform samples

## Generate a sample

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/uniform-electron.conf \
  --events 100 --output runs
```

Uniform generation has two channel shapes: `--channel 1e` writes one electron, while `--channel eh` writes a trigger electron followed by the hadron selected with `--hadron proton|neutron|pip|pim`. For `eh`, `--hadron-region FD|CD` chooses the hadron acceptance. The resulting sample labels are `1e`, `epFD`, `enFD`, `epipFD`, `epimFD`, `epCD`, `enCD`, `epipCD`, and `epimCD`. A run is written below the supplied parent as `Uniform_sample_<label>_<beam MeV>MeV/`.

For example, a central-detector pi+ sample is:

```bash
build/debug/apps/clas12-uniform \
  --channel eh --hadron pip --hadron-region CD \
  --events 100 --output runs
```

`events` is the total run size. `events-per-file` controls splitting and defaults to 25,000. The completed manifest records every file count; submission passes that exact value to GEMC and reconstruction as `JOB_NEVENTS`.

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

The `1e` electron has θ 5–40°, full φ, and a 50/50 uniform-p/uniform-1/p mixture from 0.7 GeV/c to beam momentum. In `eh`, the trigger electron has beam momentum and θ=25°. Its φ is the CLAS12 sector center closest to the direction opposite the hadron, plus the beam-dependent offset: 16° at 2.07052 GeV, 7° at 4.02962 GeV, 5° at 5.98636 GeV, and 0° otherwise. This opposite-sector constraint is not required for a CD hadron, but it is retained deliberately to be sure the trigger electron is separated in the established way.

The optional `--hadron-momentum fixed --hadron-p 1` study is accepted only for a neutron, in either FD or CD. `--hadron-angle isotropic` samples uniformly in cos(theta) inside the selected acceptance; production defaults remain flat in theta.

## Electron tester

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/electron-tester.conf \
  --output runs
```

The tester always scans electron θ from 5–40° and all φ at beam momentum, with a fixed `(0,0,-3 cm)` vertex. It provides a rough estimate of where the trigger electron in electron–hadron samples should be thrown. The maintained 25° trigger prescription was selected from this scan.

## Targets, reproducibility, and masses

`--rgm-target` resolves LUND A/Z metadata, the protected `targets.h` geometry key, and the GEMC target variation. Geometry and metadata can still be overridden independently for controlled studies.

`seed` controls kinematics and `vertex-seed` controls geometry. Defaults 67890 and 12345 are repeatable. `TRandom3(0)` asks ROOT to choose an automatic seed; a manifest that records zero therefore cannot reproduce the event sequence. The streams are separate so geometry draws do not shift kinematics.

`mass-convention=standard` is the production default and reads current values from `src/support/constants.h`, sourced from the [PDG 2026 Review of Particle Physics](https://pdg.lbl.gov/2026/listings/particle_properties.html). `legacy` selects the rounded archived constants only for exact compatibility tests.

## Output and diagnostics

`monitoring.root` contains per-PDG momentum, theta, phi, vertex, and two-dimensional distributions for every generated species. The legacy-named ROOT/PDF/PNG artifacts reproduce the archived layouts for `1e`, the tester, `epFD`, and `enFD`; new pion and CD modes rely on `monitoring.root` because no archived histogram contract exists for them. Empty `mchipo/`, `reconhipo/`, and `rootfiles/` directories preserve the downstream layout. See [diagnostics](diagnostics.md) and [sampling equations](sampling-models.md).
