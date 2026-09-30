# Outputs and completion

Both LUND sources use the same completed-run boundary:

```text
RUN/
├── lundfiles/
│   ├── <PREFIX>_1.txt
│   ├── <PREFIX>_2.txt
│   └── lund-creation-monitoring/
│       ├── lund-creation-log.json
│       ├── <PREFIX>__monitoring_plots.root  # uniform only
│       └── MonitoringPlotsPath/           # uniform only
│           ├── <PREFIX>__plots.pdf
│           └── <INDEX>_<HISTOGRAM>.png
├── mchipo/                                # prepared by either LUND source or submission
└── reconhipo/                             # prepared by either LUND source or submission
    ├── slurm-submission-log.json          # resolved submission, Git, and input provenance
    └── recon_<PREFIX>_<INDEX>_torus<SCALE>.hipo # reconstructed task output after Slurm runs
```

`lund-creation-log.json` is published last and marks a consumable run. It includes resolved LUND settings and full configure-time Git information. A failed creation may leave partial files for inspection but no completion manifest. Submission reads the manifest's exact file list and event counts rather than guessing from directory names.

Both LUND sources create empty `mchipo/` and `reconhipo/` directories as part of the completed-run layout. Submission inspects both paths before changing either one. Preview preserves existing contents, explains that execution would clear them, and creates and verifies either directory when missing; it does not create a submission log. With `--execute`, submission warns before deleting each existing directory and its contents, recreates both directories empty to clear previous run output, and calls `sbatch`. After Slurm accepts the array and returns a numeric job ID, submission atomically writes `slurm-submission-log.json`. The log records that job ID, all resolved submission parameters, the exact command, runtime Git information, and hashes of the GCARD, reconstruction YAML, and worker payload. If log publication then fails, the accepted array remains submitted.

Each submitted jobs has an `.err` and an `.out` files associated with it in the `farm-out` directory. The job ID from `sbatch` and name from the code can be used to locate the relevant job's files. In general, environment being used at the submission stage is forwarded into the jobs on the ifarm. The echo commands in [`submit_GEMC_sample.sh`](src/workflows/slurm-submission/external/submit_GEMC_sample.sh) are therefore used to validate proper definitions of each job parameters.

Uniform creation also produces ROOT monitoring and rendered PDF/PNG plots. Physical conversion produces summaries and provenance but no monitoring histograms. GEMC and reconstruction outputs appear only after the separate submission workflow executes. For field-level LUND and manifest definitions, see the [LUND data contract](../concepts/lund-data-contract.md). For replacement and recovery behavior, see the [creation configuration](../create-lund/configuration.md) and [submission guide](../submit-simulation/guide.md).
