# Uniform monitoring

The uniform LUND creator saves histograms and plots of the particles it generates. The physical LUND converter reports event counts and the settings used, but creates no monitoring histograms.

Each uniform run writes every histogram once to:

```text
RUN/lundfiles/lund-creation-monitoring/<PREFIX>__monitoring_plots.root
```

It renders the same objects into:

```text
RUN/lundfiles/lund-creation-monitoring/MonitoringPlotsPath/
├── <PREFIX>__plots.pdf
├── <PLOT_INDEX>_<HISTOGRAM>.png
└── ...
```

The plots cover the quantities relevant to the selected channel:

- electron momentum, $\theta$, and $\phi$;
- hadron momentum, $\theta$, and $\phi$ for `eh`;
- vertex coordinates, except in the electron-tester angular scan;
- single-particle momentum/angle correlations; and
- electron–hadron correlations.

Hadron object names include both species and region: `pFD`, `nCD`, `pipFD`, and so on. Production object names end with the complete channel label so ROOT's statistics display identifies the sample. Electron-tester objects instead use the suffix `Tester_e`.

Histograms use 100 bins per axis. Momentum displays extend to $1.1\,E_{\mathrm{beam}}$. Electron and FD $\theta$ displays cover $[0^\circ,50^\circ]$; CD hadron $\theta$ displays cover $[0^\circ,150^\circ]$. The standard range is $\phi\in[-180^\circ,180^\circ]$, extended to $[-200^\circ,200^\circ]$ for paired electron–hadron $\phi$ correlations. Vertex displays use $V_x,V_y\in[-5,5]\,\mathrm{cm}$ and $V_z\in[-8,5]\,\mathrm{cm}$. One-dimensional plots use `Number of events`; two-dimensional plots use ROOT's `colz` rendering.

Use these plots to inspect the sampled momenta, angles, and positions. They show what was generated, not what the detector reconstructed. Plot axes stay fixed when you change sampling limits. If a limit extends beyond an axis, check ROOT's underflow and overflow counts: entries below or above the displayed range. These plots alone do not measure detector acceptance or show that GEMC and reconstruction are correct.
