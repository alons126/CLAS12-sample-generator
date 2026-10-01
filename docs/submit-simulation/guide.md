# Simulation submission guide

Run submission from a csh/tcsh login shell on ifarm after LUND creation has published its completion manifest.

## Preview one run

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /shared/path/to/run/lundfiles
```

Preview is the default. It resolves the sample, loads and verifies the selected GEMC and COATJAVA environments, checks all detector inputs, inspects both simulation-output paths, creates a missing `mchipo/` or `reconhipo/` directory, and prints the exact `sbatch` command. It preserves existing output and does not call `sbatch` or write a submission log.

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

Truth metadata from a manifest cannot be contradicted by an override. Detector policy can be changed independently: GEMC and COATJAVA versions, compatible detector target variation, GCARD, YAML, torus scale, custom clas12Tags directory, and Slurm job name. Neither software version is inherited from LUND-creation metadata.

## Submission options

| Option | Meaning |
| --- | --- |
| `--lund-dir DIRECTORY` | Existing `RUN/lundfiles`; repeat to process several samples in order |
| `--config FILE` | Optional submission `key = value` file |
| `--num-jobs N` | Select the first N LUND files |
| `--events-per-job N` | Override the common GEMC/reconstruction event limit |
| `--gemc-version VERSION` | Select GEMC software and GCARD resources; default 5.14 |
| `--coatjava-version VERSION` | Select COATJAVA software and YAML resources; default 10.0.7 |
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
| `--channel NAME` | Uniform `1e`, `electron-tester`, `eh`, or a [complete electron-hadron label](../create-lund/uniform.md#electron-hadron-labels) such as `epFD` |
| `--hadron NAME`, `--hadron-region FD|CD` | Particle and region required when `channel=eh` |
| `--event-generator NAME` | Physical adapter label; default `genie-gst` |
| `--tune NAME` | Physical tune/model label; default `unknown` |
| `--q2-cut NAME` | Physical upstream-selection label; default `unknown`, with no cut applied during submission |

Use `source run.csh --workflow submit --help` for the live interface. [`config/submission.conf`](../../config/submission.conf) is an example, not an automatically loaded site profile.

## Detector defaults

GEMC defaults to 5.14 because that release contains the RG-M argon target and corrected one-foil carbon target used by this project[^sportes-2026-rgm]. CLAS12 GEMC detector data and available version directories are maintained in [`gemc/clas12Tags`](https://github.com/gemc/clas12Tags). GCARD defaults are selected from the manifest's target variation, beam energy, and submission-time GEMC version under `config/detector/GEMC_GCARDs_<beam-group>/<GEMC-version>/`.

COATJAVA defaults to 10.0.7. Its version independently selects the reconstruction software and the YAML directory, `config/detector/COATJAVA_YAML_configs_<beam-group>/<COATJAVA-version>/`. Only 10.0.7 YAML snapshots are currently checked in. Another release needs a reviewed YAML supplied with `--yaml` if its default file is absent. Explicit `--gcard` and `--yaml` paths override resource lookup, not software selection. Check that the selected files are compatible with the requested releases.

For $E_{\mathrm{beam}}=2.07052\,\mathrm{GeV}$, the torus default is $+0.5$. For $E_{\mathrm{beam}}=4.02962\,\mathrm{GeV}$ and $5.98636\,\mathrm{GeV}$, it is $-1.0$. The worker always applies solenoid scale $-1.0$. Other beam energies require explicit `--gcard`, `--yaml`, and `--torus` values.

### Magnetic-field consistency

For simulation matched to a data campaign, the torus and solenoid scales must match the data, including their signs. Check a reconstructed data HIPO file before selecting the simulation settings:

1. Go to the directory containing the data's HIPO files. Run `hipo-utils` to see the available commands.
2. Open a representative file, for example:

   ```bash
   hipo-utils -dump rec_clas_015652.evio.00100-00104.hipo
   ```

3. At the interactive bank prompt, type `RUN::config`.
4. Read the `torus` and `solenoid` fields. These are signed relative field scales, not magnetic-field strengths in tesla[^run-field-scales]. Check each run configuration used by the campaign.

The selected GCARD and the Slurm job must use the same torus scale. The checked-in $2\,\mathrm{GeV}$ cards describe the outbending configuration and match the submission default `--torus 0.5`:

```xml
<!-- you can scale the fields here. Remember torus -1 means e- INBENDING  -->
<option name="SCALE_FIELD" value="binary_torus, 0.5"/>
<option name="SCALE_FIELD" value="binary_solenoid, -1"/>
```

The checked-in $4\,\mathrm{GeV}$ and $6\,\mathrm{GeV}$ cards describe the inbending configuration and match the submission default `--torus -1.0`:

```xml
<!-- you can scale the fields here. Remember torus -1 means e- INBENDING  -->
<option name="SCALE_FIELD" value="binary_torus, -1"/>
<option name="SCALE_FIELD" value="binary_solenoid, -1"/>
```

When using a custom GCARD or overriding `--torus`, inspect its `SCALE_FIELD` entries and change them together. The current worker fixes the solenoid scale at $-1.0$ and exposes no solenoid override. If the data uses another solenoid scale, updating only the GCARD is insufficient: the worker configuration must also be changed and validated before submission. A mismatch means the data, selected card, and submitted command describe different magnetic-field setups.

The coordinator distinguishes the nearest-MeV sample label (`2070MeV`, `4029MeV`, or `5986MeV`) from the detector-resource group (`2GeV`, `4GeV`, or `6GeV`). These are naming and lookup values, not alternate beam energies.

The coordinator unloads and loads GEMC and uses `module switch coatjava/<version>` for COATJAVA in a private child environment. For each selection, it first prints the switching notice and native module-change messages, then a blank line, the `<module>/<version> module configuration:` heading, and `module show` output. This order applies in both preview and execution. After both selections, it prints `module list` from that environment so you can see all loaded modules, including dependencies, that Slurm will inherit. It checks and prints the loaded COATJAVA release using `LOADEDMODULES` and confirms that `recon-util` is available in `PATH`. It does not predict, compare, or validate COATJAVA installation directories. GEMC's data and executable checks remain unchanged. Missing or conflicting releases, failed module commands, or unavailable programs stop submission before simulation output is replaced.

When `--clas12tags-dir` is absent, GEMC uses the shared versioned clas12Tags directory. A custom clas12Tags checkout replaces the data directory but not the selected GEMC executable checks. The verified child environment is exported to Slurm; the interactive login shell stays unchanged. Default job names include both `GEMC<version>` and `COATJAVA<version>` for uniform and physical samples; `--job-name` overrides the name without changing either release.

Each array task also prints `Used modules:` and runs `module list` before GEMC, so the scheduler logs show the modules loaded inside the job. Check both `.out` and `.err` files because the module list may appear in the error stream. See the [worker reference](worker-reference.md#environment-interface).

Module commands in the submission report appear on one line, such as `module show gemc/5.14`, `module show coatjava/10.0.7`, and `module list`. Other copyable commands use multiline shell form.

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

Uniform input needs `channel`; `eh` also needs hadron and region. Physical input may supply event generator, tune, and $Q^2$ label, defaulting to `genie-gst`, `unknown`, and `unknown`. A detector variation or explicit GCARD is required.

## Submission record and failures

After `sbatch` returns `Submitted batch job NUMBER`, the coordinator prints the numeric job ID and atomically writes `RUN/reconhipo/slurm-submission-log.json`. It records the ID, exact command, resolved parameters, both requested software versions, executable paths, GEMC's data directory, loaded-module names, runtime Git state, and SHA-256 hashes of the GCARD, YAML, and worker payload.

If Slurm accepts an array but its response cannot be parsed, or writing the submission log fails afterward, inspect Slurm before retrying. Retrying blindly can create a duplicate array. An earlier accepted array is never cancelled automatically when a later sample fails.

Optional farm-output cleanup deletes only direct files in the exact reviewed directory and runs once per invocation. It requires both `--clear-farm-out true` and `--farm-out DIRECTORY`; omit both to preserve scheduler logs.

## Verify the finished array

The coordinator does not poll task states, retry failures, reconcile outputs, or inspect HIPO content. After the array finishes:

1. Review every array task in Slurm and its `.out` and `.err` files or in the [outstanding Jobs dashboard here](https://scicomp.jlab.org/scicomp/slurmJob/activeJob); other users can adjust those filters or use `squeue -u <username>`.
2. Compare the submitted task range with `RUN/mchipo/` and `RUN/reconhipo/`.
3. Open at least one reconstructed file:

   ```text
   hipo-utils -dump RUN/reconhipo/<file>.hipo
   ```

The file must open and display CLAS12 data banks. This is a smoke test only; one readable file does not establish that the remaining tasks succeeded or that the campaign is scientifically valid.

Jefferson Lab's [Slurm batch user guide](https://scicomp.jlab.org/docs/farm_slurm_batch) explains `sbatch` and farm batch operation. Useful commands are:

- Submit a GEMC job: `sbatch <submit-script>`
- Cancel one job: `scancel <job-id>`
- Cancel all of your jobs: `scancel --user=<username>`
- Check your jobs: `squeue -u <username>`; see also the [active-jobs dashboard](https://scicomp.jlab.org/scicomp/slurmJob/activeJob)
- Print the current priority for all pending production jobs: `squeue -t pd -p production -o "%.8Q %.10u/%10a" | uniq -c`

Cancel-all acts on every job owned by the named account, so use it only when that full scope is intended.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^run-field-scales]: COATJAVA's [`RUN::config` bank definition](https://github.com/JeffersonLab/coatjava/blob/development/etc/bankdefs/hipo4/header.json) defines the `torus` and `solenoid` fields as relative field settings.
