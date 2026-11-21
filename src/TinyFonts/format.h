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

constexpr size_t kHeaderSize = 24;
constexpr size_t kGlyphEntrySize = 16;
constexpr size_t kKerningEntrySize = 10;

// Read-only view of a font blob stored in RAM or in flash (PROGMEM).
class Font {
public:
  constexpr Font(const uint8_t* data = nullptr, bool inFlash = false)
      : _data(data), _inFlash(inFlash) {}

  // True when the blob starts with the TinyFonts magic.
  bool valid() const {
    return _data != nullptr && u8(0) == 'T' && u8(1) == 'N' && u8(2) == 'F' &&
           u8(3) == '1';
  }

  uint8_t bitsPerPixel() const { return u8(4); }
  int ascent() const { return u8(5); }
  int descent() const { return u8(6); }
  int lineGap() const { return u8(7); }
  int lineHeight() const { return ascent() + descent() + lineGap(); }
  uint16_t glyphCount() const { return u16(8); }
  uint32_t kerningCount() const { return u32(12); }
  uint32_t fallbackCodepoint() const { return u32(16); }
  size_t bitmapBase() const { return u32(20); }
  bool inFlash() const { return _inFlash; }

  // Raw little-endian accessors (offset is relative to the blob start).
  uint8_t u8(size_t offset) const {
#if TF_HAS_PGM
    if (_inFlash) return pgm_read_byte(_data + offset);
#endif
    return _data[offset];
  }
  uint16_t u16(size_t offset) const {
    return static_cast<uint16_t>(u8(offset) | (u8(offset + 1) << 8));
  }
  uint32_t u32(size_t offset) const {
    return static_cast<uint32_t>(u16(offset)) |
           (static_cast<uint32_t>(u16(offset + 2)) << 16);
  }
  int8_t i8(size_t offset) const { return static_cast<int8_t>(u8(offset)); }
  int16_t i16(size_t offset) const { return static_cast<int16_t>(u16(offset)); }

private:
  const uint8_t* _data;
  bool _inFlash;
};

// Metrics and bitmap location of one glyph.
struct Glyph {
  uint32_t codepoint = 0;
  uint32_t bitmapOffset = 0;
  uint8_t width = 0;
  uint8_t height = 0;
  int8_t left = 0;
  int8_t top = 0;
  uint16_t advance = 0;
};

inline size_t glyphEntryOffset(uint16_t index) {
  return kHeaderSize + static_cast<size_t>(index) * kGlyphEntrySize;
}

inline size_t kerningEntryOffset(const Font& font, size_t index) {
  return kHeaderSize + static_cast<size_t>(font.glyphCount()) * kGlyphEntrySize +
         static_cast<size_t>(index) * kKerningEntrySize;
}

// Looks up a codepoint in the glyph table (binary search).
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

// Finds a glyph, using the fallback codepoint when the character is missing.
inline bool findGlyph(const Font& font, uint32_t codepoint, Glyph& out) {
  if (!font.valid()) return false;
  if (lookupGlyph(font, codepoint, out)) return true;
  const uint32_t fallback = font.fallbackCodepoint();
  return fallback != codepoint && lookupGlyph(font, fallback, out);
}

// Kerning adjustment in pixels for the pair (left, right); 0 when not kerned.
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
