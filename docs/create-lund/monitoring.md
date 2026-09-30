# Uniform monitoring

Monitoring belongs only to the uniform LUND creator. The physical LUND converter reports counts and provenance but creates no monitoring histograms.

Each uniform run writes every histogram once to:

```text
RUN/lundfiles/lund-creation-monitoring/<PREFIX>__monitoring_plots.root
```

It renders the same objects into:

```text
RUN/lundfiles/lund-creation-monitoring/MonitoringPlotsPath/
├── <PREFIX>__plots.pdf
├── 1_<HISTOGRAM>.png
└── ...
```

The plots cover the quantities relevant to the selected channel:

- electron momentum, theta, and phi;
- hadron momentum, theta, and phi for `eh`;
- vertex coordinates, except in the electron-tester angular scan;
- single-particle momentum/angle correlations; and
- electron–hadron correlations.

Hadron object names include both species and region: `pFD`, `nCD`, `pipFD`, and so on. Each object name also ends with the complete channel label so ROOT's statistics display identifies the sample.

Histograms use 100 bins per axis. Momentum displays extend to 1.1 times the beam energy. Electron and FD theta displays cover 0–50°; CD hadron theta displays cover 0–150°. Phi covers −180–180°, with −200–200° for paired electron–hadron phi correlations. Vx and Vy cover −5–5 cm and Vz covers −8–5 cm. One-dimensional plots use `Number of events`; two-dimensional plots use ROOT's `colz` rendering.

These products validate the generated uniform distributions. They do not measure detector acceptance and do not replace validation of GEMC and reconstructed output.
