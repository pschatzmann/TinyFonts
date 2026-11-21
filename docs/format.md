# Font format

Each generated font is one binary blob. All multi-byte integers are
little-endian. The authoritative definition is the comment in
[src/TinyFonts/format.h](../src/TinyFonts/format.h).

## Header (24 bytes)

| Offset | Type | Field |
|---|---|---|
| 0 | `'T' 'N' 'F' '1'` | magic |
| 4 | u8 | bits per pixel (1, 2, 4 or 8) |
| 5 | u8 | ascent (pixels above the baseline) |
| 6 | u8 | descent (pixels below the baseline) |
| 7 | u8 | line gap |
| 8 | u16 | glyph count |
| 10 | u16 | reserved |
| 12 | u32 | kerning pair count |
| 16 | u32 | fallback code point |
| 20 | u32 | offset of the bitmap area from the start of the blob |

## Glyph table

`glyphCount` entries of 16 bytes, sorted by code point, so lookup is a binary
search.

| Offset | Type | Field |
|---|---|---|
| 0 | u32 | code point |
| 4 | u32 | bitmap offset, relative to the bitmap area |
| 8 | u8 | width in pixels |
| 9 | u8 | height in pixels |
| 10 | i8 | left offset from the pen position |
| 11 | i8 | distance from the baseline up to the top row |
| 12 | u16 | advance in pixels |
| 14 | 2 bytes | reserved |

## Kerning table

`kerningCount` entries of 10 bytes, sorted by (left, right):

| Offset | Type | Field |
|---|---|---|
| 0 | u32 | left code point |
| 4 | u32 | right code point |
| 8 | i16 | adjustment in pixels |

## Bitmap area

Glyphs are stored row by row, most significant bit first, with
`bitsPerPixel` bits per pixel. Each row starts on a byte boundary.
