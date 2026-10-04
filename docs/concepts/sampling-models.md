# Sampling models and random numbers

The uniform LUND creator uses angles in degrees, momentum in $\mathrm{GeV}/c$, mass in $\mathrm{GeV}/c^2$, energy in $\mathrm{GeV}$, and vertex coordinates in centimeters. For momentum magnitude $P$ and direction $(\theta,\phi)$, ROOT constructs

$$
(P_x,P_y,P_z) = P(\sin\theta\cos\phi,\sin\theta\sin\phi,\cos\theta),\qquad E = \sqrt{P^2+m^2}.
$$

The code converts degree-valued angles to radians before calling trigonometric functions. The energy calculation uses natural units ($c=1$). For these artificial samples, $P_{\mathrm{beam}}$ is numerically the configured beam energy: for example, $5.98636\,\mathrm{GeV}$ gives a $5.98636\,\mathrm{GeV}/c$ momentum bound. This uses the high-energy electron approximation rather than subtracting the electron mass to derive an exact beam momentum.

## Electron-only events

### Electron-tester

The electron-tester profiles fix $P=P_{\mathrm{beam}}$ while scanning $\theta\in[5^\circ,40^\circ]$ and the full azimuthal range. The electron angle and momentum options can override those profile choices.

### 1e

For `channel=1e`, $\theta$ and $\phi$ are uniform inside the configured ranges. The default momentum alternates between two random distributions, rather than randomly choosing which distribution to use. With lower bound $a$ and upper bound $b$:

- even run-global event IDs draw $P\sim\mathcal{U}(a,b)$;
- odd IDs draw $q\sim\mathcal{U}(b^{-1},a^{-1})$ and use $P=q^{-1}$.

Here $\mathcal{U}(a,b)$ denotes a uniform distribution between $a$ and $b$. Alternation gives equal contributions for an even event count; an odd count has one extra uniform-in-$P$ event. Angles are uniform in $\theta$, not in solid angle: this gives equal statistics in equal-width $\theta$ bins rather than an isotropic distribution.

The production bounds are $a=0.7\,\mathrm{GeV}/c$ and $b=P_{\mathrm{beam}}$. `electron-momentum=uniform` selects only the first distribution. The $2.07052\,\mathrm{GeV}$ production profile changes the $\theta$ minimum from $5^\circ$ to $2^\circ$; this is a profile choice rather than a hidden beam rule.

## Electron–hadron events

For `channel=eh`, hadron $\theta$ is uniform inside the configured FD or CD range and $\phi\in[-180^\circ,180^\circ]$. The upper momentum bound is $P_{\mathrm{beam}}$.

Charged hadrons use the same run-global even/odd mixture of distributions uniform in $P$ and uniform in $1/P$. Neutrons use a distribution uniform in $P$, including a lower bound of zero. The optional fixed mode is neutron-only.

The trigger electron has beam momentum and configured $\theta$, normally $25^\circ$. Its $\phi$ is determined rather than sampled: find the center in $\{-120^\circ,-60^\circ,0^\circ,60^\circ,120^\circ,180^\circ\}$ closest to the direction opposite the hadron, then add the configured offset $\Delta\phi$. Equal-distance ties keep the first center checked. The same separation rule is retained for CD samples even though the CD geometry does not require it.

## Vertex positions

The target catalog resolves a geometry key. `TargetGeometry` uses the external [`targets.h`](../../src/workflows/lund-creation/external/targets.h) implementation to draw $V_x$ and $V_y$ from its beam-spot distributions and $V_z$ from the selected target cell or foil positions. Exactly one vertex is drawn for each written event, and every particle in that event receives it.

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

These values describe the checked-in [`targets.h`](../../src/workflows/lund-creation/external/targets.h) snapshot. Recheck this table whenever that protected source is replaced.

Target identity, geometry, and LUND $A$/$Z$ metadata are distinct values. An $A$/$Z$ override does not change the vertex distribution.

## Random-stream ownership

Uniform creation owns two `TRandom3` objects:

- `seed` controls particle momentum and angles;
- `vertex-seed` controls vertex positions.

Each `TRandom3` object is a random-number generator (RNG) with its own sequence. Changing the target geometry draws from the vertex sequence, not the momentum-and-angle sequence. To use the external header, the geometry adapter temporarily copies the caller's vertex RNG state into the header's global RNG and copies the updated state back afterward. A mutex, a lock preventing simultaneous access, protects that shared global state.

A nonzero seed repeats a sequence only when the complete configuration, software, ROOT version, and draw order also match. [`TRandom3(0)`](https://root.cern.ch/doc/master/classTRandom3.html) requests automatic seeding. A manifest containing zero therefore cannot reproduce the sequence from that value alone.
