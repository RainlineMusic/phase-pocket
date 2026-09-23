# Duck Pocket 1.0.0

JUCE 8.0.4 sidechain VST3 / AAX by Rainline Music.

## v1.0.0

- Display-synchronised UI: 60 fps for short windows, 30 fps for 2–5 s windows.
- High-resolution 2.4 kHz graph capture and 16k history; 2 ms extrema rollups and bounded paths for long windows.
- Perspective tunnel graph grid in every theme.
- New warm Amber theme alongside Neon, Solid Dark and Solid White.
- Click-free 2.5 ms latency-aligned bypass crossfade.
- Duration automation follows an active envelope with smoothing.
- Both audio channels are represented in the oscilloscope.
- Dynamic filters use parked fast paths; the processor uses pointer-based block access.
- High-DPI chrome uses the actual graphics-context scale and is not regenerated while range handles are dragged.
- M/S percentages, centred range titles, edge-aligned live frequency labels, and persistent expanded-panel state.
- Old state values for removed controls are ignored while current settings are restored. Old automation for removed IDs cannot be restored.

See `PERFORMANCE-VALIDATION.md` for the Pro Tools macOS validation procedure and `V010-NOTES.md` for version history.

## Processing

A 5 ms lookahead soft-attack ducker. Influence 0-100 is linear depth; 100-150 is exponential. Duration sets the total key length (5 ms minimum): the first half holds, the second half fades out; 2000 ms means infinity. Changes apply live and every new hit restarts the event. The key filter is a non-resonant 12 dB/oct HP+LP and full-range endpoints bypass it. M/S balance changes processing depth, not output level.

Processing Range is a subtractive dynamic bell/shelf. The dry path is never permanently filtered; with no reduction the output is latency-aligned dry.

## Builds

GitHub Actions builds macOS universal arm64+x86_64 and Windows x64 VST3/AAX packages, runs pluginval at strictness 5, and executes the current DSP tests.

This is experimental software. Back up old projects and plug-ins before replacement.
