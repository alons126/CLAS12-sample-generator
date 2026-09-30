# Simulation submission guide

Run submission from a csh/tcsh login shell on ifarm after LUND creation has published its completion manifest.

## Preview one run

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles
```

Preview is the default. It resolves the sample, loads and verifies the selected GEMC environment, checks COATJAVA and all detector inputs, inspects both simulation-output paths, creates a missing `mchipo/` or `reconhipo/` directory, and prints the exact `sbatch` command. It preserves existing output and does not call `sbatch` or write a submission log.

Add `--execute` only after reviewing that report:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles \
    --execute
```

Execution warns, removes the exact existing `mchipo/` and `reconhipo/` directories recursively, recreates them empty, and submits the array. It never removes `lundfiles/`. The `--execute` switch is CLI-only; a configuration file cannot enable it.

## How inputs are resolved

Settings follow this precedence:

```text
command line -> optional submission config -> completion manifest -> defaults
```

CLI paths are resolved from the repository root. Paths inside a submission config are resolved from that config file. The parser accepts plain `key = value` lines, blank lines, and full-line comments. Unknown, duplicate, and empty keys fail.

Paths forwarded to the external worker may contain only letters, digits, `/`, `_`, `-`, and `.`. This restriction is checked before submission because the worker passes those paths to external commands.

The manifest supplies the exact LUND inventory and per-file event counts. The array size defaults to the selected file count, and the shared `JOB_NEVENTS` limit defaults to the largest selected file count. `--num-jobs N` selects the first N files. `--events-per-job N` replaces the shared event limit; it does not rewrite or recount the files.

Truth metadata from a manifest cannot be contradicted by an override. Detector policy can be changed independently: GEMC version, compatible detector target variation, GCARD, YAML, torus scale, custom clas12Tags directory, and Slurm job name.

## Submission options

| Option | Meaning |
| --- | --- |
| `--lund-dir DIRECTORY` | Existing `RUN/lundfiles`; repeat to process several samples in order |
| `--config FILE` | Optional submission `key = value` file |
| `--num-jobs N` | Select the first N LUND files |
| `--events-per-job N` | Override the common GEMC/reconstruction event limit |
| `--gemc-version VERSION` | Select GEMC resources; default 5.14 |
| `--gemc-target-variation NAME` | Override simulation target variation without relabeling truth target |
| `--gcard FILE` | Explicit GEMC detector card |
| `--yaml FILE` | Explicit COATJAVA reconstruction settings |
| `--torus SCALE` | Override the beam-dependent torus scale |
| `--job-name NAME` | Override the metadata-derived Slurm job name |
| `--clas12tags-dir DIRECTORY` | Use a reviewed custom clas12Tags checkout as `GEMC_DATA_DIR` |
| `--clear-farm-out true|false` | With execution, optionally delete files directly inside one exact farm-output directory; default false |
| `--farm-out DIRECTORY` | Required exact cleanup target when farm-output cleanup is true |
| `--execute` | Replace simulation output and call `sbatch` |

The following options describe LUND input that has no manifest. With a manifest, conflicting truth values are rejected rather than used to relabel the run.

| Option | Meaning |
| --- | --- |
| `--source uniform|physical` | Required input kind |
| `--beam-energy GeV` | Required truth beam energy |
| `--target ID` | Required truth target identity |
| `--prefix NAME` | Required filename prefix before `_INDEX.txt` |
| `--channel NAME` | Uniform `1e`, `electron-tester`, `eh`, or a complete label such as `epFD` |
| `--hadron NAME`, `--hadron-region FD|CD` | Particle and region required when `channel=eh` |
| `--event-generator NAME` | Physical adapter label; default `genie-gst` |
| `--tune NAME` | Physical tune/model label; default `unknown` |
| `--q2-cut NAME` | Physical upstream-selection label; default `unknown`, with no cut applied during submission |

Use `source run.csh --workflow submit --help` for the live interface. [`config/submission.conf`](../../config/submission.conf) is an example, not an automatically loaded site profile.

## Detector defaults

