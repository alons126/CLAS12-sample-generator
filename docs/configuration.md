# Configuration reference

Sample settings use UTF-8 text with one `key = value` per line. Blank lines and lines beginning with `#` are ignored. There are no sections, inline comments, quoting rules or environment-variable expansion. Values may contain spaces. Unknown/duplicate keys are rejected. Command-line `--key value` settings override the file regardless of where `--config` appears.

The two C++ LUND applications pass these settings through the shared `RunConfig` layer. It installs common and source-specific defaults, applies the optional profile and CLI overrides, resolves every `auto` value, validates the complete result, and constructs normalized paths before event processing starts. Uniform-only keys are unavailable to physical conversion and physical-only keys are unavailable to uniform generation.

`RunConfig` is configuration policy, not workflow execution. It does not generate particles, read GST event records, advance either random stream, create or remove output directories, write LUND/ROOT files, or submit GEMC jobs. Once parsing succeeds, the selected generator or converter consumes its checked values and `LundWriter` copies the complete resolved map into `manifest.json`.

The launcher does not select a sample profile implicitly. Pass `--config config/samples/NAME.conf` in each `create-lund` command, or explicitly provide every required sample option. See the [sample-profile inventory](../config/samples/README.md) for profile purposes and option groups. `config/run.json` contains build/test defaults only.

Relative paths are interpreted from the caller's working directory. The output path and local GENIE input pattern are resolved to absolute paths in the manifest. ROOT-supported remote URLs remain unchanged.

## Common sample settings

| Key | Default | Meaning |
| --- | --- | --- |
| `output` | Required | Output parent/run directory; an existing resolved run directory is replaced after a warning |
| `beam-energy` | `5.98636` | Positive beam energy in GeV |
| `rgm-target` | `Ar40` | Catalog identity resolving nucleus, vertex geometry and GEMC variation |
| `target` | `auto` | Protected `targets.h` geometry key; normally resolved from `rgm-target` |
| `A`, `Z` | `auto` | LUND nuclear metadata resolved from `rgm-target`; require 1≤A≤300, 0≤Z≤A |
| `gemc-target-variation` | `auto` | GCARD target variation resolved from `rgm-target` |
| `vertex-mode` | `target` | Sample the selected target or use explicit `fixed` coordinates |
| `vertex-x/y/z` | `0` / `0` / `-3` | Fixed-vertex coordinates in cm |
| `events` | Required | Total number of accepted events to write |
| `events-per-file` | `25000` uniform / `10000` physical | Positive split threshold recorded in the manifest; use `10000` for the protected GEMC submission payload |
| `seed` | `67890` | Uniform kinematic RNG seed; unused in GENIE conversion |
| `vertex-seed` | `12345` | Vertex RNG seed |
| `prefix` | `auto` | LUND filename label; letters, digits, `_`, `-`, `.` |
| `lund-format` | `legacy` | Legacy text precision/numbering, or `precise` |
| `mass-convention` | `legacy` | Restored constants, or `standard` pion constants |
| `render-plots` | `true` uniform / `false` physical | Render legacy-named uniform PDF/PNG artifacts or optional physical plots |
| `input` | Required for physical input | Event-generator input filename or quoted glob |
| `event-generator` | `genie` | Physical adapter name; GENIE is currently implemented |
| `event-generator-version` | `unknown` | Explicit provenance and physical-run naming component |
| `tune`, `q2-cut` | `unknown` / energy-based | Generator provenance and naming components |
| `gemc-version` | `unknown` | Planned detector-simulation version and naming component |

Counts, the split threshold, and seeds must be integers from 1 through 4294967295. Production Ar defaults resolve to A=40/Z=18. Legacy parity tests explicitly request the archived A=1/Z=1 uniform headers.

## Uniform settings

