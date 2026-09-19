# Duck Pocket v0.9

Renamed from Phase Pocket. Fonts, colours and all three themes (Neon, Solid Dark,
Solid White) are unchanged; only names, identifiers and version strings moved.

- Product: `Duck Pocket`, target `DuckPocket`, bundle/AAX id `ru.rainlinemusic.duckpocket`,
  plugin code `DkPk`. UI preferences migrated to `duckPocket.ui.*` with a fallback
  read of the old `phasePocket.ui.*` keys so existing window size, theme and graph
  windows survive the rename.

## 1. Several open editors made the host stutter

The old editor repainted the entire window from scratch on every timer tick:
background gradient, seven glow curves, two graph panels with grids and labels,
the side panel with its four shadow passes, and full text glow (nine draw calls
per label). On top of that, each trace path was built from every history sample
(the processor pushed a trace every 1200th of a second, so up to ~8000 points per
graph), at 60 Hz.

One editor fit inside a frame. Two did not, and because every plug-in window in
Ableton shares one message thread, the backlog showed up as sluggish window
dragging with low CPU load - the thread was stuck in the paint queue, not in the
audio thread. Three or more and the message thread never caught up, so the whole
host appeared frozen.

What changed:

- All static art (background, gradient, glow curves, title, graph frames, grids,
  axis and time labels, side panel, expanded panel text) is rendered once into a
  retina-aware cached image (`paintChrome` / `invalidateChrome`) and blitted.
  It is only re-rendered on theme, size, panel or range changes.
- Timer rate 60 Hz -> 30 Hz.
- Timer ticks repaint only the two plot rectangles (`gainArea`, `scopeArea`) and
  only when new traces actually arrived.
- Traces are reduced to one min/max bucket per pixel column before a path is
  built, so path complexity is bounded by the graph width instead of the history
  length.
- The processor decimates traces to `sr/400` instead of `sr/1200`, which is still
  far more resolution than a 604 px wide plot can show.
- A frozen graph is not repainted at all.

Net effect: per editor, a steady-state frame now paints two small rectangles with
at most ~600 points each, instead of the full window with thousands.

## 2. Dynamic band instead of a crossover (processing range)

v0.8 split the signal with a 4th order Linkwitz-Riley three way crossover, scaled
the middle branch and summed it back. That kept magnitude roughly flat but left a
permanent phase step at both crossover points, a measured ripple up to +0.9 dB
around the edges, and 3-6 ms of group delay at bass crossover frequencies - even
when no ducking was happening.

v0.9 never filters the dry path. The filter only extracts the selection `S(x)`,
and the engine applies

    out = dry - (1 - g) * S(dry)

which is a dynamic bell / shelf: at `g = 1` the output is bit-identical to the
input, the magnitude never exceeds unity anywhere, the phase deviation scales
with the ducking depth instead of being permanent, and the group delay is that of
a single second order section.

Shapes follow the handles:

| Handles moved | Shape | Filter |
| --- | --- | --- |
| both | dynamic bell | normalised SVF bandpass, Q from the range width (0.9-8) |
| low only | dynamic high shelf | first order TPT highpass |
| high only | dynamic low shelf | first order TPT lowpass |

As before, filtering engages only once a processing range handle is moved. With
both handles parked at 20 Hz / 20 kHz every weight and the mix are zero and the
plugin runs the plain wideband duck, bit for bit (covered by a test).

Measured at 48 kHz, range 300-3000 Hz, ~0.52 reduction:

| Hz | 40 | 100 | 200 | 300 | 500 | 1k | 2k | 3k | 5k | 8k | 12k |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| gain | 0.999 | 0.995 | 0.978 | 0.948 | 0.834 | 0.487 | 0.871 | 0.949 | 0.984 | 0.994 | 0.998 |

Group delay of the dynamic edge: -0.90 ms at 60 Hz, -0.54 ms at 100 Hz,
-0.27 ms at 200 Hz (v0.8 crossover: 3-6 ms). Reported plugin latency is unchanged
at 5 ms of lookahead, since the filter adds no reported delay.

Linear phase was considered and rejected: a phase-flat FIR steep enough to be
useful at 100 Hz needs tens of milliseconds of latency and pre-ringing smears the
duck, which is the opposite of what a lookahead ducker wants.

## 3. Freeze buttons

A small square button with a snowflake glyph sits to the right of `NOW` in both
graphs (gain history and oscilloscope). Pressing it snapshots the current history
and the graph stops. Pressing it again resumes, starting from new data only - the
old trace is dropped rather than scrolled away. The glyph lights up in the theme
accent colour while frozen. Frozen graphs are excluded from repaints.

## Tests

`Tests/v09_test.cpp` (CI target `PocketV09Test`) extends the v0.8 suite with:

- no gain above unity anywhere from 40 Hz to 12 kHz for a 300-3000 Hz range,
- parked processing range is bit-identical to the wideband duck,
- group delay bounds at 60 / 100 / 200 Hz.
