# Pro Tools macOS performance validation

This repository's CI builds a universal AAX and checks its signature and both
architectures. It cannot measure Pro Tools' editor or audio behaviour. Run the
following on an Apple silicon Mac with Pro Tools, using the same session and
hardware buffer for each case.

1. Insert one Duck Pocket AAX on a playing stereo track. Record the Pro Tools
   version, macOS version, native/Rosetta mode, sample rate, display scaling,
   session buffer size and plug-in build number.
2. Observe playback for 30 seconds with the plug-in editor closed. Repeat with
   the editor open at 100 ms, 1 s, 2 s and 5 s. Use Solid Dark first; repeat
   100 ms and 5 s with Neon. Repeat 5 s with three open instances.
3. Record GUI responsiveness, Pro Tools CPU/AAE meter, any audio underruns,
   and frame rate (a screen recording is sufficient for visible stutters).
4. Attach Instruments **Time Profiler** to Pro Tools during each 30-second run.
   Inspect the main-thread time under `DuckPocketAudioProcessorEditor::graph`,
   `juce::PathStrokeType::createStrokedPath`, path flattening, and image drawing.
   Inspect the audio thread separately for `processAudio`/`pocket::Engine::process`.
5. Compare against the previous release and the VST3 in a macOS host with the
   same window and themes. If GUI time improves but audio underruns remain with
   the editor closed, investigate the audio path as a separate fault.

Acceptance: the 5 s AAX window should play without new underruns and should
keep the host responsive during a 30-second run, including with three editors
open. Check that peaks and gain minima remain visible and that freeze/resume,
resize, theme switches and presets behave correctly for all window sizes.

The 2 ms rollup only affects display data at 2 s and 5 s. DSP audio and saved
active parameter values are unchanged by the graph optimisation.
