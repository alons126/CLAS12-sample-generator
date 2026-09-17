# Uniform samples

## Generate a small sample

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/uniform-electron.conf \
  --events 100 --output runs
```

Uniform generation creates `Uniform_sample_<channel>_<beam-energy in MeV>`, padded to four MeV digits, below the supplied output directory. For example, this produces `runs/Uniform_sample_1e_5986MeV/lundfiles/`.

`events` is the total run size. `events-per-file` controls splitting and defaults to 25,000 in the maintained application, preserving the earlier imported launcher setting. The pinned upstream generator now defaults to 10,000. Set `--events-per-file 10000` for its current file layout. The completed manifest records each file's count, which the submission workflow passes to GEMC and reconstruction as `JOB_NEVENTS`; no submission-specific split is required for simulation correctness.

Use `uniform-proton.conf` or `uniform-neutron.conf` for the two-particle modes. The file prefix is a label; the manifest carries the actual channel, energy and file counts.

## Sampling prescriptions

| Channel | Sampled particle | Trigger electron |
| --- | --- | --- |
| `1e` | Electron: flat theta 5–40°, phi −180–180°, momentum 0–beam energy | The sampled electron |
| `ep` | Proton: flat theta 5–45°, phi −180–180°, fixed momentum 1 GeV/c by default | Momentum magnitude = the beam-energy value in GeV/c (c=1), theta 25°, sector-based phi |
| `en` | Neutron: flat theta 5–35°, phi −180–180°, fixed momentum 1 GeV/c by default | Same trigger prescription |

In legacy fixed-momentum modes and in `1e`, angles are **uniform in theta**, not in cos(theta). With `--nucleon-momentum sampled`, en uses a flat cos(theta) distribution within the same legacy limits, while ep retains flat theta. Both retain uniform phi. These are acceptance-study inputs, with no exclusive energy/momentum conservation constraint between the trigger electron and nucleon.

For `ep/en`, the trigger phi is the closest of −120, −60, 0, 60, 120, 180 degrees to the direction opposite the nucleon, plus the configured offset. Automatic offsets are 16° at 2.07052 GeV, 7° at 4.02962 GeV, 5° at 5.98636 GeV, and 0° otherwise. This now depends on beam energy, never on the output folder name.

Disable fixed momentum with `--nucleon-momentum sampled`: en is isotropic inside the legacy 5–35° range with uniform momentum; ep keeps the 5–45° angular prescription and alternates uniform-p and uniform-1/p draws. Even runs contain exactly half of each proton component; odd runs have one extra uniform-p event. Momentum bounds are shared and must be positive for ep. See the [equations](sampling-models.md).

Use an explicit neutron momentum scan:

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/uniform-neutron.conf \
  --nucleon-momentum sampled --nucleon-p-min 0.3 --nucleon-p-max 3 \
  --output runs
```

For the angular electron tester:

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/electron-tester.conf \
  --output runs
```

This holds electron momentum at the beam value and fixes the vertex at `(0,0,-3 cm)`, matching the legacy Hall B engineering-center tester. `electron-momentum` affects only `1e`; the electron in `ep/en` is always the configured trigger. Nucleon settings do not affect `1e`.

Select an RG-M material/assembly with `--rgm-target`. The catalog resolves its LUND `A/Z`, protected `targets.h` geometry key, and GEMC variation. Available identifiers are `H1`, `D2`, `He4`, `C12-four-foil`, `Sn-nat-four-foil`, `Ca40`, `Ca48`, `C12-small`, `C12-large`, `Ar40`, `Sn120-large`, plus the archived `C12-legacy` and `Sn120-legacy` configurations. Geometry and metadata remain individually overridable for controlled compatibility studies.

## Reproducibility and diagnostics

`seed` controls kinematics and `vertex-seed` controls geometry. Both must be nonzero ROOT-compatible 32-bit seeds. Defaults use different seeds for the two streams; use distinct seeds to avoid correlations from identical RNG sequences. Repeating resolved settings and seeds with the same software/ROOT produces the same LUND data; output locations and file names may differ. Change seeds between independent runs or batches to avoid duplicating samples. Legacy output passes byte-for-byte comparison with every pinned upstream channel at matched seeds and settings. For upstream ep/en parity, select uniform momentum over 0.3 GeV/c to the beam energy and flat-theta sampling; the maintained default fixed profile and requested sampled profiles are different modes. Historical production files used an automatically seeded kinematic RNG; their individual events cannot be recreated unless that RNG state is available. `--lund-format precise` intentionally changes formatting and numbering.

`monitoring.root` contains `pid_<PDG>_p_GeV`, `theta_deg`, `phi_deg`, `vx_cm`, `vy_cm`, `vz_cm`, `theta_vs_phi`, `theta_vs_p`, and `phi_vs_p` under the same PDG prefix. The archived `<prefix>_plots.root` restores all original channel-specific names, binning, and electron–nucleon correlations; `legacy_histograms.root` is an identical stable-name copy. By default, `MonitoringPlotsPath/` contains the legacy-named multipage PDF and numbered histogram PNGs. The run also prepares empty `mchipo/`, `reconhipo/`, and `rootfiles/` directories. Set `--render-plots false` to skip PDF/PNG rendering while retaining the directory and ROOT artifacts. ROOT metadata and plot styling are not byte-identical; histogram bin content and errors are tested. See [diagnostics](diagnostics.md).
