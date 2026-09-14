# Uniform samples

## Generate a small sample

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/uniform-electron.conf \
  --events-per-file 100 --output runs/electron-example
```

Use `uniform-proton.conf` or `uniform-neutron.conf` for the two-particle modes. The file prefix is a label; the manifest carries the actual channel, energy and file counts.

## Sampling prescriptions

| Channel | Sampled particle | Trigger electron |
| --- | --- | --- |
| `1e` | Electron: flat theta 5–40°, phi −180–180°, momentum 0–beam energy | The sampled electron |
| `ep` | Proton: flat theta 5–45°, phi −180–180°, fixed momentum 1 GeV by default | Momentum = beam energy, theta 25°, sector-based phi |
| `en` | Neutron: flat theta 5–35°, phi −180–180°, fixed momentum 1 GeV by default | Same trigger prescription |

Angles are **uniform in theta**, not in cos(theta). These are acceptance-study inputs, with no exclusive energy/momentum conservation constraint between the trigger electron and nucleon.

For `ep/en`, the trigger phi is the closest of −120, −60, 0, 60, 120, 180 degrees to the direction opposite the nucleon, plus the configured offset. Automatic offsets are 16° at 2.07052 GeV, 7° at 4.02962 GeV, 5° at 5.98636 GeV, and 0° otherwise. This now depends on beam energy, never on the output folder name.

Use an explicit momentum scan:

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/uniform-neutron.conf \
  --nucleon-momentum uniform --nucleon-p-min 0.3 --nucleon-p-max 3 \
  --output runs/neutron-momentum-scan
```

For the angular electron tester:

```bash
build/debug/apps/clas12-uniform \
  --config config/samples/electron-tester.conf \
  --output runs/angular-tester
```

This holds electron momentum at the beam value and fixes the vertex at the origin. `electron-momentum` affects only `1e`; the electron in `ep/en` is always the configured trigger. Nucleon settings do not affect `1e`.

## Reproducibility and diagnostics

`seed` controls kinematics and `vertex-seed` controls geometry. Both must be nonzero ROOT-compatible 32-bit seeds. Defaults use different seeds for the two streams; use distinct seeds to avoid correlations from identical RNG sequences. Repeating resolved settings and seeds with the same software/ROOT produces the same LUND data; output locations and file names may differ. Change seeds between independent runs or batches to avoid duplicating samples. This refactor does not promise byte equality with historical output, which used a time-dependent kinematic seed and lower output precision.

`monitoring.root` contains `pid_<PDG>_p_GeV`, `theta_deg`, `phi_deg`, `vx_cm`, `vy_cm`, `vz_cm`, `theta_vs_phi`, `theta_vs_p`, and `phi_vs_p` under the same PDG prefix. For 2D plots, the quantity after `vs` is the x-axis. Underflow/overflow bins retain values outside display bounds. These diagnostic names replace the old global histogram collection; see [migration](migration.md).
