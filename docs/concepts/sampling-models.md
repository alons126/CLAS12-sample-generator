# Sampling models and random numbers

The uniform LUND creator uses angles in degrees, momentum in GeV/c, mass in GeV/c², energy in GeV, and vertex coordinates in centimeters. For magnitude p and direction theta, phi, ROOT constructs

\[
(p_x,p_y,p_z)=p(\sin\theta\cos\phi,\sin\theta\sin\phi,\cos\theta),
\qquad E=\sqrt{p^2+m^2}.
\]

## Electron-only events

For `channel=1e`, theta and phi are uniform inside the configured ranges. The default momentum is a deterministic 50/50 mixture over bounds a and b:

- even run-global event IDs draw p uniformly from [a,b];
- odd IDs draw q uniformly from [1/b,1/a] and use p=1/q.

The production bounds are a=0.7 GeV/c and b=beam momentum. `electron-momentum=uniform` selects only the first distribution. The 2.07052 GeV production profile changes the theta minimum from 5° to 2°; this is a profile choice rather than a hidden beam rule.

The electron tester fixes p to the beam momentum while scanning theta from 5–40° and full phi.

## Electron–hadron events

For `channel=eh`, hadron theta is uniform inside the configured FD or CD range and phi is uniform from −180° to 180°. The upper momentum bound is the beam momentum.

Charged hadrons use the same run-global even/odd mixture of uniform-p and uniform-1/p. Neutrons use uniform-p, including a lower bound of zero. The optional fixed mode is neutron-only.

The trigger electron has beam momentum and configured theta, normally 25°. Its phi is determined rather than sampled: find the center in {−120, −60, 0, 60, 120, 180}° closest to the direction opposite the hadron, then add the configured offset. Equal-distance ties keep the first center checked. The same separation rule is retained for CD samples even though the CD geometry does not require it.

## Vertex positions

The target catalog resolves a geometry key. `TargetGeometry` uses the external `targets.h` implementation to draw Vx and Vy from its beam-spot distributions and Vz from the selected target cell or foil positions. Exactly one vertex is drawn for each written event, and every particle in that event receives it.

In the checked-in target source, Vx and Vy are independent Gaussian draws with mean 0 and sigma 0.04 cm. Vz follows the resolved geometry:

| Geometry | Vz prescription in cm |
| --- | --- |
| `Ar` | Uniform from −5.75 to −5.25 |
| `liquid` | Uniform from −5.5 to −0.5 |
| `4-foil` | Equal choice of −4.875, −3.625, −2.375, and −1.125 |
| `1-foil` | Fixed at −0.5 |
| `1-foil-small` | Fixed at −2.1 |
| `1-foil-large` | Fixed at −2.32 |
| `Ca` | Fixed at −3.0 |

These values describe the checked-in `targets.h` snapshot. Recheck this table whenever that protected source is replaced.

Target identity, geometry, and LUND A/Z metadata are distinct values. An A/Z override does not change the vertex distribution.

## Random-stream ownership

Uniform creation owns two `TRandom3` objects:

- `seed` controls particle momentum and angles;
- `vertex-seed` controls target positions.

The streams remain separate, so a geometry change does not consume values from the kinematic sequence. The geometry adapter temporarily transfers the vertex RNG state through the external header's global generator under a mutex, then returns the updated state to the caller.

A nonzero seed repeats a sequence only when the complete configuration, software, ROOT version, and draw order also match. `TRandom3(0)` requests automatic seeding. A manifest containing zero therefore cannot reproduce the sequence from that value alone.
