# Sampling models and random numbers

The uniform LUND creator uses angles in degrees, momentum in $\mathrm{GeV}/c$, mass in $\mathrm{GeV}/c^2$, energy in $\mathrm{GeV}$, and vertex coordinates in centimeters. For momentum magnitude $p$ and direction $(\theta,\phi)$, ROOT constructs

$$
(p_x,p_y,p_z) = p(\sin\theta\cos\phi,\sin\theta\sin\phi,\cos\theta),\qquad E = \sqrt{p^2+m^2}.
$$

## Electron-only events

For `channel=1e`, $\theta$ and $\phi$ are uniform inside the configured ranges. The default momentum is a deterministic 50/50 mixture over bounds $a$ and $b$:

- even run-global event IDs draw $p\sim\mathcal{U}(a,b)$;
- odd IDs draw $q\sim\mathcal{U}(b^{-1},a^{-1})$ and use $p=q^{-1}$.

The production bounds are $a=0.7\,\mathrm{GeV}/c$ and $b=p_{\mathrm{beam}}$. `electron-momentum=uniform` selects only the first distribution. The $2.07052\,\mathrm{GeV}$ production profile changes the $\theta$ minimum from $5^\circ$ to $2^\circ$; this is a profile choice rather than a hidden beam rule.

The electron tester fixes $p=p_{\mathrm{beam}}$ while scanning $\theta\in[5^\circ,40^\circ]$ and the full azimuthal range.

## Electron–hadron events

For `channel=eh`, hadron $\theta$ is uniform inside the configured FD or CD range and $\phi\in[-180^\circ,180^\circ]$. The upper momentum bound is $p_{\mathrm{beam}}$.

Charged hadrons use the same run-global even/odd mixture of distributions uniform in $p$ and uniform in $1/p$. Neutrons use a distribution uniform in $p$, including a lower bound of zero. The optional fixed mode is neutron-only.

The trigger electron has beam momentum and configured $\theta$, normally $25^\circ$. Its $\phi$ is determined rather than sampled: find the center in $\{-120^\circ,-60^\circ,0^\circ,60^\circ,120^\circ,180^\circ\}$ closest to the direction opposite the hadron, then add the configured offset $\Delta\phi$. Equal-distance ties keep the first center checked. The same separation rule is retained for CD samples even though the CD geometry does not require it.

## Vertex positions

The target catalog resolves a geometry key. `TargetGeometry` uses the external `targets.h` implementation to draw $V_x$ and $V_y$ from its beam-spot distributions and $V_z$ from the selected target cell or foil positions. Exactly one vertex is drawn for each written event, and every particle in that event receives it.

In the checked-in target source, $V_x$ and $V_y$ are independent Gaussian draws,

$$
V_x,V_y\sim\mathcal{N}(0,\sigma^2),\qquad \sigma=0.04\,\mathrm{cm}.
$$

$V_z$ follows the resolved geometry:

| Geometry | $V_z$ prescription ($\mathrm{cm}$) |
| --- | --- |
| `Ar` | $V_z\sim\mathcal{U}(-5.75,-5.25)$ |
| `liquid` | $V_z\sim\mathcal{U}(-5.5,-0.5)$ |
| `4-foil` | Equal choice from $\{-4.875,-3.625,-2.375,-1.125\}$ |
| `1-foil` | $V_z=-0.5$ |
| `1-foil-small` | $V_z=-2.1$ |
| `1-foil-large` | $V_z=-2.32$ |
| `Ca` | $V_z=-3.0$ |

These values describe the checked-in `targets.h` snapshot. Recheck this table whenever that protected source is replaced.

Target identity, geometry, and LUND $A$/$Z$ metadata are distinct values. An $A$/$Z$ override does not change the vertex distribution.

## Random-stream ownership

Uniform creation owns two `TRandom3` objects:

- `seed` controls particle momentum and angles;
- `vertex-seed` controls target positions.

The streams remain separate, so a geometry change does not consume values from the kinematic sequence. The geometry adapter temporarily transfers the vertex RNG state through the external header's global generator under a mutex, then returns the updated state to the caller.

A nonzero seed repeats a sequence only when the complete configuration, software, ROOT version, and draw order also match. `TRandom3(0)` requests automatic seeding. A manifest containing zero therefore cannot reproduce the sequence from that value alone.
