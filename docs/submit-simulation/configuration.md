# Submission configuration reference

Use this page when selecting submission settings, software releases, detector resources, or manually described LUND input. The [submission guide](guide.md) gives the operating steps.

## How inputs are resolved

The command line has the highest priority. If a setting is absent there, submission looks in the optional config, then the completion manifest, then defaults:

```text
command line -> optional submission config -> completion manifest -> defaults
```

Relative command-line paths start at the repository root. Relative paths inside a submission config start at that file's directory. Use plain `key = value` lines, blank lines, and full-line comments. Unknown, duplicate, and empty keys cause an error.

Paths forwarded to the external worker may contain only letters, digits, `/`, `_`, `-`, and `.`. This restriction is checked before submission because the worker passes those paths to external commands.

The manifest supplies the exact LUND inventory and per-file event counts. The array size defaults to the number of selected files, and the shared `JOB_NEVENTS` limit defaults to the largest event count among them. `--num-jobs N` selects the first N files. `--events-per-job N` replaces the shared event limit; it does not rewrite or recount the files. A limit smaller than a file's event count leaves later events unprocessed; a larger limit does not add events to a shorter file.

If the manifest says the sample used one beam energy or target, an option claiming another is rejected. You can still change the detector setup: GEMC and COATJAVA versions, a compatible target variation, GCARD, YAML, torus scale, custom clas12Tags directory, and Slurm job name. Select both software versions during submission; LUND creation does not select them.

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
| `--clear-farm-out true\|false` | With execution, optionally delete files directly inside one exact farm-output directory; default false |
| `--farm-out DIRECTORY` | Required exact cleanup target when farm-output cleanup is true |
| `--execute` | Replace simulation output and call `sbatch` |

The following options describe LUND input that has no manifest. With a manifest, conflicting truth values are rejected rather than used to relabel the run.

| Option | Meaning |
| --- | --- |
| `--source uniform\|physical` | Required input kind |
| `--beam-energy GeV` | Required truth beam energy |
| `--target ID` | Required truth target identity |
| `--prefix NAME` | Required filename prefix before `_INDEX.txt` |
| `--channel NAME` | Uniform `electron-tester`, `1e`, `eh`, or a [complete electron-hadron label](../create-lund/uniform.md#electron-hadron-labels) such as `epFD` |
| `--hadron NAME`, `--hadron-region FD\|CD` | Particle and region required when `channel=eh` |
| `--event-generator NAME` | Physical adapter label; default `genie-gst` |
| `--tune NAME` | Physical tune/model label; default `unknown` |
| `--q2-cut NAME` | Physical upstream-selection label; default `unknown`, with no cut applied during submission |

Use [`source run.csh --workflow submit --help`](../../run.csh) for the live interface. [`config/submission.conf`](../../config/submission.conf) is an example, not an automatically loaded site profile.

## Detector defaults

GEMC defaults to 5.14 because that release contains the RG-M argon target and corrected one-foil carbon target used by this code[^sportes-2026-rgm]. CLAS12 GEMC detector data and available version directories are maintained in [`gemc/clas12Tags`](https://github.com/gemc/clas12Tags). GCARD defaults are selected from the manifest's target variation, beam energy, and submission-time GEMC version under [`config/detector/`](../../config/detector/), using the pattern `config/detector/GEMC_GCARDs_<beam-group>/<gemc-version>/`.

COATJAVA defaults to 10.0.7. Its version independently selects the reconstruction software and the YAML directory under [`config/detector/`](../../config/detector/), using the pattern `config/detector/COATJAVA_YAML_configs_<beam-group>/<coatjava-version>/`. Only 10.0.7 YAML snapshots are currently checked in. Another release needs a reviewed YAML supplied with `--yaml` if its default file is absent. Explicit `--gcard` and `--yaml` paths override resource lookup, not software selection. Check that the selected files are compatible with the requested releases.

For $E_{\mathrm{beam}}=2.07052\,\mathrm{GeV}$, the torus default is $+0.5$. For $E_{\mathrm{beam}}=4.02962\,\mathrm{GeV}$ and $5.98636\,\mathrm{GeV}$, it is $-1.0$. The worker always applies solenoid scale $-1.0$. Beam labels outside the three supported lookup groups require explicit `--gcard`, `--yaml`, and `--torus` values.

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

The coordinator distinguishes the campaign sample label (`2070MeV`, `4029MeV`, or `5986MeV`) from the detector-resource group (`2GeV`, `4GeV`, or `6GeV`). The three standard beam energies use these fixed labels; other energies are rounded to the nearest MeV for naming. Detector defaults are looked up using the resolved MeV label. These are naming and lookup values, not alternate beam energies.

## Software selection and version reports

The submission program starts a separate shell process to select software; it does not change your login shell's settings. It unloads and loads GEMC and uses `module switch coatjava/<coatjava-version>` for COATJAVA. For each version, it prints the switching notice and the module system's messages, then a blank line, a configuration heading, and `module show` output. The heading identifies `gemc/<gemc-version>` or `coatjava/<coatjava-version>`. Preview and execution use this same order. After both selections, `module list` shows all modules that will be passed to the jobs, including dependencies.

`LOADEDMODULES` lists loaded module names. The submission program uses that list to check and print the COATJAVA release. It also finds `recon-util` through `PATH`, the list of directories searched for commands. It does not assume a particular COATJAVA installation directory. For GEMC, it checks the versioned installation, executable path, and selected data directory. Missing or conflicting releases, failed module commands, or missing programs stop submission before simulation-output directories are deleted.

Passing these checks means the requested software was selected, required files exist, and arguments can be passed safely to the worker. It does not prove that the GCARD geometry, YAML reconstruction settings, and magnetic fields match your data. Review those together before a production run.

When `--clas12tags-dir` is absent, GEMC uses the shared versioned clas12Tags directory. A custom clas12Tags checkout replaces the data directory but not the selected GEMC executable checks. The verified child environment is exported to Slurm; the interactive login shell stays unchanged. Default job names include both `GEMC<gemc-version>` and `COATJAVA<coatjava-version>` for uniform and physical samples; `--job-name` overrides the name without changing either release.

Before GEMC, each array task prints the requested software versions, inherited module selections, and paths of `gemc` and `recon-util` to its `.out` file. This reports the job environment without requiring the `module` shell function or querying the binaries' versions. See the [worker reference](worker-reference.md#environment-interface).

Module commands in the submission report appear on one line, such as `module show gemc/5.14`, `module show coatjava/10.0.7`, and `module list`. Other copyable commands use multiline shell form.

## LUND input without a manifest

You can submit independently prepared LUND files that have no completion manifest. In that case, supply the source, beam energy, target, filename prefix, event limit, and other required sample settings yourself. Number files consecutively from `PREFIX_1.txt` through `PREFIX_N.txt`, without gaps. The submission program does not read every file to count events or guess missing indexes.

```tcsh
source run.csh \
    --workflow submit \
    --lund-dir /path/to/manual-run/lundfiles \
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

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^run-field-scales]: COATJAVA's [`RUN::config` bank definition](https://github.com/JeffersonLab/coatjava/blob/development/etc/bankdefs/hipo4/header.json) defines the `torus` and `solenoid` fields as relative field settings.
