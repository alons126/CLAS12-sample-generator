# Run directories and outputs

For example, creating a uniform 1e sample with `--output /path/to/quickstart-output` produces the run directory `/path/to/quickstart-output/Uniform__1e__5986MeV` at the quickstart's beam energy. We call that complete directory `RUN` below. Both LUND applications use the same directory structure: creation writes `lundfiles/`, and the submitted Slurm jobs later write `mchipo/` and `reconhipo/`:

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
│           └── <PLOT_INDEX>_<HISTOGRAM>.png
├── mchipo/
│   └── mc_<PREFIX>_<INDEX>_torus<TORUS>.hipo
└── reconhipo/
    ├── recon_<PREFIX>_<INDEX>_torus<TORUS>.hipo
    └── slurm-submission-log.json
```

## Path notation

These names are placeholders, not shell variables. Replace them before running an example; do not type angle brackets into a command.

| Name | Meaning |
| --- | --- |
| `OUTPUT` | Parent supplied with creation's `--output`; for example, `/path/to/quickstart-output` |
| `RUN` | Complete resolved sample directory printed by creation; for example, `OUTPUT/Uniform__1e__5986MeV` |
| `<PREFIX>` | LUND filename prefix, recorded as `prefix` in the manifest and exported as `SAMPLE_FILE_PREFIX` to the worker |
| `<INDEX>` | One-based LUND file index; it equals the worker's `SLURM_ARRAY_TASK_ID`, not the event ID inside the file |
| `<TORUS>` | Signed scale exported as `TORUS_FIELD`; its text appears in HIPO filenames |
| `<PLOT_INDEX>` | Monitoring plot's export order, independent of the LUND file index |
| `<HISTOGRAM>` | ROOT histogram object name |
| `JOB_ID` | Slurm array ID returned by submission, not the one-based task index |

The generic directory formulas in configuration references use lowercase placeholders named after settings, such as `<target>`, `<tune>`, and `<q2-cut>`. `<beam-label>` is the complete campaign label, including `MeV` (for example, `5986MeV`); `<beam-group>` is the detector lookup group (for example, `6GeV`). Neither replaces the precise `beam-energy` setting. Angle-bracketed `<username>` denotes your login name, not a checked-in personal account.

Pass the parent `OUTPUT` to creation's `--output`, then pass the resulting `RUN/lundfiles` to submission's `--lund-dir`. To keep separate studies, use different output parents when their run names would match.

Relative paths, such as `runs/example`, need a starting directory. With [`run.csh`](../../run.csh), that directory is the repository root. When running a compiled application directly, it is the directory your shell is currently in. Paths in a creation profile use that same starting directory; paths in a submission config instead start at the config file's directory.

## Output lifecycle

The contents appear in stages:

| Stage | What exists |
| --- | --- |
| Successful LUND creation | LUND text files, the completion manifest, empty `mchipo/` and `reconhipo/`; uniform runs also contain monitoring products |
| Accepted `sbatch` submission | `slurm-submission-log.json` records the accepted job ID and resolved submission; HIPO output may not exist yet |
| Completed Slurm tasks | GEMC output appears under `mchipo/`, and COATJAVA reconstruction output appears under `reconhipo/` |

The final manifest name is a completion marker. A failed or interrupted creation may leave partial files and a temporary `.json.tmp` file. Do not submit those files as a completed run: normal submission needs the final manifest, and the resolver rejects input with only a temporary manifest. A separate [manual-input interface](../submit-simulation/configuration.md#lund-input-without-a-manifest) supports independently prepared files without a manifest; it does not establish that an interrupted creation succeeded. The [LUND data contract](../concepts/lund-data-contract.md) defines the text records and manifest fields.

If creation finds the same `RUN/` directory, it warns, deletes that directory and everything inside it, then recreates it. Submission deletes less: preview keeps existing files, while `--execute` deletes and recreates only `mchipo/` and `reconhipo/`. It leaves `lundfiles/` untouched. Read the [submission guide](../submit-simulation/guide.md) before replacing earlier simulation output.

Each Slurm task also writes scheduler `.out` and `.err` files outside `RUN`, to the farm-output path configured by the worker. See [follow jobs and verify output](../submit-simulation/verification.md) for log locations, Slurm commands, failure indicators, and HIPO checks.

The simulation and reconstruction products use the [HIPO format](https://github.com/gavalian/hipo). Downstream physics analysis is outside this repository; [CLAS12ROOT](https://github.com/JeffersonLab/clas12root/tree/master) provides ROOT interfaces for reading and analyzing CLAS12 HIPO data.
