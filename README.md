# Duck Pocket 0.9

Experimental JUCE 8.0.4 sidechain VST3 / AAX by Rainline Music. Renamed from
Phase Pocket in v0.9; fonts, colours and all three themes are unchanged.

## v0.9

- Editor performance: all static art is cached in one retina-aware image, the
  timer runs at 30 Hz, only the two plot rectangles are repainted, and traces are
  reduced to one min/max bucket per pixel column. Several open editors no longer
  saturate the host message thread.
- Processing range is now a dynamic bell / shelf: the dry path is never filtered
  and the selected band is subtracted in proportion to the ducking, so at rest the
  output is bit-identical to the input and the magnitude never exceeds unity.
  Group delay drops from 3-6 ms to below 1 ms at 60-200 Hz. Filtering still
  engages only once a processing range handle is moved.
- Freeze buttons with a snowflake glyph sit right of `NOW` in both graphs. They
  stop the graph for inspection and resume with fresh data only.

## Processing

A 5 ms lookahead soft-attack ducker. Influence 0-100 is linear depth; 100-150 is
exponential (100 = 1x, 125 = 2.83x, 150 = 8x). Duration sets the total key length
with the last 20% fading out; 2000 ms means infinity. The key filter is a
non-resonant 12 dB/oct HP+LP and full-range endpoints bypass it. M/S balance
changes processing depth, not output level.

## Builds

GitHub Actions builds macOS universal arm64+x86_64 and Windows x64 VST3 and AAX
packages and runs pluginval at strictness 5 plus the DSP regression suite.

This is experimental software. Back up old projects and plug-ins before
replacement.
