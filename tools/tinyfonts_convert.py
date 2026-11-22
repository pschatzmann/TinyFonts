#!/usr/bin/env python3
"""Convert a TrueType/OpenType font into a TinyFonts C header.

Runs on the development machine. The output is a plain byte array in the
binary layout documented in src/TinyFonts/format.h, wrapped in a C header
that can be included from an Arduino sketch.

Example:
  python3 tools/tinyfonts_convert.py --font Lato-Regular.ttf --size 12 \
      --bpp 2 --name Lato12 --out src/TinyFonts/Fonts/Lato12.h

Requires: Pillow and fontTools (pip install pillow fonttools).
"""

import argparse
import os
import struct
import sys
import unicodedata

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

MAGIC = b"TNF1"
HEADER_SIZE = 24
GLYPH_SIZE = 16
KERN_SIZE = 10


# Named character sets for --chars. Each entry is a list of inclusive ranges.
CHARSETS = {
    "ascii": [(0x20, 0x7E)],
    "latin1": [(0x20, 0x7E), (0xA0, 0xFF)],
    "latin-ext": [(0x20, 0x7E), (0xA0, 0x24F), (0x1E00, 0x1EFF)],
    "greek": [(0x20, 0x7E), (0x370, 0x3FF)],
    "cyrillic": [(0x20, 0x7E), (0x400, 0x4FF), (0x500, 0x52F)],
    "punctuation": [(0x2010, 0x2027), (0x2030, 0x205E)],
    "currency": [(0x20A0, 0x20BF)],
    "euro": [(0x20AC, 0x20AC)],
}


def parse_chars(spec):
    """Parses a character specification into a sorted list of codepoints.

    Accepts comma-separated items, each one of:
      - a named set from CHARSETS, e.g. 'latin-ext' or 'cyrillic'
      - a range, e.g. '0x20-0x7E'
      - a single codepoint, e.g. '0x20AC' or '233'
    Example: 'ascii,latin-ext,greek,cyrillic,euro'
    """
    codepoints = set()
    for part in spec.split(","):
        part = part.strip().lower()
        if not part:
            continue
        if part in CHARSETS:
            ranges = CHARSETS[part]
        elif "-" in part:
            lo, hi = part.split("-", 1)
            ranges = [(int(lo, 0), int(hi, 0))]
        else:
            value = int(part, 0)
            ranges = [(value, value)]
        for lo, hi in ranges:
            if not (0 <= lo <= hi <= 0x10FFFF):
                sys.exit(f"error: invalid codepoint range in '{part}'")
            codepoints.update(range(lo, hi + 1))
    return sorted(codepoints)


def report_missing(font_path, codepoints):
    """Warns about codepoints the font has no glyph for (they fall back)."""
    cmap = TTFont(font_path).getBestCmap()
    # Unassigned code points (category Cn) are not characters, so skip them.
    missing = [cp for cp in codepoints
               if cp not in cmap and unicodedata.category(chr(cp)) != "Cn"]
    if missing:
        shown = ", ".join(f"U+{cp:04X}" for cp in missing[:12])
        more = f" and {len(missing) - 12} more" if len(missing) > 12 else ""
        print(f"warning: font has no glyph for {len(missing)} requested "
              f"character(s): {shown}{more}; they will use the fallback glyph",
              file=sys.stderr)
    return missing


def quantize(value, bpp):
    """Maps an 8-bit coverage value onto the 2**bpp levels of the format."""
    max_value = (1 << bpp) - 1
    return (value * max_value + 127) // 255


