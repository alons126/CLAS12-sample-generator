# Sampling models and random-number conventions

Momentum is in GeV/c, mass in GeV/c², energy in GeV, vertices in cm, and configured angles in degrees. For magnitude p and angles θ,φ, the generated Cartesian momentum is

\[
(p_x,p_y,p_z)=p(\sin\theta\cos\phi,\sin\theta\sin\phi,\cos\theta),
\qquad E=\sqrt{p^2+m^2}.
\]

## Electron-only mode

`channel=1e` draws θ uniformly from 5–40° and φ uniformly over the full azimuth. Its default momentum alternates by run-global event index over bounds \(a=0.7\) GeV/c and \(b=E_{beam}\): even indices use \(p\sim U(a,b)\), and odd indices use \(1/p\sim U(1/b,1/a)\). `electron-momentum=uniform` selects pure uniform-p; `beam` is the electron-tester mode.

The tester always keeps the 5–40° and full-φ scan at beam momentum. It provides a rough estimate for the trigger-electron placement in electron–hadron samples; the fixed 25° trigger value was selected from that scan.

## Electron–hadron mode

`channel=eh` selects the second particle with `hadron=proton|neutron|pip|pim` and its angular region with `hadron-region=FD|CD`.

| Hadron | FD θ | CD θ | FD p minimum | CD p minimum | Default p model |
| --- | --- | --- | ---: | ---: | --- |
| proton | 5–45° | 35–145° | 0.3 | 0.2 | mixed |
| neutron | 5–35° | 35–145° | 0 | 0 | uniform |
| pip, pim | 5–45° | 35–140° | 0.2 | 0.1 | mixed |

Maximum p is the beam energy. `theta` draws θ uniformly; optional `isotropic` draws cos(theta) uniformly within the same configured limits. φ is always uniform over −180° to 180°. Fixed 1 GeV/c momentum is an optional neutron-only mode in either region.

For charged hadrons, `mixed` alternates uniform-p and uniform-1/p:

\[
p_U\sim U(a,b),\qquad q\sim U(1/b,1/a),\quad p_I=1/q.
\]

Its inverse component has cumulative distribution

\[
F_I(p)=\frac{1/a-1/p}{1/a-1/b}.
\]

Even event indices use \(p_U\); odd indices use \(p_I\). The alternation continues across file boundaries. Neutrons default to pure uniform-p so their distribution can cover migration around analysis-imposed thresholds.

## Trigger electron

The `eh` trigger electron has beam momentum and θ=25°. Its φ is the closest center in {−120,−60,0,60,120,180} degrees to the direction opposite the hadron, followed by the configured beam offset. The opposite-sector correlation is not obligatory for CD hadrons; it is retained deliberately to be sure the trigger electron follows the established separated placement. All particles in an event share one sampled vertex.

## RNG ownership

Uniform generation owns separate `TRandom3` streams for kinematics (`seed`) and geometry (`vertex-seed`). Nonzero seeds are repeatable when the complete configuration, software, ROOT version, and draw order match. `TRandom3(0)` requests ROOT automatic seeding and is intentionally nonrepeatable; recording zero does not record the internal seed. Reference comparisons therefore use known nonzero seeds.
