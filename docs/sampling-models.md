# Sampling models and random-number conventions

## 1. Definitions

Momentum is expressed in GeV/c, mass in GeV/c² and energy in GeV (c=1), vertex coordinates in cm. CLI angles are in degrees and converted to radians for vector construction. In this chapter let U(a,b) denote a uniform draw between a and b.

The Cartesian momentum is

\[
(p_x,p_y,p_z)=p(\sin\theta\cos\phi,\sin\theta\sin\phi,\cos\theta),
\qquad E=\sqrt{p^2+m^2}.
\]

The original angular windows are retained:

| Sampled particle | θ range | φ range |
| --- | --- | --- |
| Electron (`1e` and angular tester) | 5–40° | −180–180° |
| Proton (`ep`) | 5–45° | −180–180° |
| Neutron (`en`) | 5–35° | −180–180° |

Settings expose these limits for explicit studies; no mode automatically expands them to a full sphere.

## 2. Electron samples

The ordinary `1e` mode draws θ uniformly in its angular interval, φ uniformly over azimuth and p uniformly from 0 to the beam value. Its density in θ is constant, so it is not uniform solid angle. This prescription matches the archived generator.

`electron-momentum=beam` fixes p to the beam value while retaining the same angular draws. The angular-tester profile uses `vertex-mode=fixed` at `(0,0,-3 cm)`. It does not consume vertex random numbers.

## 3. Fixed nucleon mode

`nucleon-momentum=fixed` retains the archived behavior: θ uniform over the channel window, φ uniform over azimuth and p fixed to `nucleon-p` (default 1 GeV/c). The fixed option applies to both ep and en.

## 4. Non-fixed neutron mode

`nucleon-momentum=sampled` resolves to `uniform` for en. With the default `nucleon-angle=auto`, the angular model resolves to `isotropic`:

\[
\mu=\cos\theta\sim U(\cos\theta_{max},\cos\theta_{min}),\quad
\phi\sim U(-\pi,\pi),\quad p\sim U(p_{min},p_{max}).
\]

Because dΩ=dφ d(cosθ), this is uniform in solid angle **within the unchanged window**. Equivalently,

\[
f_\theta(\theta)=\frac{\sin\theta}{\cos\theta_{min}-\cos\theta_{max}}.
\]

It is not uniform in three-dimensional momentum-space volume (which would imply a different radial density). Defaults are p_min=0.3 GeV/c and p_max=beam energy. This is a requested extension, not the archived fixed-momentum distribution.

## 5. Non-fixed proton mode

`nucleon-momentum=sampled` resolves to `mixed` for ep. Its θ and φ prescriptions remain the legacy ones. The two momentum components share strictly positive bounds a=p_min and b=p_max:

\[
p_U\sim U(a,b),\qquad q\sim U(1/b,1/a),\quad p_I=1/q.
\]

The inverse-momentum component has density and cumulative distribution

\[
f_I(p)=\frac{1}{(1/a-1/b)p^2},\qquad
F_I(p)=\frac{1/a-1/p}{1/a-1/b}.
\]

The combined density is

\[
f(p)=\frac{1}{2(b-a)}+\frac{1}{2(1/a-1/b)p^2},\quad a\leq p\leq b.
\]

The implementation alternates components using the run-global event index: even indices draw uniform p; odd indices draw uniform 1/p. Thus an even-length run contains exactly half of each component, and an odd run has one extra uniform-p event. Alternation continues across file boundaries; an odd-size file may contain one extra component. This implements uniform **1/p**, not a logarithmic distribution (density 1/p), and not a 50/50 mixture of weights.

`nucleon-momentum=uniform` remains an explicit pure-uniform-p option for studies. `nucleon-angle=theta|isotropic` is an explicit override; `auto` implements the production prescriptions above. Resolved modes appear in the manifest.

## 6. Trigger electron in ep/en

The trigger electron has momentum equal to beam energy and default θ=25°. Its φ is the closest of {−120,−60,0,60,120,180} degrees to the azimuth opposite the nucleon, followed by an offset. Ties retain the first angle in that ordered list, matching the archived code.

Automatic offsets are 16°, 7° and 5° at beam energies 2.07052, 4.02962 and 5.98636 GeV respectively, with a 10⁻⁶ GeV comparison tolerance; other energies use 0°. The new code uses the configured energy rather than a substring of the output path.

All particles from one event share a vertex. The trigger construction is unchanged when switching the nucleon momentum model.

## 7. RNG ownership and reproducibility

Uniform generation owns two `TRandom3` instances: `seed` for kinematics and `vertex-seed` for the target. GENIE conversion uses only the vertex stream. Defaults are 67890 and 12345 respectively. ROOT documents uniform/Gaussian draws and the special behavior of seed zero in its [TRandom](https://root.cern.ch/doc/master/classTRandom.html) and [TRandom3](https://root.cern.ch/doc/master/classTRandom3.html) references.

The new CLI rejects seed zero; the archived uniform caller constructed `TRandom3(0)`. Reference adapters substitute a known nonzero kinematic seed without changing the event kernel. Matching kernels, settings and ROOT with identical seeds reproduces tested LUND bytes. Existing production files cannot be reconstructed event-by-event from an unknown automatic RNG state. Distinct run/stream seeds should be selected for independent samples; a changed sampling algorithm can consume a different sequence of draws.

The [validation chapter](validation.md) describes empirical-CDF checks derived from the equations above.
