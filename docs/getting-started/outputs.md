# Run directories and outputs

Both LUND sources create the same run boundary:

```text
RUN/
├── lundfiles/
│   ├── <PREFIX>_1.txt
│   ├── <PREFIX>_2.txt
│   └── lund-creation-monitoring/
│       ├── lund-creation-log.json
│       ├── <PREFIX>__monitoring_plots.root   # uniform only
│       └── MonitoringPlotsPath/             # uniform only
│           ├── <PREFIX>__plots.pdf
│           └── <INDEX>_<HISTOGRAM>.png
├── mchipo/
│   └── mc_<PREFIX>_<INDEX>_torus<SCALE>.hipo
└── reconhipo/
    ├── recon_<PREFIX>_<INDEX>_torus<SCALE>.hipo
    └── slurm-submission-log.json
```

The contents appear in stages:

| Stage | What exists |
| --- | --- |
| Successful LUND creation | LUND text files, the completion manifest, empty `mchipo/` and `reconhipo/`; uniform runs also contain monitoring products |
| Accepted `sbatch` submission | `slurm-submission-log.json` records the accepted job ID and resolved submission; HIPO output may not exist yet |
| Completed Slurm tasks | GEMC output appears under `mchipo/`, and COATJAVA reconstruction output appears under `reconhipo/` |

The final manifest name is a completion marker. A failed or interrupted creation may leave partial files and a temporary `.json.tmp` file. Do not submit those files as a completed run: normal submission needs the final manifest, and the resolver rejects input with only a temporary manifest. A separate [manual-input interface](../submit-simulation/guide.md#lund-input-without-a-manifest) supports independently prepared files without a manifest; it does not establish that an interrupted creation succeeded. The [LUND data contract](../concepts/lund-data-contract.md) defines the text records and manifest fields.

The creation workflow replaces an existing resolved `RUN/` directory after warning. Submission has a different boundary: preview preserves existing simulation output, while `--execute` replaces only `mchipo/` and `reconhipo/` and preserves `lundfiles/`. The [submission guide](../submit-simulation/guide.md) defines those actions and the recovery implications.

Each Slurm task also writes scheduler `.out` and `.err` files to the farm-output path configured by the worker. The worker echoes its received settings for diagnosis; those lines report values but do not validate that the values are scientifically correct. After jobs finish, inspect every task state and log, compare the output inventory with the submitted array, and open at least one reconstructed file with `hipo-utils -dump`.

The simulation and reconstruction products use the [HIPO format](https://github.com/gavalian/hipo). Downstream physics analysis is outside this repository; [CLAS12ROOT](https://github.com/JeffersonLab/clas12root/tree/master) provides ROOT interfaces for reading and analyzing CLAS12 HIPO data.
