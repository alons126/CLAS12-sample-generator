# Uniform monitoring and figure products

## 1. Scope and output file

Monitoring is produced only by uniform LUND generation. Physical event-generator conversion copies supported truth particles into LUND and creates no ROOT monitoring file, PDF, or PNG.

Each uniform run stores every monitoring histogram exactly once in:

```text
lundfiles/lund-gen-monitoring/<prefix>_monitoring_plots.root
```

`UniformMonitoring` is implemented beside the generator in [`src/lund-generation/uniform-lund-creator/`](../../src/lund-generation/uniform-lund-creator). There is no separate shared monitoring layer because physical conversion does not consume it.

## 2. Legacy plot contract

The maintained definitions preserve the archived plot organization:

- 100 bins per axis.
- Momentum range 0 to 1.1 times the beam energy and axis unit `[GeV]`.
- Electron and FD polar-angle display range 0–50 degrees.
- CD hadron polar-angle display range 0–150 degrees, covering the configured CD generation ranges.
- Azimuth range −180–180 degrees; paired electron-hadron azimuth correlations retain −200–200 degrees.
- Vertex x/y ranges −5–5 cm and vertex z range −8–5 cm. This common z range aims to cover the target position of every RG-M target[^sportes-2026-rgm][^rgm-analysis-note].
- One-dimensional y-axis title `Number of events`.
- Centered axis titles, title size 0.06 and label size 0.0425.
- A 1000×750 grid canvas with bottom/left/right margins 0.14/0.16/0.12.
- One-dimensional plots drawn normally and two-dimensional plots drawn with `colz`.

The 1e and electron-tester histogram names and definitions remain the archived ones. Electron-hadron monitoring retains the archived electron, hadron, vertex, single-particle correlation, and electron-hadron correlation families while adding particle and detector-region labels.

## 3. Hadron and region notation

A hadron `part` detected in the Central Detector or Forward Detector is written as `partCD` or `partFD`. Plain ROOT object names use `p`, `n`, `pip`, or `pim`; displayed ROOT titles use `p`, `n`, `#pi^{+}`, or `#pi^{-}`.

Examples:

```cpp
TH1D("P_pFD_epFD",
     "P_{pFD} in (e,e'pFD) sample;P_{pFD} [GeV]",
     100, 0, Ebeam * 1.1);

TH1D("P_pipCD_epipCD",
     "P_{#pi^{+}CD} in (e,e'#pi^{+}CD) sample;P_{#pi^{+}CD} [GeV]",
     100, 0, Ebeam * 1.1);
```

The region-bearing particle token is used consistently in momentum, theta, phi, vertex, particle-correlation, and electron-hadron correlation histograms. Each ROOT object name ends with the complete resolved channel label (`epFD`, `enFD`, `epipFD`, `epimFD`, or its CD counterpart). ROOT therefore shows the actual uniform channel in the statistics box.

## 4. Rendered products

Every uniform generation run renders the same histograms stored in the ROOT file into:

```text
lundfiles/lund-gen-monitoring/MonitoringPlotsPath/
├── Uniform_<sample-label>_plots_<beam>MeV.pdf
├── 1_<histogram-name>.png
├── 2_<histogram-name>.png
└── ...
```

Rendering is part of the uniform output contract for every channel and does not create a second ROOT file.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
