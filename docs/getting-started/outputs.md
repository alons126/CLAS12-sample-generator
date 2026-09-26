# Outputs and completion

Both LUND sources use the same completed-run boundary:

```text
RUN/
├── lundfiles/
│   ├── PREFIX_1.txt
│   ├── PREFIX_2.txt
│   └── lund-creation-monitoring/
│       ├── lund-creation-log.json
│       ├── PREFIX_monitoring_plots.root   # uniform only
│       └── MonitoringPlotsPath/           # uniform only
├── mchipo/                                # prepared by uniform creation or submission
└── reconhipo/                             # prepared by uniform creation or submission
    ├── slurm-submission-log.json          # resolved submission, Git, and input provenance
    └── recon_PREFIX_INDEX_torusSCALE.hipo # reconstructed task output after Slurm runs
```

`lund-creation-log.json` is published last and marks a consumable run. It includes resolved LUND settings and full configure-time Git information. A failed creation may leave partial files for inspection but no completion manifest. Submission reads the manifest's exact file list and event counts rather than guessing from directory names.

With `--execute`, submission recreates `mchipo/` and `reconhipo/`, then atomically writes `slurm-submission-log.json` before calling `sbatch`. Preview stays read-only and does not create this log. The log records all resolved submission parameters, the exact command, runtime Git information, and hashes of the GCARD, reconstruction YAML, and worker payload.

Uniform creation also produces ROOT monitoring and rendered PDF/PNG plots. Physical conversion produces summaries and provenance but no monitoring histograms. GEMC and reconstruction outputs appear only after the separate submission workflow executes.

For field-level LUND and manifest definitions, see the [LUND data contract](../concepts/lund-data-contract.md). For replacement and recovery behavior, see the [creation configuration](../create-lund/configuration.md) and [submission guide](../submit-simulation/guide.md).