def pack_bitmap(pixels, width, height, bpp):
    """Packs row-major 8-bit coverage pixels into the TinyFonts bitmap layout."""
    stride = (width * bpp + 7) // 8
    out = bytearray(stride * height)
    for y in range(height):
        for x in range(width):
            level = quantize(pixels[y * width + x], bpp)
            bit = x * bpp
            out[y * stride + bit // 8] |= level << (8 - bpp - bit % 8)
    return bytes(out)


def render_glyph(font, codepoint, bpp):
    """Rasterizes one glyph. Returns a dict with metrics and packed bitmap."""
    ch = chr(codepoint)
    advance = round(font.getlength(ch))
    left, top, right, bottom = font.getbbox(ch, anchor="ls")
    width, height = right - left, bottom - top
    empty = {"width": 0, "height": 0, "left": 0, "top": 0,
             "advance": advance, "bitmap": b""}
    if width <= 0 or height <= 0:
        return empty

    image = Image.new("L", (width, height), 0)
    ImageDraw.Draw(image).text((-left, -top), ch, font=font, fill=255,
                               anchor="ls")
    pixels = image.tobytes()
    if not any(pixels):
        return empty

    return {
        "width": width,
        "height": height,
        "left": left,
        "top": -top,  # distance from the baseline up to the top row
        "advance": advance,
        "bitmap": pack_bitmap(pixels, width, height, bpp),
    }


def _gpos_pair_values(ttf, names):
    """Yields (left_name, right_name, font_units) from GPOS PairPos lookups.

    `names` is the set of glyph names of interest. Handles both pair formats
    (explicit pairs and class-based), including extension lookups.
    """
    if "GPOS" not in ttf:
        return
    lookups = ttf["GPOS"].table.LookupList
    if lookups is None:
        return
    for lookup in lookups.Lookup:
        subtables = lookup.SubTable
        lookup_type = lookup.LookupType
        if lookup_type == 9:  # extension wrapper
            lookup_type = subtables[0].ExtensionLookupType if subtables else 0
            subtables = [s.ExtSubTable for s in subtables]
        if lookup_type != 2:  # 2 = PairAdjustment
            continue

        for st in subtables:
            if st.Format == 1:
                coverage = st.Coverage.glyphs
                for index, first in enumerate(coverage):
                    if first not in names:
                        continue
                    for record in st.PairSet[index].PairValueRecord:
                        if record.SecondGlyph in names:
                            value = getattr(record.Value1, "XAdvance", 0) or 0
                            if value:
                                yield first, record.SecondGlyph, value
            elif st.Format == 2:
                classes1 = st.ClassDef1.classDefs
                classes2 = st.ClassDef2.classDefs
                for first in names:
                    c1 = classes1.get(first, 0)
                    for second in names:
                        c2 = classes2.get(second, 0)
                        record = st.Class1Record[c1].Class2Record[c2]
                        value = getattr(record.Value1, "XAdvance", 0) or 0
                        if value:
                            yield first, second, value


def load_kerning(ttf, codepoints, size):
    """Collects pair kerning from 'kern' and GPOS, scaled to pixels.

    Returns a sorted list of (left_codepoint, right_codepoint, pixels).
    """
    units_per_em = ttf["head"].unitsPerEm
    wanted = set(codepoints)
    name_to_cp = {}
    for cp, glyph_name in ttf.getBestCmap().items():
        if cp in wanted and glyph_name not in name_to_cp:
            name_to_cp[glyph_name] = cp
    if not name_to_cp:
        return []

    units = {}  # (left_name, right_name) -> font units
    if "kern" in ttf:
        for subtable in ttf["kern"].kernTables:
            for pair, value in getattr(subtable, "kernTable", {}).items():
                units[pair] = value
    for left, right, value in _gpos_pair_values(ttf, set(name_to_cp)):
        units[(left, right)] = value  # GPOS overrides the legacy table

    pairs = {}
    for (left, right), value in units.items():
        if left in name_to_cp and right in name_to_cp:
            pixels = round(value * size / units_per_em)
            if pixels:
                pairs[(name_to_cp[left], name_to_cp[right])] = pixels
    if not pairs:
        print("warning: no kerning pairs found in the selected characters",
              file=sys.stderr)
    return [(l, r, v) for (l, r), v in sorted(pairs.items())]


def build_blob(font_path, size, bpp, codepoints, fallback, line_gap,
               kerning_enabled=True):
    font = ImageFont.truetype(font_path, size)
    ascent, descent = font.getmetrics()
    if not (0 <= ascent <= 255 and 0 <= descent <= 255 and 0 <= line_gap <= 255):
        sys.exit("error: vertical metrics do not fit in one byte")

    glyphs = []
    for cp in codepoints:
        glyph = render_glyph(font, cp, bpp)
        glyph["codepoint"] = cp
        if glyph["width"] > 255 or glyph["height"] > 255:
            sys.exit(f"error: glyph U+{cp:04X} is too large")
        if not (-128 <= glyph["left"] <= 127 and -128 <= glyph["top"] <= 127):
            sys.exit(f"error: glyph U+{cp:04X} offset does not fit in i8")
        if glyph["advance"] > 65535:
            sys.exit(f"error: glyph U+{cp:04X} advance does not fit in u16")
        glyphs.append(glyph)

    if len(glyphs) > 65535:
        sys.exit("error: too many glyphs")

    report_missing(font_path, codepoints)
    ttf = TTFont(font_path)
    kerning = load_kerning(ttf, codepoints, size) if kerning_enabled else []
    kerning = [(l, r, v) for l, r, v in kerning if -32768 <= v <= 32767]

    if fallback not in {g["codepoint"] for g in glyphs}:
        print(f"warning: fallback U+{fallback:04X} is not in the character set",
              file=sys.stderr)

    bitmap_base = HEADER_SIZE + len(glyphs) * GLYPH_SIZE + len(kerning) * KERN_SIZE
    header = MAGIC + struct.pack("<BBBBHHIII", bpp, ascent, descent, line_gap,
                                 len(glyphs), 0, len(kerning), fallback,
                                 bitmap_base)

    glyph_table = bytearray()
    bitmaps = bytearray()
    for g in glyphs:
        glyph_table += struct.pack("<IIBBbbHxx", g["codepoint"], len(bitmaps),
                                   g["width"], g["height"], g["left"],
                                   g["top"], g["advance"])
        bitmaps += g["bitmap"]

    kern_table = bytearray()
    for left, right, value in kerning:
        kern_table += struct.pack("<IIh", left, right, value)

    blob = bytes(header) + bytes(glyph_table) + bytes(kern_table) + bytes(bitmaps)
    return blob, len(glyphs), len(kerning), ascent, descent


def emit_header(blob, name, source, info):
    lines = []
    lines.append("// Generated by tools/tinyfonts_convert.py -- do not edit.")
    lines.append(f"// Source: {source}")
    lines.append(f"// {info}")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <TinyFonts.h>")
    lines.append("")
    lines.append("namespace tinyfonts {")
    lines.append("")
    lines.append(f"static const uint8_t {name}_DATA[] PROGMEM = {{")
    for i in range(0, len(blob), 16):
        chunk = ", ".join(f"0x{b:02X}" for b in blob[i:i + 16])
        lines.append(f"  {chunk},")
    lines.append("};")
    lines.append("")
    lines.append(f"static const Font {name}({name}_DATA, true);")
    lines.append("")
    lines.append("}  // namespace tinyfonts")
    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--font", required=True, help="TTF/OTF file")
    parser.add_argument("--size", type=int, default=12,
                        help="font size in pixels (em height)")
    parser.add_argument("--bpp", type=int, choices=[1, 2, 4, 8], default=2,
                        help="coverage bits per pixel (default 2)")
    parser.add_argument("--chars", default="ascii",
                        help="characters: named sets (" + ", ".join(CHARSETS) +
                             "), ranges like 0x20-0x7E, or codepoints, "
                             "comma separated (default: ascii)")
    parser.add_argument("--fallback", default="0x3F",
                        help="codepoint drawn for unknown characters (default '?')")
    parser.add_argument("--line-gap", type=int, default=0,
                        help="extra pixels between lines (default 0)")
    parser.add_argument("--no-kerning", action="store_true",
                        help="omit the kerning table (much smaller)")
    parser.add_argument("--name", required=True,
                        help="C identifier for the font, e.g. Lato12")
    parser.add_argument("--out", required=True, help="output .h file")
    args = parser.parse_args()

    if not args.name.isidentifier():
        sys.exit("error: --name must be a valid C identifier")

    codepoints = parse_chars(args.chars)
    if not codepoints:
        sys.exit("error: no characters selected")
    fallback = int(args.fallback, 0)

    blob, glyph_count, kern_count, ascent, descent = build_blob(
        args.font, args.size, args.bpp, codepoints, fallback, args.line_gap,
        kerning_enabled=not args.no_kerning)

    info = (f"{args.size}px, {args.bpp} bpp, {glyph_count} glyphs, "
            f"{kern_count} kerning pairs, {len(blob)} bytes")
    text = emit_header(blob, args.name, os.path.basename(args.font), info)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"wrote {args.out}: {info}")


if __name__ == "__main__":
    main()
