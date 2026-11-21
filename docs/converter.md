# Font converter

`tools/tinyfonts_convert.py` runs on your development machine. It rasterizes a
TrueType or OpenType font at one pixel size and bit depth, extracts metrics and
kerning, and writes a C header with the font as a `PROGMEM` byte array.

## Requirements

```sh
pip install pillow fonttools
```

## Usage

```sh
python3 tools/tinyfonts_convert.py \
    --font Lato-Regular.ttf --size 16 --bpp 2 \
    --chars ascii,latin1,euro --name Lato16 --out MySketch/Lato16.h
```

| Option | Default | Meaning |
|---|---|---|
| `--font` | required | TTF or OTF file |
| `--size` | 12 | font size in pixels (em height) |
| `--bpp` | 2 | coverage bits per pixel: 1, 2, 4 or 8 |
| `--chars` | `ascii` | characters to include (see below) |
| `--fallback` | `0x3F` (`?`) | code point drawn for characters not in the font |
| `--line-gap` | 0 | extra pixels between lines |
| `--no-kerning` | off | leave out the kerning table (much smaller) |
| `--name` | required | C identifier, also the name of the `Font` object |
| `--out` | required | output `.h` file |

## Character sets

`--chars` takes a comma-separated list. Each item can be:

- a named set: `ascii`, `latin1`, `latin-ext`, `greek`, `cyrillic`,
  `punctuation`, `currency`, `euro`
- a range, for example `0x20-0x7E`
- a single code point, for example `0x20AC`

Example: `--chars ascii,latin1,euro`. The converter warns about characters the
font has no glyph for. They are then drawn with the fallback glyph.

## Size

Kerning pairs grow roughly with the square of the number of characters, so
large character sets get big. At 16 px, for example:

| Characters | Size |
|---|---|
| `ascii,latin1,euro` | about 45 KB |
| `ascii,latin1,euro,greek` | about 82 KB |
| `ascii,latin1,euro,greek,cyrillic` | about 290 KB |
| `ascii,latin1,euro` with `--no-kerning` | about 31 KB |

Use `--no-kerning` on small devices, or a smaller character set.

## Kerning

Kerning comes from the legacy `kern` table and from GPOS pair positioning,
including class-based pairs. Other GPOS features are not read.

## Sizes are pixels

The size is the em height in pixels, which is how Pillow and FreeType
interpret it. A "12 px" font has a line height of about 15 px.
