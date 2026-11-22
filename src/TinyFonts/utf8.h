// TinyFonts - UTF-8 decoding.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tinyfonts {

/// Code point returned for malformed UTF-8 (U+FFFD).
constexpr uint32_t kReplacementChar = 0xFFFD;

/// Decodes one UTF-8 sequence at p and advances p past it. Malformed input yields kReplacementChar. p must be less than end.
inline uint32_t decodeUtf8(const char*& p, const char* end) {
  const unsigned char c = static_cast<unsigned char>(*p++);
  if (c < 0x80) return c;

  uint32_t cp;
  uint8_t extra;
  uint32_t minValue;
  if ((c & 0xE0) == 0xC0) {
    cp = c & 0x1F;
    extra = 1;
    minValue = 0x80;
  } else if ((c & 0xF0) == 0xE0) {
    cp = c & 0x0F;
    extra = 2;
    minValue = 0x800;
  } else if ((c & 0xF8) == 0xF0) {
    cp = c & 0x07;
    extra = 3;
    minValue = 0x10000;
  } else {
    return kReplacementChar;
  }

  for (uint8_t i = 0; i < extra; ++i) {
    if (p >= end) return kReplacementChar;
    const unsigned char cc = static_cast<unsigned char>(*p);
    if ((cc & 0xC0) != 0x80) return kReplacementChar;
    cp = (cp << 6) | (cc & 0x3F);
    ++p;
  }

  if (cp < minValue || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
    return kReplacementChar;
  }
  return cp;
}

}  // namespace tinyfonts