| Key | Default | Meaning |
| --- | --- | --- |
| `channel` | `1e` | `1e`, `ep`, `en` |
| `electron-theta-min/max` | `5` / `40` | `1e` theta range, degrees |
| `electron-momentum` | `uniform` | `uniform` in [0, beam), or `beam` |
| `nucleon-theta-min/max` | `5` / `auto` | Theta range; auto maximum 45° for ep, 35° for en |
| `nucleon-momentum` | `fixed` | `fixed`, `sampled`, `uniform`, `mixed`; sampled resolves by channel |
| `nucleon-angle` | `auto` | `theta` or `isotropic`; auto is isotropic for non-fixed en, theta otherwise |
| `nucleon-p` | `1` | Fixed momentum, GeV/c |
| `nucleon-p-min/max` | `0.3` / `auto` | Momentum bounds in GeV/c; auto maximum uses the beam-energy value under c=1 |
| `trigger-theta` | `25` | Trigger electron theta, degrees |
| `trigger-phi-offset` | `auto` | Offset in degrees; energy-based defaults in uniform guide |

Theta limits require 0≤min<max≤180. Fixed momentum must be positive; uniform momentum bounds require 0≤min<max. All numeric values must be finite. `mixed` requires channel ep and a strictly positive minimum momentum; it alternates uniform-p and uniform-1/p draws. `sampled` resolves to `uniform` for en and `mixed` for ep. Resolved `auto` values are written to the manifest. Settings are validated even when inactive for the selected channel.

## Target geometry

The authoritative source is the replaceable [`src/common/external/targets.h`](../src/common/external/targets.h); see [external inputs](external-inputs.md) for provenance and replacement instructions. The table describes the checked-in snapshot and must be reviewed after updates.

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
| `point` | compatibility alias for fixed x=y=0, z=−3 |

Unknown geometries fail instead of writing sentinel coordinates. Geometry does not automatically select a matching detector card.

## RG-M target catalog

The maintained catalog maps target identity onto the protected geometry source and official GEMC variation. Natural tin uses representative LUND `A=119`; choose an explicit isotope override when the event sample requires one. Empty-target configurations are not LUND vertex sources and therefore are not catalog entries.

| Identifier | A/Z | Vertex geometry | GEMC target variation |
| --- | --- | --- | --- |
| `H1` | 1/1 | `liquid` | `rga_spring2019` |
| `D2` | 2/1 | `liquid` | `rgb_fall2019` |
| `He4` | 4/2 | `liquid` | `rgm_fall2021_He` |
| `C12-four-foil` | 12/6 | `4-foil` | `rgm_fall2021_Cx4` |
| `Sn-nat-four-foil` | 119/50 | `4-foil` | `rgm_fall2021_Snx4` |
| `Ca40`, `Ca48` | 40/20, 48/20 | `Ca` | `rgm_fall2021_Ca` |
| `C12-small` | 12/6 | `1-foil-small` | `rgm_fall2021_C_S` |
| `C12-large` | 12/6 | `1-foil-large` | `rgm_fall2021_C_L` |
| `Ar40` | 40/18 | `Ar` | `rgm_fall2021_Ar` |
| `Sn120-large` | 120/50 | `1-foil-large` | `rgm_fall2021_Sn_L` |
| `C12-legacy`, `Sn120-legacy` | 12/6, 120/50 | `1-foil` | Removed legacy variations retained for reproduction |

## Manifest

Schema version 1 contains `workflow`, project `version`, configure-time Git `revision` (including a dirty marker when applicable), `root_version`, the compiled header hash `targets_sha256`, resolved string-valued `config`, `scanned_events`, `written_events`, and `files` objects with relative `path` and integer `events`.

It is a completion record and pipeline input, not a content-addressed archive: retain the source checkout and original GST files for full provenance. Legacy masses and all LUND fields/precision are defined in the [data contract](data-contracts.md). ROOT monitoring files may contain timestamps; reproducibility checks compare LUND output.

## Detector and site settings

Gcards and reconstruction YAML live under `config/detector/Generation_files_{2,4,6}GeV/{devGEMC5.12,5.14}/`. Their contents are retained from the imported repository. Select actual files with the runner's `--gcard` and `--reconstruction` options; field scales are explicit. Gcards originate from JeffersonLab/clas12-config; [provenance and required field settings](external-inputs.md) specify outbending at 2 GeV and inbending at 4/6 GeV.

Site JSON accepts `gemc` and `recon` executable names/paths. Optional `slurm` requires `account`, `partition`, `time`, and `mem` strings and accepts `output`/`error` log-path strings. The JLab file is an editable example of the imported resources, not a claim that those allocations suit every run. Load the necessary environment before executing/submitting.
