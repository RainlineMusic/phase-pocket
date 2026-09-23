# Duck Pocket v0.10 (historical notes)

These notes describe development before v1.0.0. For the current rendering
budget, parameter list and Pro Tools test procedure, use `README.md` and
`PERFORMANCE-VALIDATION.md`.

## Display and UI

- Replaced the 30 Hz GUI timer with a display-synchronised `VBlankAttachment`, capped at 60 fps.
- Increased graph capture from about 400 to about 2400 packets/s. A 100 ms view now has roughly 240 source packets instead of 40.
- Expanded editor history to 16,384 packets (over 6.8 seconds at 2.4 kHz), enough for the 5 second view.
- Graphs still aggregate min/max into one bucket per pixel column. Empty columns in short windows are interpolated between real extrema, and the outlines use curved paths.
- Graph paint vectors and buckets are preallocated and reused; the 60 fps path performs no routine vector allocations.
- Dynamic graph loops stop once samples are older than the selected window.
- Reduced each animated glow from two extra strokes to one.
- Static chrome uses the graphics context's real physical pixel scale rather than the monitor's nominal scale.
- Added the warm `Amber` theme based on the supplied brown/orange reference.
- Replaced rectangular graph grids with curved perspective tunnel rails in every theme.
- M/S control now shows `Mid: n%` and `Side: n%`; left movement reduces the Side percentage and right movement reduces Mid.
- Sidechain Filter and Processing Range titles are centred over their sliders. Frequency values are aligned to the left and right edges.
- Expanded-panel state is saved both with the plug-in state and in UI preferences.
- Range labels are dynamic overlays, so dragging no longer rebuilds the full high-DPI chrome image on every mouse event.
- Oscilloscope packets now combine both channels instead of silently displaying only channel 1.

## Audio/DSP

- Added a 2.5 ms click-free bypass crossfade between processed audio and latency-aligned dry audio. It adds no latency.
- Duration is latched when an event begins. Automating Duration cannot jump an envelope already in progress; the new value is used by the next event.
- Added pointer-based block access in the processor instead of per-sample `getSample`/`setSample` calls.
- The processing-range filter is skipped while fully parked.
- Sidechain HP/LP processing is skipped while the sidechain range is fully parked.
- Removed unused audio-thread meter atomics.
- Parameter and input non-finite values are sanitised before filter configuration.
- Deprecated parameters retain their IDs for session compatibility but are marked non-automatable metadata.

## Tests and CI

- New `Tests/v10_test.cpp` keeps the complete v0.9 DSP suite and adds:
  - click-size bound and settled-identity checks for bypass;
  - Duration automation continuity and next-event behaviour;
  - NaN/Inf sanitisation.
- CMake and CI now build/run `PocketV10Test`.
- Version and artifact names are updated to 0.10.
- Windows builds define `NOMINMAX`/`WIN32_LEAN_AND_MEAN`; source is compiled as UTF-8.

## v0.10.1 follow-up

- Added a display-only 3-tap reconstruction filter after per-pixel aggregation to suppress one-pixel bucket phase shimmer while traces scroll. DSP data is unchanged.
- Fixed 1-20 ms double notches: the detector now requires a real 4 ms low-level re-arm gap before a decaying/ringing key tail can start another event.
- Gain History uses exact parabolic cylinder geometry: end lenses at 10% height, inward quarter rings at 4%, a straight centre ring, full top/bottom generators, and middle generators terminating on the inner lenses.
- Gain History and Oscilloscope now share one Graph Window setting from 100 ms to 5 s.
- Added `PocketShortDurationTest` covering 1, 2, 5, 10 and 20 ms decaying key tails.
