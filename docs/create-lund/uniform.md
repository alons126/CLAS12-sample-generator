# Uniform LUND creation

The uniform LUND creator makes controlled acceptance-test events. It samples configured momentum and angle ranges but does not model an electron–nucleus interaction or enforce exclusive energy-momentum conservation.

## Run a reviewed profile

```tcsh
source run.csh \
    --workflow create-lund \
    --source uniform \
    --config config/samples/uniform-lund-creation/uniform-1e-5986MeV.conf \
    --events 100 \
    --output /path/to/output
```

The checked-in profile provides the sample definition. The explicit `--events 100` makes this a smoke test; production profiles request 50,000,000 events, while electron-tester profiles request 1,000,000. Command-line values override matching profile values. The [sample-profile inventory](../../config/samples/README.md) lists the profile pattern for every implemented channel at 2.07052, 4.02962, and 5.98636 GeV.

## Available channels

| Channel | Event content | Output label |
| --- | --- | --- |
| `1e` | One sampled electron | `1e` |
| `electron-tester` | One beam-momentum electron in an angular scan | `electron-tester` |
| `eh` | One trigger electron followed by one selected hadron | `epFD`, `enFD`, `epipFD`, `epimFD`, `epCD`, `enCD`, `epipCD`, or `epimCD` |

For `eh`, select `--hadron proton|neutron|pip|pim` and `--hadron-region FD|CD`. The electron is always written first. Both particles receive the same sampled vertex position.

The automatic run name and filename prefix are `Uniform__<label>__<beam-MeV>MeV`. File splitting does not restart the zero-based event-number sequence.

`events` is the total run size. `events-per-file` defaults to 25,000 and controls LUND file splitting. The completion manifest records the exact count in every file; submission uses the largest selected file count as the common GEMC/reconstruction event limit.

## Production definitions

The 1e sample draws theta from 5–40° and phi from the full azimuth. Its default momentum alternates between uniform-p and uniform-1/p from 0.7 GeV/c to the beam momentum. The 2.07052 GeV outbending profile deliberately extends theta down to 2°. These are project generation bounds chosen to cover the forward-electron region described for CLAS12 and its electromagnetic calorimeter; they are not detector-efficiency cuts.[^burkert-clas12][^asryan-ecal]

Electron–hadron samples use a beam-momentum trigger electron at 25°. Its phi is placed at the CLAS12 sector center closest to the direction opposite the hadron, then shifted by 16° at 2.07052 GeV, 7° at 4.02962 GeV, 5° at 5.98636 GeV, and 0° at other beam energies unless overridden. This separation rule is retained for CD samples even though the CD geometry does not require it. The hadron generation bounds below were chosen to cover the relevant CLAS12 forward-detector and central-detector regions described by the spectrometer and reconstruction system.[^burkert-clas12][^ziegler-reconstruction]

| Hadron | FD theta | CD theta | FD p minimum | CD p minimum | Default momentum model |
| --- | --- | --- | ---: | ---: | --- |
| proton | 5–45° | 35–145° | 0.3 GeV/c | 0.2 GeV/c | mixed uniform-p / uniform-1/p |
| neutron | 5–35° | 35–145° | 0 | 0 | uniform-p |
| $\pi^{+}$ or $\pi^{-}$ | 5–45° | 35–140° | 0.2 GeV/c | 0.1 GeV/c | mixed uniform-p / uniform-1/p |

Every hadron momentum range ends at the beam momentum. Hadron theta is uniform within its configured range and phi is uniform from −180° to 180°, so equal-width angular bins receive comparable generated statistics. The optional fixed-momentum mode is neutron-only and defaults to 1 GeV/c. The [sampling model](../concepts/sampling-models.md) gives the exact distributions, trigger correlation, and random-stream rules.

The electron tester scans 5–40° and full phi at beam momentum while sampling the selected target geometry. It is the rough angular study from which the 25° trigger-electron prescription was selected.

## Validation status

The 1e, epFD, enFD, and electron-tester modes are the production-tested uniform modes. FD charged-pion modes and all CD modes are implemented and covered by software checks but have not completed detector-level production validation. Their profile headers retain this warning.

## Targets and repeatability

The reviewed uniform profiles use `target = Ar40`. They resolve to `target-geometry = Ar`, `A = 40`, `Z = 18`, and `gemc-target-variation = rgm_fall2021_Ar`. For another target, target identity and beam energy select the compatible detector variation and vertex geometry; an explicit `gemc-target-variation` override changes the resolved geometry as well. A/Z are separate LUND-header metadata and do not silently change geometry.

The default `seed = 67890` controls particle kinematics, while `vertex-seed = 12345` controls target positions through a separate random stream. Nonzero seeds are repeatable when the software, complete configuration, and draw order match. `TRandom3(0)` requests automatic, nonrepeatable seeding, so a manifest containing zero cannot reproduce that sequence from the recorded value alone.

Electron, proton, neutron, and charged-pion masses come from the protected target source; photon mass is exactly zero. The [LUND data contract](../concepts/lund-data-contract.md) lists their serialized values and precision.

## Results

The creator writes split LUND files, a completion manifest, one ROOT monitoring file, one combined PDF, and individual PNG plots. It also prepares empty `mchipo/` and `reconhipo/` directories for later submission. See [uniform monitoring](monitoring.md) and [run directories](../getting-started/outputs.md).

[^burkert-clas12]: V. D. Burkert et al., “The CLAS12 Spectrometer at Jefferson Laboratory,” *Nucl. Instrum. Meth. A* **959**, 163419 (2020). [doi:10.1016/j.nima.2020.163419](https://doi.org/10.1016/j.nima.2020.163419)

[^ziegler-reconstruction]: V. Ziegler et al., “The CLAS12 software framework and event reconstruction,” *Nucl. Instrum. Meth. A* **959**, 163472 (2020). [doi:10.1016/j.nima.2020.163472](https://doi.org/10.1016/j.nima.2020.163472)

[^asryan-ecal]: G. Asryan et al., “The CLAS12 forward electromagnetic calorimeter,” *Nucl. Instrum. Meth. A* **959**, 163425 (2020). [doi:10.1016/j.nima.2020.163425](https://doi.org/10.1016/j.nima.2020.163425)
