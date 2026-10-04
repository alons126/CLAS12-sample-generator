# Physical LUND conversion

The physical LUND converter copies supported particles from existing event-generator output into LUND files. Truth-level particles are the simulated particles before detector simulation and reconstruction. The physical LUND converter does not run an event generator, randomly choose replacement momenta, compute cross sections, or apply detector-acceptance cuts. Its current input reader, called an adapter, reads [GENIE](https://github.com/GENIE-MC/Generator) GST (generator summary tree) data stored in [ROOT](https://github.com/root-project/root) files. Select it with `--event-generator genie-gst`.

## Convert GENIE GST input

Use [`run.csh`](../../run.csh) with the profile [`genie-gst.conf`](../../config/samples/physical-lund-creation/genie-gst.conf):

```tcsh
source run.csh \
    --workflow create-lund \
    --source physical \
    --config config/samples/physical-lund-creation/genie-gst.conf \
    --input '/path/to/gst*.root' \
    --events 100 \
    --output /path/to/physical-output
```

Quote patterns such as `'/path/to/gst*.root'` so your shell passes the pattern to ROOT instead of expanding it. Each input file must contain a ROOT tree named `gst`, a table with one entry per event. ROOT reads matching files in sequence as one chain.

Supply electron-scattering GST, not neutrino-scattering GST. The adapter always labels `pxl`, `pyl`, and `pzl` as a scattered electron and uses the configured beam energy and target metadata. It does not check those labels against incoming-lepton, beam-energy, or target branches. A matching tree schema alone does not establish that the input describes the intended campaign.

## Required GST fields

| Branches | ROOT types |
| --- | --- |
| `qel`, `mec`, `res`, `dis` | `Bool_t` |
| `resid`, `nf` | `Int_t` |
| `pxl`, `pyl`, `pzl` | `Double_t` |
| `pdgf[nf]` | `Int_t` array |
| `pxf[nf]`, `pyf[nf]`, `pzf[nf]` | `Double_t` arrays |

A branch is a named field in the ROOT tree. The adapter checks that required branches exist, have the expected types, and provide enough array elements before reading particles by index. Empty input, malformed records, read errors, or input with no supported events cause failure. No completion manifest is written in those cases.

## Selection and translation

The adapter keeps events carrying one of four GENIE interaction-mechanism flags[^electrons-for-neutrinos]:

- **QE, quasielastic scattering:** the lepton scatters from a bound nucleon without producing a pion at the primary interaction. The outgoing nucleon can interact again while leaving the nucleus.
- **MEC, meson-exchange currents:** the interaction involves currents carried between nucleons by exchanged mesons, contributing to a two-nucleon response. GENIE groups this contribution under its two-particle–two-hole (2p2h) mechanism: two nucleons are excited out of occupied nuclear states, leaving two vacancies.
- **RES, resonance production:** the lepton excites a nucleon to a baryon resonance, which then decays, typically to a nucleon and one or more mesons.
- **DIS, deep-inelastic scattering:** the lepton scatters from a quark inside the nucleon; GENIE also uses its DIS treatment for nonresonant inelastic hadron production.

These are input-generator classifications. The physical LUND converter does not infer a mechanism from final-state particles or recalculate it. It writes QE, MEC, RES, and DIS as process codes 1, 2, 3, and 4 in LUND header field 10, using that priority if malformed input sets more than one flag. This field is a process tag, not a cross-section weight. GST `resid` is written in header field 4, following the pinned RG-M GENIE-to-LUND converter where `RES_ID` replaced the earlier `targP` polarization value[^rgm-resid]. This preserves an established compatibility convention; it does not treat `resid` as polarization. The zero-based GST entry number is written in field 9.

Within each accepted event, the scattered electron is first. Protons, neutrons, charged pions, and photons then follow in GST order. Other species are skipped. Neutral pions must be decayed during upstream GENIE production so their daughter photons are present in GST; a residual PDG 111 entry is skipped because the physical LUND converter cannot reconstruct missing daughter four-momenta.

GENIE inhibits $\pi^0$ decay by default in the relevant decay configuration. When producing GST input for the physical LUND converter, set [`DecayParticleWithCode=111` to `true` in `CommonDecay.xml`](https://github.com/GENIE-MC/Generator/blob/6a91779ea7443dc7b6ea31ad89985f23be127e4d/config/CommonDecay.xml#L29). This makes GENIE place the daughter photons in the truth record before conversion.

The physical LUND converter copies momentum and samples one vertex position for each event, the position in the target assigned to its particles. All retained particles share it. It calculates particle energy from the copied momentum and the particle mass. It applies neither a fiducial cut (a selection by detector position or angle) nor a $Q^2$ cut. The `q2-cut` setting records the cut used when the input was generated; it does not perform that cut during conversion.

### Why upstream samples use $Q^2$ cuts

In the one-photon-exchange description, the inclusive electron-scattering cross section contains a $1/Q^4$ factor[^electrons-for-neutrinos]. Here $Q^2$ is the positive magnitude of the squared four-momentum transferred by the electron. The complete cross section also contains structure functions, which describe the target's response, and other kinematic factors, especially at low $Q^2$. Thus $1/Q^4$ explains the forward enhancement; it is not an exact scaling law for every channel. Neglecting the electron mass,

$$
Q^2 \simeq 4E_{e}E_{e'}\sin^2\!\left(\frac{\theta_e}{2}\right),
$$

where $E_e$ and $E_{e'}$ are the incident and scattered electron energies and $\theta_e$ is the scattered electron's angle to the beam. At fixed energies, smaller angles give smaller $Q^2$. Generation without a suitable lower limit can concentrate statistics on very-forward electrons below the useful CLAS12 acceptance. A minimum-$Q^2$ cut is not identical to a fixed angular cut because the scattered energy also varies.

The GENIE production campaign described here uses beam-dependent minimum-$Q^2$ cuts: $0.02\,\mathrm{GeV}^2$ at $2.07052\,\mathrm{GeV}$, $0.25\,\mathrm{GeV}^2$ at $4.02962\,\mathrm{GeV}$, and $0.40\,\mathrm{GeV}^2$ at $5.98636\,\mathrm{GeV}$. The $4$ and $6\,\mathrm{GeV}$ values are data-informed campaign choices intended to follow the lower edge of observed CLAS12 coverage, not universal detector thresholds supplied by the cited paper. The $2\,\mathrm{GeV}$ choice is GENIE's built-in electromagnetic-scattering threshold[^genie-em-q2-threshold]. These are upstream generation cuts, not detector-efficiency corrections. The code assumes that the supplied GST sample was generated with the minimum named by `q2-cut`; it does not calculate $Q^2$ or verify the input distribution. The user must therefore make the label match the actual truth-level generation cut. The physical LUND converter records that assertion as provenance without applying another cut.

## File cutoff

For example, with 250 supported input entries, `events = 300`, and `events-per-file = 100`, conversion writes two 100-event files. It scans entry 201 but does not write it: only 50 input entries remain, too few to start a third file.

`events` limits how many events are written. `events-per-file`, default 10,000, sets when to start a new file and when to stop near the end of the input. Before starting a second or later file, the physical LUND converter requires at least `events-per-file` input entries from the current entry onward. This check does not prevent starting the first file or interrupt a file already started. Reaching the event limit or the end of the input can still leave a short file.

The check counts input entries, including reactions that may later be skipped. Having enough input entries therefore does not guarantee a full output file. The manifest records entries examined, events written, and the count in each LUND file. Examined entries minus written events includes skipped reactions and, when this end-of-input rule stops conversion, the supported entry examined but not written. The final progress line states why conversion stopped.

## Provenance and output names

The physical LUND converter never derives scientific metadata from a filename. Set target, beam, generator version, tune, and $Q^2$ label explicitly or through a profile.

### Automatic GENIE tune lookup

`tune = auto` uses a GENIE-specific production convention, not a general event-generator feature. It is intended for GENIE samples generated with [`eAScatteringGridSubmitter.py`](https://github.com/GENIE-MC/Generator/blob/3a50ba6d0918f62b023194eb2d6b5267b2815868/src/scripts/production/python/eAScatteringGridSubmitter.py) using `--store-comitinfo`. That is the exact option spelling in the linked revision. It is disabled by default.

With that option enabled, the GENIE script writes `input_options.txt` in its `--jobs-topdir` directory. The file records the parsed production options, including `TUNE`, whose value comes from the script's `--tune` setting. This metadata file is separate from the GST ROOT files; the physical LUND converter does not create it.

The physical LUND converter searches upward from the local input path for the exact directory name `master-routine_validation_01-eScattering`, then reads the `TUNE` entry from `input_options.txt` in that directory's parent. This matches the GENIE script's default version, production name, and cycle. If those names were changed, the metadata file is absent or unreadable, `TUNE` is missing or empty, or the input is remote, automatic lookup records `unknown` and conversion continues.

For GENIE samples produced another way, or stored in another layout, supply `--tune NAME` explicitly. This skips the search. The metadata convention is not a requirement for converting valid GST input, and finding the file does not verify how the sample was generated.

The default nested layout is:

```text
OUTPUT/<target>/<event-generator>__<tune>/<q2-cut>__<beam-label>/
```

The automatic file prefix is `<target>__<event-generator>[-<event-generator-version>]__<tune>__<q2-cut>__<beam-label>`. `output-layout = metadata` instead puts target variation, generator/version, tune, selection, and beam into one directory name. The [configuration reference](configuration.md) defines both forms.

Physical conversion produces no ROOT/PDF/PNG monitoring. It writes LUND files, the completion manifest, and empty simulation-output directories.

[^electrons-for-neutrinos]: A. Papadopoulou et al. (electrons for neutrinos Collaboration), “Inclusive Electron Scattering And The GENIE Neutrino Event Generator,” *Phys. Rev. D* **103**, 113003 (2021). [doi:10.1103/PhysRevD.103.113003](https://doi.org/10.1103/PhysRevD.103.113003)

[^genie-em-q2-threshold]: GENIE defines its electromagnetic-scattering minimum as `kMinQ2Limit = 0.02` in [`KineUtils.h`](https://github.com/GENIE-MC/Generator/blob/master/src/Framework/Utils/KineUtils.h), in units of $\mathrm{GeV}^2$.

[^rgm-resid]: RG-M, *GENIE to LUND converter*, pinned revision `d0d6050`, [source line defining `RES_ID` in place of `targP`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GENIE_to_LUND.C#L25).
