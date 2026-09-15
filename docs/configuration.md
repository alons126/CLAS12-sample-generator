# Configuration reference

Sample settings use UTF-8 text with one `key = value` per line. Blank lines and lines beginning with `#` are ignored. There are no sections, inline comments, quoting rules or environment-variable expansion. Values may contain spaces. Unknown/duplicate keys are rejected. Command-line `--key value` settings override the file regardless of where `--config` appears.

Relative paths are interpreted from the caller's working directory. The output path and local GENIE input pattern are resolved to absolute paths in the manifest. ROOT-supported remote URLs remain unchanged.

## Common sample settings

| Key | Default | Meaning |
| --- | --- | --- |
| `output` | Required | New run directory; existing directories are rejected |
| `beam-energy` | `5.98636` | Positive beam energy in GeV |
| `target` | `Ar` | Vertex geometry name, independent of A/Z |
| `A`, `Z` | `1`, `1` | LUND nuclear metadata; require 1≤A≤300, 0≤Z≤A |
| `events` | Required | Total number of accepted events to write; files split automatically at 10,000 events |
| `seed` | `67890` | Uniform kinematic RNG seed; unused in GENIE conversion |
| `vertex-seed` | `12345` | Vertex RNG seed |
| `prefix` | `Uniform_sample` / `GENIE_sample` | Filename label; letters, digits, `_`, `-`, `.` |
| `lund-format` | `legacy` | Legacy text precision/numbering, or `precise` |
| `mass-convention` | `legacy` | Restored constants, or `standard` pion constants |
| `render-plots` | `false` | `true` additionally writes diagnostic PDF/PNG files |
| `input` | Required for GENIE | ROOT GST input filename or quoted glob |

Counts and seeds must be integers from 1 through 4294967295. The explicit Ar example files set A=40/Z=18; bare CLI defaults retain the imported uniform header convention A=Z=1. Always select the intended metadata for production.

## Uniform settings

| Key | Default | Meaning |
| --- | --- | --- |
| `channel` | `1e` | `1e`, `ep`, `en` |
| `electron-theta-min/max` | `5` / `40` | `1e` theta range, degrees |
| `electron-momentum` | `uniform` | `uniform` in [0, beam), or `beam` |
| `nucleon-theta-min/max` | `5` / `auto` | Theta range; auto maximum 45° for ep, 35° for en |
| `nucleon-momentum` | `fixed` | `fixed`, `sampled`, `uniform`, `mixed`; sampled resolves by channel |
| `nucleon-angle` | `auto` | `theta` or `isotropic`; auto is isotropic for non-fixed en, theta otherwise |
| `nucleon-p` | `1` | Fixed momentum, GeV |
| `nucleon-p-min/max` | `0.3` / `auto` | Uniform bounds; auto maximum is beam energy |
| `trigger-theta` | `25` | Trigger electron theta, degrees |
| `trigger-phi-offset` | `auto` | Offset in degrees; energy-based defaults in uniform guide |

Theta limits require 0≤min<max≤180. Fixed momentum must be positive; uniform momentum bounds require 0≤min<max. All numeric values must be finite. `mixed` requires channel ep and a strictly positive minimum momentum; it alternates uniform-p and uniform-1/p draws. `sampled` resolves to `uniform` for en and `mixed` for ep. Resolved `auto` values are written to the manifest. Settings are validated even when inactive for the selected channel.

## Target geometry

The authoritative source is the replaceable [`src/common/external/targets.h`](../src/common/external/targets.h); see [external inputs](external-inputs.md) for provenance and replacement instructions. The table describes the checked-in snapshot and must be reviewed after updates.

All positions below are in cm in the imported GEMC coordinate convention. Except `point`, x and y are independent Gaussians with mean 0 and sigma 0.04 cm.

| Name | z prescription |
| --- | --- |
| `Ar` | Uniform −5.75 to −5.25 |
| `liquid` | Uniform −5.5 to −0.5 |
| `4-foil` | Equal choice of −4.875, −3.625, −2.375, −1.125 |
| `1-foil` | −0.5 |
| `1-foil-small` | −2.1 |
| `1-foil-large` | −2.32 |
| `Ca` | −3.0 |
| `point` | x=y=z=0 |

Unknown geometries fail instead of writing sentinel coordinates. Geometry does not automatically select a matching detector card.

## Manifest

Schema version 1 contains `workflow`, project `version`, configure-time Git `revision` (including a dirty marker when applicable), `root_version`, the compiled header hash `targets_sha256`, resolved string-valued `config`, `scanned_events`, `written_events`, and `files` objects with relative `path` and integer `events`.

It is a completion record and pipeline input, not a content-addressed archive: retain the source checkout and original GST files for full provenance. Legacy masses and all LUND fields/precision are defined in the [data contract](data-contracts.md). ROOT monitoring files may contain timestamps; reproducibility checks compare LUND output.

## Detector and site settings

Gcards and reconstruction YAML live under `config/detector/Generation_files_{2,4,6}GeV/{devGEMC5.12,5.14}/`. Their contents are retained from the imported repository. Select actual files with the runner's `--gcard` and `--reconstruction` options; field scales are explicit. Gcards originate from JeffersonLab/clas12-config; [provenance and required field settings](external-inputs.md) specify outbending at 2 GeV and inbending at 4/6 GeV.

Site JSON accepts `gemc` and `recon` executable names/paths. Optional `slurm` requires `account`, `partition`, `time`, and `mem` strings and accepts `output`/`error` log-path strings. The JLab file is an editable example of the imported resources, not a claim that those allocations suit every run. Load the necessary environment before executing/submitting.
