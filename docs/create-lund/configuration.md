# Configuration reference

Sample settings use UTF-8 text with one `key = value` per line. Blank lines and lines beginning with `#` are ignored. There are no sections, inline comments, quoting rules or environment-variable expansion. Values may contain spaces. Unknown/duplicate keys are rejected. Command-line `--key value` settings override the file regardless of where `--config` appears.

The two C++ LUND applications pass these settings through the shared `RunConfig` layer. It installs common and source-specific defaults, applies the optional profile and CLI overrides, resolves every `auto` value, validates the complete result, and constructs normalized paths before event processing starts. Uniform-only keys are unavailable to physical conversion and physical-only keys are unavailable to uniform generation.

Target resolution has one deliberate order. `rgm-target` first selects the catalog default geometry, A, Z, and GEMC target variation. Explicit `target`, `A`, `Z`, or `gemc-target-variation` values are then applied as independent overrides. Normal profiles therefore specify only `rgm-target`; derived values still appear in the completed manifest. Compatibility profiles retain an override only when they intentionally differ from the catalog, such as the archived uniform A=1/Z=1 header.

`RunConfig` is configuration policy, not workflow execution. It does not generate particles, read GST event records, advance either random stream, create or remove output directories, write LUND/ROOT files, or submit GEMC jobs. Once parsing succeeds, the selected generator or converter consumes its checked values and `LundWriter` copies the complete resolved map into `lundfiles/lund-creation-monitoring/lund-creation-log.json`.

The launcher does not select a sample profile implicitly. Pass a profile from `config/samples/uniform-lund-creation/` or `config/samples/physical-lund-creation/` in each `create-lund` command, or explicitly provide every required sample option. See the [sample-profile inventory](../../config/samples/README.md) for profile purposes and option groups. `config/run.json` contains build defaults only.

Relative paths are interpreted from the caller's working directory. The output path and local GENIE input pattern are resolved to absolute paths in the manifest. ROOT-supported remote URLs remain unchanged.

## Common sample settings

| Key | Default | Meaning |
| --- | --- | --- |
| `output` | Required | Output parent/run directory; an existing resolved run directory is replaced after a warning |
| `beam-energy` | `5.98636` | Positive beam energy in GeV |
| `rgm-target` | `Ar40` | Catalog identity that first supplies geometry, A/Z, and GEMC-variation defaults |
| `target` | `auto` | Optional override of the external [`targets.h`](../../src/workflows/lund-creation/external/targets.h) geometry selected by `rgm-target` |
| `A`, `Z` | `auto` | Optional LUND-metadata overrides applied after target defaults; require 1≤A≤300, 0≤Z≤A |
| `gemc-target-variation` | `auto` | Optional GCARD target-variation override applied after the catalog default |
| `events` | Required | Total number of accepted events to write |
| `events-per-file` | `25000` uniform / `10000` physical | Positive split threshold; before each physical follow-up file it also sets the minimum remaining-input block aligned with submission `JOB_NEVENTS` |
| `seed` | `67890` | Uniform kinematic RNG seed; zero requests ROOT automatic, nonrepeatable seeding; unused in physical conversion |
| `vertex-seed` | `12345` | Vertex RNG seed; zero requests ROOT automatic, nonrepeatable seeding |
| `prefix` | `auto` | LUND filename label; letters, digits, `_`, `-`, `.` |
| `input` | Required for physical input | Event-generator input filename or quoted glob |
| `event-generator` | `genie-gst` | Physical adapter name; generator and input format are explicit |
| `event-generator-version` | `unknown` | Explicit provenance and physical-run naming component |
| `tune`, `q2-cut` | `unknown` / energy-based | Generator provenance and naming components |
| `gemc-version` | `unknown` | Planned detector-simulation version and naming component |

Counts and the split threshold must be integers from 1 through 4294967295. Seeds may range from 0 through 4294967295. A nonzero seed is reproducible; `TRandom3(0)` asks ROOT to choose an automatic seed, so a manifest containing zero cannot reproduce the generated sequence. Production Ar defaults resolve to A=40/Z=18.

## Uniform settings

| Key | Default | Meaning |
| --- | --- | --- |
| `channel` | `1e` | `1e` for one sampled electron, `electron-tester` for the beam-momentum angular scan, or `eh` for trigger electron plus selected hadron |
| `hadron` | `proton` | `proton`, `neutron`, `pip`, or `pim`; used by `eh` |
| `hadron-region` | `FD` | `FD` or `CD`; resolves the hadron angular and momentum thresholds |
| `electron-theta-min/max` | `5` / `40` | Electron-only/tester theta range, degrees; the production 2.07052 GeV outbending 1e profile explicitly uses `2` / `40` |
| `electron-momentum` | `auto` | `mixed` for 1e, `beam` for eh; explicit `uniform`, `mixed`, or `beam` |
| `electron-p-min/max` | `0.7` / beam | 1e momentum bounds in GeV/c |
| `hadron-theta-min/max` | `auto` / `auto` | FD: p/pions 5–45°, n 5–35°; CD: nucleons 35–145°, pions 35–140° |
| `hadron-momentum` | `auto` | charged hadrons → `mixed`; neutron → `uniform`; neutron-only `fixed` is optional |
| `hadron-p` | `1` | Fixed neutron momentum in GeV/c |
| `hadron-p-min` | species/region | p: 0.3 FD, 0.2 CD; pip/pim: 0.2 FD, 0.1 CD; n: 0; upper bound is always beam energy |
| `trigger-theta` | `25` | Trigger electron theta in eh, degrees |
| `trigger-phi-offset` | energy-based | Offset from sector closest to opposite hadron direction |

