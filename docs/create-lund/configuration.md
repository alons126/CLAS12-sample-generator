# LUND-creation configuration reference

Both LUND applications accept `--key value` pairs and at most one `--config FILE`. A configuration file uses plain `key = value` lines. Blank lines and full-line `#` comments are accepted. Duplicate and unknown keys are rejected.

The parser trims whitespace and splits at the first `=`. It does not interpret inline comments, quotes, sections, or shell variables. Quotes and an inline `#` become literal parts of the value, so do not use shell syntax in profiles. Relative input and output paths start at the application's working directory, not at the profile's directory.

The application first reads defaults, then the profile, then the command line. Later values replace earlier ones. It then calculates `auto` values and checks the result:

```text
built-in defaults -> configuration file -> command line -> automatic values -> validation
```

The launcher does not choose a sample profile. Name one explicitly, or supply every required value. Launcher build settings belong to [`config/run.json`](../../config/run.json.md), not to a sample profile.

## Common settings

| Key | Default | Meaning |
| --- | --- | --- |
| `output` | required | Parent output directory; the program adds the final run name |
| `events` | required | Maximum number of events written, from 1 to 4294967295 |
| `events-per-file` | 25000 uniform; 10000 physical | Maximum events per file; also the minimum remaining input entries needed to start a later physical-output file |
| `beam-energy` | `5.98636` | Positive beam energy $E_{\mathrm{beam}}$ in $\mathrm{GeV}$ |
| `target` | `Ar40` | Target identity used to resolve `A`, `Z`, GEMC variation, and vertex geometry |
| `gemc-target-variation` | `auto` | Compatible target-variation override; changes the resolved geometry with it |
| `A`, `Z` | `auto` | Independent LUND-header overrides; require $1\le A\le 300$ and $0\le Z\le A$ |
| `seed` | `67890` | Uniform-kinematics seed; accepted but unused for physical input |
| `vertex-seed` | `12345` | Seed for vertex-position sampling |
| `prefix` | `auto` | LUND filename prefix using letters, numbers, `_`, `-`, and `.` |

The parser accepts the event-count range shown above, but serialized event IDs must also fit a signed 32-bit integer. Uniform IDs start at zero, so no run can write more than 2147483648 events. Physical IDs use GST entry indexes and must not exceed 2147483647 even when few events are retained. The writer fails without a completion manifest if an ID exceeds that limit. Submission additionally requires each file's count to fit its positive signed 32-bit event limit.

Seeds range from 0 to 4294967295. A nonzero ROOT `TRandom3` seed is repeatable when software, configuration, and draw order match. Seed 0 asks ROOT for automatic, nonrepeatable seeding; the manifest records the configured zero, not the internally chosen value.

`target`, `A`, and `Z` are related but not interchangeable configuration keys. The target and variation select spatial geometry. The `A` and `Z` values are written to the LUND header. Explicit `A`/`Z` overrides never silently change geometry.

## Target catalog

| Target | $A$ | $Z$ | Automatic GEMC variation | Vertex geometry |
| --- | ---: | ---: | --- | --- |
| `H1` | 1 | 1 | `rga_spring2019` | `liquid` |
| `D2` | 2 | 1 | `rgb_fall2019` | `liquid` |
| `He4` | 4 | 2 | `rgm_fall2021_He` | `liquid` |
| `Ar40` | 40 | 18 | `rgm_fall2021_Ar` | `Ar` |
| `C12`, $2.07052\,\mathrm{GeV}$ | 12 | 6 | `rgm_fall2021_C_S` | `1-foil-small` |
| `C12`, $4.02962\,\mathrm{GeV}$ | 12 | 6 | `rgm_fall2021_C_L` | `1-foil-large` |
| `C12`, $5.98636\,\mathrm{GeV}$ | 12 | 6 | `rgm_fall2021_Cx4` | `4-foil` |
| `Ca40`/`Ca48` | 40/48 | 20 | `rgm_fall2021_Ca` | `Ca` |
| `Sn120` | 120 | 50 | `rgm_fall2021_Sn_L` | `1-foil-large` |
| `Sn-nat` | 119 | 50 | `rgm_fall2021_Snx4` | `4-foil` |

C12 at another beam energy requires an explicit compatible variation. Run 15733 is the documented $4.02962\,\mathrm{GeV}$ exception and uses `rgm_fall2021_C_S`[^sportes-2026-rgm]. `Sn-nat` represents natural tin using the code's header value $A=119$, not a pure tin-119 isotope. The geometry rules come from the protected external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); see [external inputs](../concepts/external-inputs.md). A supported LUND target does not guarantee that a matching GCARD is included for every beam and software release; check detector resources separately before submission.

## Uniform settings

