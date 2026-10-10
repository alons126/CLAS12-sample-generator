# Uniform LUND creation

The uniform LUND creator randomly chooses particle momenta and angles within configured ranges. These samples help study which particles CLAS12 can detect and reconstruct. They do not model real electron–nucleus interactions: the particles in an event are not required to satisfy an interaction's energy and momentum balance.

## Run a reviewed profile

Use [`run.csh`](../../run.csh) with the profile [`uniform-1e-5986MeV.conf`](../../config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf):

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/quickstart-output
```

The profile is a text file containing the sample settings. `--events 100` replaces its event count with a small number for a basic check. Without that override, electron-tester profiles request 1,000,000 events and production profiles request 50,000,000. Command-line values take priority over profile values. The [sample-profile inventory](../../config/samples/README.md) lists the files for every implemented channel at $2.07052\,\mathrm{GeV}$, $4.02962\,\mathrm{GeV}$, and $5.98636\,\mathrm{GeV}$.

## Available channels

| Channel | Event content | Output label |
| --- | --- | --- |
| $(e,e')$ angular tester (`electron-tester`) | One beam-momentum electron in an angular scan | `electron-tester` |
| $(e,e')$ (`1e`) | One sampled electron | `1e` |
| $(e,e'h)$ (`eh`) | One trigger electron followed by one selected hadron | One of the electron-hadron labels defined below |

For `eh`, select `--hadron proton|neutron|pip|pim` and `--hadron-region FD|CD`. In these command-line values, `pip` means a positively charged pion ($\pi^{+}$) and `pim` means a negatively charged pion ($\pi^{-}$). `FD` means the forward-detector sampling region and `CD` means the central-detector sampling region. These names describe the generated angular range; they do not claim that GEMC or reconstruction will detect the particle.

The uniform LUND creator does not generate kaon samples because kaon yields in the physical data are low. The supported hadrons are protons, neutrons, and charged pions.

### Electron-hadron labels

Each output label joins `e` for the trigger electron, the selected hadron, and the detector-region abbreviation. The electron is always written first, followed by the hadron, and both particles receive the same sampled vertex position.

| Label | Hadron option | Region | Event content |
| --- | --- | --- | --- |
| `epFD` | `proton` | Forward detector (FD) | $e^{-}p$ |
| `enFD` | `neutron` | Forward detector (FD) | $e^{-}n$ |
| `epipFD` | `pip` | Forward detector (FD) | $e^{-}\pi^{+}$ |
| `epimFD` | `pim` | Forward detector (FD) | $e^{-}\pi^{-}$ |
| `epCD` | `proton` | Central detector (CD) | $e^{-}p$ |
| `enCD` | `neutron` | Central detector (CD) | $e^{-}n$ |
| `epipCD` | `pip` | Central detector (CD) | $e^{-}\pi^{+}$ |
| `epimCD` | `pim` | Central detector (CD) | $e^{-}\pi^{-}$ |

The automatic run name and filename prefix are `Uniform__<channel-label>__<beam-label>`. Events are numbered from zero across the whole run. Starting a new output file does not restart those event numbers.

`events` is the total run size. `events-per-file` defaults to 25,000 and controls LUND file splitting. The completion manifest records the exact count in every file; submission uses the largest event count among the selected files as the common GEMC/reconstruction event limit.

## Production definitions

The electron tester scans $\theta\in[5^\circ,40^\circ]$ and full $\phi$ at $P=P_{\mathrm{beam}}$ while sampling the selected target geometry. It is the rough angular study from which the $25^\circ$ trigger-electron prescription was selected.

The 1e sample draws $\theta\in[5^\circ,40^\circ]$ and $\phi$ over the full azimuth. Its default momentum alternates between distributions uniform in $P$ and uniform in $1/P$, from $0.7\,\mathrm{GeV}/c$ to $P_{\mathrm{beam}}$. The $2.07052\,\mathrm{GeV}$ outbending profile deliberately extends $\theta$ down to $2^\circ$. These are uniform-sampling bounds chosen to cover the forward-electron region described for CLAS12 and its electromagnetic calorimeter; they are not detector-efficiency cuts[^burkert-clas12][^asryan-ecal].

Electron–hadron samples use a beam-momentum trigger electron at $\theta_e=25^\circ$. Its $\phi$ is placed at the CLAS12 sector center closest to the direction opposite the hadron, then shifted by $\Delta\phi=16^\circ$ at $2.07052\,\mathrm{GeV}$, $7^\circ$ at $4.02962\,\mathrm{GeV}$, $5^\circ$ at $5.98636\,\mathrm{GeV}$, and $0^\circ$ at other beam energies unless overridden. This separation rule is retained for CD samples even though the CD geometry does not require it. The hadron generation bounds below were chosen to cover the relevant CLAS12 forward-detector and central-detector regions described by the spectrometer and reconstruction system[^burkert-clas12][^ziegler-reconstruction].

| Hadron | FD $\theta$ | CD $\theta$ | FD $P_{\min}$ | CD $P_{\min}$ | Default momentum model |
| --- | --- | --- | ---: | ---: | --- |
| $p$ | $[5^\circ,45^\circ]$ | $[35^\circ,145^\circ]$ | $0.3\,\mathrm{GeV}/c$ | $0.2\,\mathrm{GeV}/c$ | mixed uniform in $P$ / uniform in $1/P$ |
| $n$ | $[5^\circ,35^\circ]$ | $[35^\circ,145^\circ]$ | 0 | 0 | uniform in $P$ |
| $\pi^{+}$ or $\pi^{-}$ | $[5^\circ,45^\circ]$ | $[35^\circ,140^\circ]$ | $0.2\,\mathrm{GeV}/c$ | $0.1\,\mathrm{GeV}/c$ | mixed uniform in $P$ / uniform in $1/P$ |

Every hadron momentum range ends at $P_{\mathrm{beam}}$. Hadron $\theta$ is uniform within its configured range and $\phi\in[-180^\circ,180^\circ]$, so equal-width angular bins receive comparable generated statistics. The optional fixed-momentum mode is neutron-only and defaults to $1\,\mathrm{GeV}/c$. The [sampling model](../concepts/sampling-models.md) gives the exact distributions, trigger correlation, and random-stream rules.

## Validation status

The electron-tester, 1e, epFD, and enFD modes are the production-tested uniform modes. FD charged-pion modes and all CD modes are implemented and covered by software checks but have not completed detector-level production validation. Their profile headers retain this warning.

## Targets and repeatability

The reviewed uniform profiles use `target = Ar40`. They resolve to `target-geometry = Ar`, $A=40$, $Z=18$, and `gemc-target-variation = rgm_fall2021_Ar`. For another target, target identity and beam energy select the compatible detector variation and vertex geometry; an explicit `gemc-target-variation` override changes the resolved geometry as well. $A$ / $Z$ are separate LUND-header metadata and do not silently change geometry.

The default `seed = 67890` controls particle kinematics, while `vertex-seed = 12345` controls vertex positions through a separate random stream. Nonzero seeds are repeatable when the software, complete configuration, and draw order match. [`TRandom3(0)`](https://root.cern.ch/doc/master/classTRandom3.html) requests automatic, nonrepeatable seeding, so a manifest containing zero cannot reproduce that sequence from the recorded value alone.

Electron, proton, neutron, and charged-pion masses come from the external [`targets.h`](../../src/workflows/lund-creation/external/targets.h); photon mass is exactly zero. The [LUND data contract](../concepts/lund-data-contract.md) lists their serialized values and precision.

## Results

The uniform LUND creator writes split LUND files, a completion manifest, one ROOT monitoring file, one combined PDF, and individual PNG plots. It also prepares empty `mchipo/` and `reconhipo/` directories for later submission. See [uniform monitoring](monitoring.md) and [run directories](../getting-started/outputs.md).

[^burkert-clas12]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^ziegler-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)

[^asryan-ecal]: G. Asryan et al., “The CLAS12 forward electromagnetic calorimeter,” *Nucl. Instrum. Meth. A* **959**, 163425 (2020). [doi:10.1016/j.nima.2020.163425](https://doi.org/10.1016/j.nima.2020.163425)
