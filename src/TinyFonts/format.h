// TinyFonts - embedded font format and accessors.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

// Binary layout of a TinyFonts blob. All multi-byte integers are little-endian.
//
//   Header (24 bytes)
//     0  'T' 'N' 'F' '1'          magic
//     4  u8  bitsPerPixel         1, 2, 4 or 8 (coverage depth of each pixel)
//     5  u8  ascent               pixels above the baseline
//     6  u8  descent              pixels below the baseline
//     7  u8  lineGap             extra pixels between lines
//     8  u16 glyphCount
//     10 u16 reserved
//     12 u32 kerningCount        32-bit: large Unicode sets have many pairs
//     16 u32 fallbackCodepoint    used for characters that are not in the font
//     20 u32 bitmapBase           offset of the bitmap area from the blob start
//
//   Glyph table: glyphCount entries of 16 bytes, sorted by codepoint
//     0 u32 codepoint
//     4 u32 bitmapOffset          relative to bitmapBase
//     8 u8  width                 bitmap width in pixels
//     9 u8  height                bitmap height in pixels
//     10 i8 left                  x offset of the bitmap from the pen position
//     11 i8 top                   distance from the baseline up to the top row
//     12 u16 advance              pen advance in pixels
//     14 2 bytes reserved
//
//   Kerning table: kerningCount entries of 10 bytes, sorted by (left, right)
//     0 u32 left codepoint
//     4 u32 right codepoint
//     8 i16 adjustment in pixels (added after `left`, before `right`)
//
//   Bitmap area: each glyph is stored row by row, most significant bit first,
//   with bitsPerPixel bits per pixel. Every row starts on a byte boundary.

namespace tinyfonts {

/// Size of the fixed header in bytes.
constexpr size_t kHeaderSize = 24;
/// Size of one glyph table entry in bytes.
constexpr size_t kGlyphEntrySize = 16;
/// Size of one kerning table entry in bytes.
constexpr size_t kKerningEntrySize = 10;

/**
 * @brief Read-only view of a TinyFonts font blob in RAM or flash.
 *
 * Metrics, glyph lookup and kerning are read directly from the blob, which is
 * never copied. Generate blobs with tools/tinyfonts_convert.py.
 */
class Font {
public:
  /// Creates a view of a font blob. Set inFlash for blobs stored in PROGMEM.
  constexpr Font(const uint8_t* data = nullptr, bool inFlash = false)
      : _data(data), _inFlash(inFlash) {}

  /// True when the blob is non-null and starts with the magic bytes "TNF1".
  bool valid() const {
    return _data != nullptr && u8(0) == 'T' && u8(1) == 'N' && u8(2) == 'F' &&
           u8(3) == '1';
  }

  /// Coverage depth per pixel: 1, 2, 4 or 8.
  uint8_t bitsPerPixel() const { return u8(4); }
  /// Pixels above the baseline.
  int ascent() const { return u8(5); }
  /// Pixels below the baseline.
  int descent() const { return u8(6); }
  /// Extra pixels between lines.
  int lineGap() const { return u8(7); }
  /// Line height: ascent + descent + line gap.
  int lineHeight() const { return ascent() + descent() + lineGap(); }
  /// Number of glyphs in the glyph table.
  uint16_t glyphCount() const { return u16(8); }
  /// Number of kerning pairs.
  uint32_t kerningCount() const { return u32(12); }
  /// Code point drawn for characters the font does not contain.
  uint32_t fallbackCodepoint() const { return u32(16); }
  /// Offset of the bitmap area from the start of the blob.
  size_t bitmapBase() const { return u32(20); }
  /// True if the blob is stored in flash (PROGMEM).
  bool inFlash() const { return _inFlash; }

