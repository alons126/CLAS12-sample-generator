# Terminology

For example, create a uniform 1e sample, submit its LUND files to ifarm, and compare the reconstructed electrons with the written electrons. These actions produce different records and check different things. Use the terms below to distinguish them throughout the manual.

## Workflows and settings

| Term | Meaning |
| --- | --- |
| LUND-creation workflow | The `create-lund` workflow. It writes particles before detector simulation to LUND files. |
| Uniform LUND creator | The `--source uniform` path. It samples configured momenta and angles for deliberately unphysical acceptance studies. |
| Physical LUND converter | The `--source physical` path. It copies supported particle content from existing event-generator output; it does not run the event generator. |
| Physical-input adapter | The format-specific reader used by the physical LUND converter, currently `genie-gst`. It translates input events without duplicating the common LUND-writing code. |
| Simulation-submission workflow | The `submit` workflow. It submits existing LUND files to ifarm Slurm jobs that run GEMC detector simulation and COATJAVA reconstruction. |
| Sample profile | A `key = value` configuration file describing a LUND sample. Launcher settings and submission configuration are separate inputs. |
| Run directory | The complete output directory for one resolved sample, written as `RUN` in examples. It contains `lundfiles/`, `mchipo/`, and `reconhipo/`. |
| Beam label | A name such as `5986MeV` used in sample directories and filenames. It does not replace the precise `beam-energy` value. |
| Detector lookup group | A name such as `6GeV` used to locate GCARD and reconstruction YAML files. It is distinct from the beam label. |
| Vertex position | The event's position in the target, with vertex coordinates Vx, Vy, and Vz in centimeters. LUND creation samples one vertex per written event and assigns it to every particle in that event. |

Target identity (`target`, such as `Ar40`), vertex geometry (`target-geometry`, such as `Ar`), detector target variation (`gemc-target-variation`), and nuclear header metadata (`A` and `Z`) describe distinct settings. See the [vertex-position contract](sampling-models.md#vertex-positions) before changing them.

## Records and validation

The LUND creation log and Slurm submission log are JSON files written automatically by the code. The campaign record is assembled separately by the person or group responsible for the study.

| Term | Meaning |
| --- | --- |
| Provenance | The settings, input details, and software versions that describe how output was produced. It is information stored in records, not a separate completion check. |
| LUND creation log | The automatically written `lund-creation-log.json` file. It lists resolved LUND settings, files, and event counts and marks completion of the program's checked creation stages. It does not establish detector-level validity or independently verify uniform PDF/PNG exports. |
| Slurm submission log | The automatically written `slurm-submission-log.json` file. It describes a submission accepted by Slurm, including its job ID and resolved settings. It does not establish that tasks finished successfully. |
| Simulation campaign | A related set of simulation runs for a physics study. A data campaign instead groups detector data-taking runs; a GENIE production campaign groups upstream event-generator runs. |
| Campaign record | The report and supporting files assembled by the person or group responsible for a campaign. It combines settings, automatic records, job results, comparison plots, and conclusions. The code does not generate it or require a particular file format. |
| Software checks | Checks that code follows its interface and data contracts, such as particle order, event counts, and file splitting. Passing them does not establish that detector output suits an analysis. |
| Detector-level validation | Comparisons of simulated and reconstructed results under specified detector and reconstruction conditions, with enough statistics to assess their suitability for an analysis. |
| Production-tested | The reported status of uniform modes used and checked in detector-simulation production. It is not certification of every target, beam energy, release, or override; the supporting campaign record must be reviewed. |

The [LUND data contract](lund-data-contract.md) defines the automatic records. The [scientific validation guide](../development/validation.md#campaign-record) defines the evidence to retain in a campaign record. Preserve exact filenames and configuration keys even when their spelling differs from these descriptive terms.