Every event samples exactly one vertex from the selected target geometry and shares it among its particles; fixed coordinates are not a maintained mode. The trigger-electron opposite-sector rule is retained for CD samples even though it is not geometrically obligatory. The electron tester always scans theta 5–40° and all phi at beam momentum; its scan motivated the 25° production trigger setting. Mixed sampling requires a positive lower bound. Resolved labels are `1e`, `electron-tester`, `epFD`, `enFD`, `epipFD`, `epimFD`, `epCD`, `enCD`, `epipCD`, and `epimCD`; they control output directory and automatic prefix names. Resolved values are recorded in the manifest.

## Target geometry

The authoritative source is the replaceable [`src/workflows/lund-creation/external/targets.h`](../../src/workflows/lund-creation/external/targets.h); see [external inputs](../concepts/external-inputs.md) for provenance and replacement instructions. The table describes the checked-in snapshot and must be reviewed after updates.

All positions below are in cm in the imported GEMC coordinate convention. Target-sampled x and y are independent Gaussians with mean 0 and sigma 0.04 cm.

| Name | z prescription |
| --- | --- |
| `Ar` | Uniform −5.75 to −5.25 |
| `liquid` | Uniform −5.5 to −0.5 |
| `4-foil` | Equal choice of −4.875, −3.625, −2.375, −1.125 |
| `1-foil` | −0.5 |
| `1-foil-small` | −2.1 |
| `1-foil-large` | −2.32 |
| `Ca` | −3.0 |

Unknown geometries fail instead of writing sentinel coordinates. Geometry does not automatically select a matching detector card.

## RG-M target catalog

The maintained catalog centralizes the same kind of selection that the legacy submission script performed with target/beam conditionals. Each identity supplies the external geometry key, nuclear metadata, and official default GEMC variation. Natural tin uses representative LUND `A=119`; choose an explicit isotope override when the event sample requires one. Empty-target configurations are not LUND vertex sources and therefore are not catalog entries.

For RG-M production, `Ar40` is valid at the nominal 2, 4, and 6 GeV beam energies. Use `C12-small`, the small 4 mm one-foil target, at 2 GeV; use `C12-large`, the large 6 mm one-foil target, at 4 GeV; and use `C12-four-foil` at 6 GeV. Run 15733 is the exception: it used the small 4 mm one-foil C12 target at 4 GeV. The target note documents the foil sizes, beam use, and corresponding GEMC variations, while the RG-M analysis note records the target cells and beam energies[^sportes-2026-rgm][^rgm-analysis-note]. The catalog does not block an explicit nonproduction pairing, but such a choice must be treated as a controlled study rather than an RG-M production setting.

| Identifier | A | Z | Vertex geometry | GEMC target variation | RG-M beam use |
| --- | --- | --- | --- | --- | --- |
| `H1` | 1 | 1 | `liquid` | `rga_spring2019` | See campaign metadata |
| `D2` | 2 | 1 | `liquid` | `rgb_fall2019` | See campaign metadata |
| `He4` | 4 | 2 | `liquid` | `rgm_fall2021_He` | See campaign metadata |
| `C12-four-foil` | 12 | 6 | `4-foil` | `rgm_fall2021_Cx4` | 6 GeV |
| `Sn-nat-four-foil` | 119 | 50 | `4-foil` | `rgm_fall2021_Snx4` | See campaign metadata |
| `Ca40` | 40 | 20 | `Ca` | `rgm_fall2021_Ca` | See campaign metadata |
| `Ca48` | 48 | 20 | `Ca` | `rgm_fall2021_Ca` | See campaign metadata |
| `C12-small` | 12 | 6 | `1-foil-small` | `rgm_fall2021_C_S` | 2 GeV; also 4 GeV for run 15733 |
| `C12-large` | 12 | 6 | `1-foil-large` | `rgm_fall2021_C_L` | 4 GeV except run 15733 |
| `Ar40` | 40 | 18 | `Ar` | `rgm_fall2021_Ar` | 2, 4, and 6 GeV |
| `Sn120-large` | 120 | 50 | `1-foil-large` | `rgm_fall2021_Sn_L` | See campaign metadata |

## Manifest

Schema version 1 contains `workflow`, project `version`, the short configure-time Git `revision`, a full `git` object (repository, branch, commit, status, tag, tracking state, and GitHub tree link), `root_version`, the compiled header hash `targets_sha256`, resolved string-valued `config`, `scanned_events`, `written_events`, and `files` objects with relative `path` and integer `events`.

It is a completion record and pipeline input, not a content-addressed archive: retain the source checkout and original GST files for full provenance. Rounded masses and all LUND fields/precision are defined in the [data contract](../concepts/lund-data-contract.md). ROOT monitoring files may contain timestamps; reproducibility checks compare LUND output.

## Detector and submission settings

Supply `--lund-dir RUN/lundfiles` to infer settings from its manifest, with optional `--config` and CLI overrides. GEMC falls back to 5.14. It selects GCARD/YAML resources explicitly and uses the external payload’s scheduler defaults. There are no site JSON files. See the [submission guide](../submit-simulation/guide.md).

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)

[^rgm-analysis-note]: Andrew Denniston, Justin Estee, Julian Kahlbow, and Erin Marshall Seroka, *RG-M Analysis Note: 6 GeV Electron Proton Selection and Particle ID*, unpublished draft, Massachusetts Institute of Technology and The George Washington University, February 2026.
