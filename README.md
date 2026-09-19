# Duck Pocket 0.10

JUCE 8.0.4 sidechain VST3 / AAX by Rainline Music.

## v0.10

- Display-synchronised UI capped at 60 fps.
- High-resolution 2.4 kHz graph capture, 16k history, per-pixel min/max aggregation and interpolated curved envelopes.
- Perspective tunnel graph grid in every theme.
- New warm Amber theme alongside Neon, Solid Dark and Solid White.
- Click-free 2.5 ms latency-aligned bypass crossfade.
- Duration automation is latched per event and cannot jump an active envelope.
- Both audio channels are represented in the oscilloscope.
- Dynamic filters use parked fast paths; the processor uses pointer-based block access.
- High-DPI chrome uses the actual graphics-context scale and is not regenerated while range handles are dragged.
- M/S percentages, centred range titles, edge-aligned live frequency labels, and persistent expanded-panel state.
- Deprecated parameter IDs remain loadable but are marked as non-automatable metadata.

See `V010-NOTES.md` for implementation details and test coverage.

## Processing

A 5 ms lookahead soft-attack ducker. Influence 0-100 is linear depth; 100-150 is exponential. Duration sets the total key length with the last 20% fading out; 2000 ms means infinity. The key filter is a non-resonant 12 dB/oct HP+LP and full-range endpoints bypass it. M/S balance changes processing depth, not output level.

Processing Range is a subtractive dynamic bell/shelf. The dry path is never permanently filtered; with no reduction the output is latency-aligned dry.

## Builds

GitHub Actions builds macOS universal arm64+x86_64 and Windows x64 VST3/AAX packages, runs pluginval at strictness 5, and executes `PocketV10Test`.

This is experimental software. Back up old projects and plug-ins before replacement.