| Key | Default | Meaning |
| --- | --- | --- |
| `channel` | `1e` | `1e`, `electron-tester`, or `eh` |
| `hadron` | `proton` | `proton` ($p$), `neutron` ($n$), `pip` ($\pi^{+}$), or `pim` ($\pi^{-}$); used by `eh`. See the [electron-hadron label definitions](uniform.md#electron-hadron-labels). |
| `hadron-region` | `FD` | `FD` or `CD`; used by `eh` |
| `electron-theta-min/max` | `5`/`40` | Electron-only $\theta$ bounds, in degrees |
| `electron-momentum` | `auto` | `auto`, `uniform`, `mixed`, or `beam` |
| `electron-p-min/max` | `0.7`/beam | Electron momentum bounds in $\mathrm{GeV}/c$ |
| `hadron-theta-min/max` | `auto` | $\theta$ bounds resolved from hadron species and region |
| `hadron-momentum` | `auto` | `auto`, compatibility alias `sampled`, `uniform`, `mixed`, or neutron-only `fixed` |
| `hadron-p-min` | `auto` | Species/region minimum; maximum is always beam momentum |
| `hadron-p` | `1` | Fixed neutron momentum in $\mathrm{GeV}/c$ |
| `trigger-theta` | `25` | Trigger-electron $\theta$, in degrees |
| `trigger-phi-offset` | `auto` | $\Delta\phi=16^\circ$, $7^\circ$, or $5^\circ$ at the three standard beams; otherwise $0^\circ$ |

`auto` resolves electron momentum to `mixed` for 1e and `beam` for the other channels. It resolves charged-hadron momentum to `mixed` and neutron momentum to `uniform`. The [uniform guide](uniform.md) owns the production ranges; the [sampling model](../concepts/sampling-models.md) owns the mathematical definitions.

## Physical settings

| Key | Default | Meaning |
| --- | --- | --- |
| `input` | required | GENIE GST [ROOT](https://github.com/root-project/root) file, quoted local pattern, or ROOT-supported remote address |
| `event-generator` | `genie-gst` | Generator/format adapter; this is the only implemented value |
| `event-generator-version` | `unknown` | Generator version recorded in the log and optionally included in the filename prefix |
| `tune` | `auto` | Read `TUNE` from [GENIE production metadata](physical.md#automatic-genie-tune-lookup) saved with GENIE's `--store-comitinfo`; otherwise record `unknown` |
| `q2-cut` | beam-based | Record the minimum-$Q^2$ cut used to generate the input; conversion neither applies nor checks it |
| `output-layout` | `nested` | `nested` or `metadata` |

Automatic minimum-$Q^2$ cut labels are `Q2-0.02`, `Q2-0.25`, and `Q2-0.40` for $2.07052\,\mathrm{GeV}$, $4.02962\,\mathrm{GeV}$, and $5.98636\,\mathrm{GeV}$. Selecting or accepting one of these labels asserts that the supplied truth-level sample was generated with that minimum cut. The code assumes the assertion is correct: it does not calculate $Q^2$, inspect the input distribution, or remove events. An incorrect label therefore records incorrect provenance. The [physical-conversion guide](physical.md#why-upstream-samples-use-q2-cuts) explains the cuts' relation to the electron cross section and CLAS12 angular acceptance. Other energies resolve to `none`. Accepted underscore spellings normalize to the hyphenated form.

The nested directory is `OUTPUT/<target>/<event-generator>__<tune>/<q2-cut>__<beam-label>`. The metadata directory is `OUTPUT/<gemc-target-variation>__<event-generator>-<event-generator-version>__<tune>__<q2-cut>__<beam-label>`. Unsafe characters in each directory-name component are replaced; the manifest also keeps the original setting values. GEMC and COATJAVA versions are selected during simulation submission and are not part of LUND creation.

## Output replacement

The application calculates and checks all settings before the writer changes output. The writer turns the run path into a complete absolute path and rejects unsafe deletion targets: the filesystem root, home directory, current directory, or a directory containing the source checkout. If the exact run directory exists, it warns, deletes that directory and everything inside it, and recreates it. Rerunning with the same run path therefore replaces both partial and successful earlier output.

Different settings can resolve to the same directory. Uniform run names include only channel and beam; changing target, seeds, counts, or `prefix` does not make a new run directory. The physical nested layout omits generator version and detector variation. Use a different `output` parent when preserving distinct studies; changing only `prefix` does not protect earlier output. These writer checks are separate from the launcher's earlier disposable-checkout refresh.

[^sportes-2026-rgm]: Alon Sportes, *Technical Note: Implementation of New RG-M Targets in GEMC*, CLAS12 Note 2026-001, Jefferson Lab, CLAS12, February 2026. [Note PDF](https://misportal.jlab.org/mis/physics/clas12/viewFile.cfm/2026-001.pdf?documentId=185)
