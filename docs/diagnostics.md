# Diagnostics and figure products

## 1. Common per-particle diagnostics

`monitoring.root` is filled for written events and contains independent histograms for each written PDG species. Names begin `pid_<PDG>_`:

| Suffix | Axes/range | Bins |
| --- | --- | --- |
| `p_GeV` | p: 0 to 1.1 × beam energy | 100 |
| `theta_deg` | θ: 0–180° | 100 |
| `phi_deg` | φ: −180–180° | 100 |
| `vx_cm`, `vy_cm` | x or y: −0.3–0.3 cm | 100 |
| `vz_cm` | z: −7–1 cm | 100 |
| `theta_vs_phi` | x=φ, y=θ | 100 × 100 |
| `theta_vs_p` | x=p, y=θ | 100 × 100 |
| `phi_vs_p` | x=p, y=φ | 100 × 100 |

Correlations use the corresponding one-dimensional ranges. Underflow and overflow retain values outside the display bounds. Histograms are owned by the run and detached from ROOT global directory ownership while filling.

## 2. Legacy uniform diagnostics

`legacy_histograms.root` restores the archived histogram names, axis ranges, binning, and correlations. The definitions are in [LegacyMonitoring.cpp](../src/common/LegacyMonitoring.cpp), with independent comparisons against the archived `Histograms.cpp` initializers and event functions.

| Channel | Histograms | Content |
| --- | --- | --- |
| `1e` | 9 | Electron θ, φ, p, x/y/z vertex and three angular/momentum correlations |
| `ep` | 27 | Electron and proton single-particle histograms plus nine electron–proton correlations |
| `en` | 27 | Electron and neutron single-particle histograms plus nine electron–neutron correlations |
| Angular tester | 6 | Electron θ, φ, p and three angular/momentum correlations |

Names follow the original pattern, e.g. `Theta_e_1e`, `Vz_n_en`, `Theta_p_VS_P_p_ep`, `Phi_e_VS_Theta_n_en`. In `A_VS_B`, B is the x-axis and A the y-axis. Single-particle θ ranges are 0–50°, momenta 0–1.1 × beam energy and φ −180–180°. Legacy vertex ranges are −5–5 cm; consequently Ar z entries lie in underflow. The φ_e versus φ_N pair plots retain the archived −200–200° ranges. These ranges are kept for parity even when the common diagnostics provide a more useful display.

The original unweighted histogram errors are obtained with `Sumw2` after filling. Parity tests compare names, axis bounds, every bin (including flow bins), entries and bin errors. ROOT file timestamps/object metadata are not compared byte-for-byte.

## 3. Legacy GENIE diagnostic

The converter writes `theta_e_VS_phi_e` to `legacy_histograms.root` with x=electron φ (−180–180°), y=electron θ (0–50°), and 100 × 100 bins. It fills **before process filtering**, matching the archived converter, so it includes scanned events later rejected for having no recognized process flag. Common per-PDG diagnostics include only written events. This distinction matters when comparing entry counts.

A zero-momentum electron is mapped to θ=0 in this diagnostic to avoid division by zero. Such pathological input is outside the finite-momentum parity fixture.

## 4. Rendered products

Use `--render-plots true` with either CLI. Uniform generation creates `monitoring_plots/uniform.pdf` and one PNG per legacy histogram. GENIE conversion creates `monitoring_plots/genie.pdf` and `theta_e_VS_phi_e.png`. ROOT runs in batch mode for these plots.

The numerical histograms restore compatibility; rendering uses standardized filenames and simple styles. Historical PDF bytes, canvas layout details and output folder names are not a parity contract. Keep ROOT histograms as the numerical source for later technical-note figures.