GEMC defaults to 5.14 because that release contains the RG-M argon target and corrected one-foil carbon target used by this project.[^sportes-2026-rgm] CLAS12 GEMC detector data and available version directories are maintained in [`gemc/clas12Tags`](https://github.com/gemc/clas12Tags). The default detector resources are selected from the manifest's target variation, beam energy, and submission-time GEMC version.

For 2.07052 GeV, the torus default is +0.5. For 4.02962 and 5.98636 GeV, it is −1.0. The worker always applies solenoid −1.0. Other beam energies require explicit `--gcard`, `--yaml`, and `--torus` values.

The coordinator distinguishes the nearest-MeV sample label (`2070MeV`, `4029MeV`, or `5986MeV`) from the detector-resource group (`2GeV`, `4GeV`, or `6GeV`). These are naming and lookup values, not alternate beam energies.

When `--clas12tags-dir` is absent, the coordinator checks the shared versioned clas12Tags directory, loads `gemc/<version>`, confirms that `GEMC_DATA_DIR` matches the request, and verifies that the resolved `gemc` executable belongs to it. A custom clas12Tags checkout replaces the data directory but not the selected GEMC executable checks. The verified child environment is exported to Slurm.

## Submit several runs

Repeat `--lund-dir`:

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/run-a/lundfiles \
    --lund-dir /shared/run-b/lundfiles \
    --num-jobs 5
```

Every sample is fully resolved before any output replacement or submission begins. The coordinator then processes samples in order and creates one independent array per sample. If a later sample fails, later samples are skipped, but arrays already accepted by Slurm remain submitted.

## LUND input without a manifest

Manifest-free input is supported for archived files. Supply `source`, beam energy, target, prefix, event limit, and source-specific metadata explicitly. Files must be contiguous from `PREFIX_1.txt` through `PREFIX_N.txt` because the resolver will not guess gaps or scan every file to count events.

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/archive/lundfiles \
    --source uniform \
    --beam-energy 2.07052 \
    --target Ar40 \
    --channel eh \
    --hadron neutron \
    --hadron-region FD \
    --prefix Uniform__enFD__2070MeV \
    --events-per-job 10000 \
    --gemc-target-variation rgm_fall2021_Ar
```

Uniform input needs `channel`; `eh` also needs hadron and region. Physical input may supply event generator, tune, and Q² label, defaulting to `genie-gst`, `unknown`, and `unknown`. A detector variation or explicit GCARD is required.

## Submission record and failures

After `sbatch` returns `Submitted batch job NUMBER`, the coordinator prints the numeric job ID and atomically writes `RUN/reconhipo/slurm-submission-log.json`. It records the ID, exact command, resolved parameters, runtime Git state, and SHA-256 hashes of the GCARD, YAML, and worker payload.

If Slurm accepts an array but its response cannot be parsed, or writing the submission log fails afterward, inspect Slurm before retrying. Retrying blindly can create a duplicate array. An earlier accepted array is never cancelled automatically when a later sample fails.

Optional farm-output cleanup deletes only direct files in the exact reviewed directory and runs once per invocation. It requires both `--clear-farm-out true` and `--farm-out DIRECTORY`; omit both to preserve scheduler logs.

## Verify the finished array

The coordinator does not poll task states, retry failures, reconcile outputs, or inspect HIPO content. After the array finishes:

1. Review every array task in Slurm and its `.out` and `.err` files or in the [outstanding Jobs dashboard here](https://scicomp.jlab.org/scicomp/slurmJob/activeJob?user=asportes&account=clas12); other users can adjust those filters or use `squeue -u <username>`.
2. Compare the submitted task range with `RUN/mchipo/` and `RUN/reconhipo/`.
3. Open at least one reconstructed file:

   ```text
   hipo-utils -dump RUN/reconhipo/<file>.hipo
   ```

The file must open and display CLAS12 data banks. This is a smoke test only; one readable file does not establish that the remaining tasks succeeded or that the campaign is scientifically valid.

Useful Slurm commands include `squeue -u <username>` and `scancel <job-id>`. Use `scancel --user=<username>` only when intentionally cancelling all jobs owned by that account.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