  // Raw little-endian accessors (offset is relative to the blob start).
  /// Reads one byte at offset, from flash or RAM.
  uint8_t u8(size_t offset) const {
#if TF_HAS_PGM
    if (_inFlash) return pgm_read_byte(_data + offset);
#endif
    return _data[offset];
  }
  /// Reads a little-endian 16-bit value at offset.
  uint16_t u16(size_t offset) const {
    return static_cast<uint16_t>(u8(offset) | (u8(offset + 1) << 8));
  }
  /// Reads a little-endian 32-bit value at offset.
  uint32_t u32(size_t offset) const {
    return static_cast<uint32_t>(u16(offset)) |
           (static_cast<uint32_t>(u16(offset + 2)) << 16);
  }
  /// Reads one signed byte at offset.
  int8_t i8(size_t offset) const { return static_cast<int8_t>(u8(offset)); }
  /// Reads a little-endian signed 16-bit value at offset.
  int16_t i16(size_t offset) const { return static_cast<int16_t>(u16(offset)); }

private:
  const uint8_t* _data;
  bool _inFlash;
};

/// @brief Metrics and bitmap location of one glyph, read from the glyph table.
struct Glyph {
  /// Unicode code point of the glyph.
  uint32_t codepoint = 0;
  /// Offset of the glyph bitmap, relative to the bitmap area.
  uint32_t bitmapOffset = 0;
  /// Bitmap width in pixels.
  uint8_t width = 0;
  /// Bitmap height in pixels.
  uint8_t height = 0;
  /// Horizontal offset from the pen position to the bitmap.
  int8_t left = 0;
  /// Distance from the baseline up to the top bitmap row.
  int8_t top = 0;
  /// Pen advance after the glyph, in pixels.
  uint16_t advance = 0;
};

/// Byte offset of glyph table entry `index` in the blob.
inline size_t glyphEntryOffset(uint16_t index) {
  return kHeaderSize + static_cast<size_t>(index) * kGlyphEntrySize;
}

/// Byte offset of kerning table entry `index` in the blob.
inline size_t kerningEntryOffset(const Font& font, size_t index) {
  return kHeaderSize + static_cast<size_t>(font.glyphCount()) * kGlyphEntrySize +
         static_cast<size_t>(index) * kKerningEntrySize;
}

/// Looks up one code point in the glyph table, without fallback. Returns false if it is not present.
inline bool lookupGlyph(const Font& font, uint32_t codepoint, Glyph& out) {
  size_t lo = 0;
  size_t hi = font.glyphCount();
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (font.u32(glyphEntryOffset(static_cast<uint16_t>(mid))) < codepoint) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  if (lo >= font.glyphCount()) return false;
  const size_t o = glyphEntryOffset(static_cast<uint16_t>(lo));
  if (font.u32(o) != codepoint) return false;
  out.codepoint = codepoint;
  out.bitmapOffset = font.u32(o + 4);
  out.width = font.u8(o + 8);
  out.height = font.u8(o + 9);
  out.left = font.i8(o + 10);
  out.top = font.i8(o + 11);
  out.advance = font.u16(o + 12);
  return true;
}

/// Finds a glyph, using the fallback code point if the character is missing. Returns false if neither exists.
inline bool findGlyph(const Font& font, uint32_t codepoint, Glyph& out) {
  if (!font.valid()) return false;
  if (lookupGlyph(font, codepoint, out)) return true;
  const uint32_t fallback = font.fallbackCodepoint();
  return fallback != codepoint && lookupGlyph(font, fallback, out);
}

/// Kerning adjustment in pixels for the pair (left, right); 0 if the pair is not kerned.
inline int kerning(const Font& font, uint32_t left, uint32_t right) {
  if (!font.valid()) return 0;
  size_t lo = 0;
  size_t hi = font.kerningCount();
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    const size_t o = kerningEntryOffset(font, mid);
    const uint32_t l = font.u32(o);
    const uint32_t r = font.u32(o + 4);
    if (l < left || (l == left && r < right)) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  if (lo >= font.kerningCount()) return 0;
  const size_t o = kerningEntryOffset(font, lo);
  if (font.u32(o) == left && font.u32(o + 4) == right) return font.i16(o + 8);
  return 0;
}

}  // namespace tinyfonts
