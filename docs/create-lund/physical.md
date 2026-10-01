# Physical LUND conversion

The physical LUND converter translates existing event-generator truth into the project's common LUND output. It does not run an event generator, resample particle kinematics, compute cross sections, or apply acceptance cuts. The implemented adapter reads [GENIE](https://github.com/GENIE-MC/Generator) GST [ROOT](https://github.com/root-project/root) trees and is selected as `genie-gst`.

## Convert GENIE GST input

```tcsh
source run.csh \
    --workflow create-lund \
    --source physical \
    --config config/samples/physical-lund-creation/genie-gst.conf \
    --input '/path/to/gst*.root' \
    --events 100 \
    --output /path/to/output
```

Quote filename patterns so ROOT receives them unchanged. The input must contain a tree named `gst`; files matching the pattern are read as one ordered chain.

## Required GST fields

| Branches | ROOT types |
| --- | --- |
| `qel`, `mec`, `res`, `dis` | `Bool_t` |
| `resid`, `nf` | `Int_t` |
| `pxl`, `pyl`, `pzl` | `Double_t` |
| `pdgf[nf]` | `Int_t` array |
| `pxf[nf]`, `pyf[nf]`, `pzf[nf]` | `Double_t` arrays |

The adapter checks every required branch, stored type, and current array length before indexed access. Empty input, malformed records, read errors, and input containing no supported event all fail without a completion manifest.

## Selection and translation

The adapter keeps QE, MEC, RES, and DIS events. It writes their process codes as 1, 2, 3, and 4 in LUND header field 10, using that priority if malformed input sets more than one flag. This field is a process tag, not a cross-section weight. GST `resid` is written in header field 4, following the pinned RG-M GENIE-to-LUND converter where `RES_ID` replaced the earlier `targP` polarization value.[^rgm-resid] This preserves an established compatibility convention; it does not treat `resid` as polarization. The zero-based GST entry number is written in field 9.

Within each accepted event, the scattered electron is first. Protons, neutrons, charged pions, and photons then follow in GST order. Other species are skipped. Neutral pions must be decayed during upstream GENIE production so their daughter photons are present in GST; a residual PDG 111 entry is skipped because the converter cannot reconstruct missing daughter four-momenta.

The converter copies momentum and chooses one target vertex for the event. Every retained particle receives that same vertex. Particle energy is recalculated from the copied momentum and the project's mass source. No fiducial or $Q^2$ cut is applied. `q2-cut` is provenance describing the input selection.

### Why upstream samples use $Q^2$ cuts

In the one-photon-exchange description, the inclusive electron-scattering cross section contains a leading $1/Q^4$ dependence[^electrons-for-neutrinos]. The complete cross section also contains structure functions and other kinematic factors, especially at low $Q^2$, so $1/Q^4$ is the dominant scaling argument rather than an exact description of every channel. Since

$$
Q^2 \simeq 4E_{e}E_{e'}\sin^2\!\left(\frac{\theta_e}{2}\right),
$$

the smallest electron scattering angles correspond to the smallest $Q^2$ values. Generating without a suitable lower limit would therefore devote most events to very low-$Q^2$, very-forward electrons, including the region below the useful CLAS12 electron acceptance.

The truth-level GENIE samples used by this project apply beam-dependent minimum-$Q^2$ cuts chosen to follow the lower edge of the observed CLAS12 data coverage: $0.02\,\mathrm{GeV}^2$ at $2.07052\,\mathrm{GeV}$, $0.25\,\mathrm{GeV}^2$ at $4.02962\,\mathrm{GeV}$, and $0.40\,\mathrm{GeV}^2$ at $5.98636\,\mathrm{GeV}$. The $2\,\mathrm{GeV}$ choice is GENIE's built-in electromagnetic-scattering threshold.[^genie-em-q2-threshold] These are upstream generation cuts, not detector-efficiency corrections. The code assumes that the supplied GST sample was generated with the minimum named by `q2-cut`; it does not calculate $Q^2$ or verify the input distribution. The user must therefore make the label match the actual truth-level generation cut. The converter records that assertion as provenance without applying another cut.

## File cutoff

`events` limits written events. `events-per-file`, default 10,000, controls both file rollover and the physical-input tail rule. Before an accepted event would start a second or later file, the converter requires at least one full `events-per-file` block of input entries beginning with that entry. The first file is always allowed, and a file that has started is never interrupted.

The cutoff counts input entries, not accepted events. Unsupported reactions inside an allowed block can therefore produce a short LUND file. The completion manifest records scanned and written counts plus every per-file count; their difference includes skipped input. The final progress line names the stop reason.

## Provenance and output names

The converter never derives scientific metadata from a filename. Set target, beam, generator version, tune, and $Q^2$ label explicitly or through a profile. With `tune = auto`, it looks for the exact `TUNE` entry in `input_options.txt` above the standard `master-routine_validation_01-eScattering/` directory and otherwise records `unknown`.

The default nested layout is:

```text
OUTPUT/<target>/<event-generator>__<tune>/<Q2-label>__<beam-MeV>MeV/
```

The automatic file prefix is `<target>__<event-generator>[-<version>]__<tune>__<Q2-label>__<beam-MeV>MeV`. `output-layout = metadata` instead puts target variation, generator/version, tune, selection, and beam into one directory name. The [configuration reference](configuration.md) defines both forms.

Physical conversion produces no ROOT/PDF/PNG monitoring. It writes LUND files, the completion manifest, and empty simulation-output directories.

[^electrons-for-neutrinos]: A. Papadopoulou et al. (electrons for neutrinos Collaboration), “Inclusive Electron Scattering And The GENIE Neutrino Event Generator,” *Phys. Rev. D* **103**, 113003 (2021). [doi:10.1103/PhysRevD.103.113003](https://doi.org/10.1103/PhysRevD.103.113003)

[^genie-em-q2-threshold]: GENIE defines its electromagnetic-scattering minimum as `kMinQ2Limit = 0.02` in [`KineUtils.h`](https://github.com/GENIE-MC/Generator/blob/master/src/Framework/Utils/KineUtils.h#L113), in units of $\mathrm{GeV}^2$.

[^rgm-resid]: RG-M, *GENIE to LUND converter*, pinned revision `d0d6050`, [source line defining `RES_ID` in place of `targP`](https://github.com/awild7/rgm/blob/d0d60503229a57784d25c8ea3cd71f9e061f295c/Simulation/GENIE_to_LUND.C#L25).
