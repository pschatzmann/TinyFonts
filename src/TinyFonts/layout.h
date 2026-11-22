// TinyFonts - text measurement, kerning and word wrapping.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "format.h"
#include "utf8.h"

namespace tinyfonts {

/// @brief Size of a laid-out block of text, in pixels, with its line count.
struct TextBox {
  /// Width of the widest line, in pixels.
  int width = 0;
  /// Total height of all lines, in pixels.
  int height = 0;
  /// Number of lines.
  int lines = 0;
};

/// Advance of one code point in pixels, or 0 if neither it nor the fallback exists.
inline int advanceOf(const Font& font, uint32_t codepoint) {
  Glyph g;
  return findGlyph(font, codepoint, g) ? g.advance : 0;
}

// Each call makes progress, so the loop always terminates.
/// Splits [begin, end) into lines at '\n', at spaces, and before characters that would exceed maxWidth (maxWidth <= 0 disables wrapping). Calls onLine(lineBegin, lineEnd, width) for each line and returns the number of lines.
template <typename OnLine>
inline int wrapText(const Font& font, const char* p, const char* end,
                    int maxWidth, OnLine onLine) {
  int lines = 0;
  while (p < end) {
    const char* lineStart = p;
    const char* lineEnd = end;
    const char* next = end;
    int lineWidth = -1;

    int width = 0;
    uint32_t prev = 0;
    bool hasPrev = false;
    const char* breakAt = nullptr;      // position of the last space
    const char* breakNext = nullptr;    // position after that space
    int breakWidth = 0;                 // width before that space

    while (p < end) {
      const char* charStart = p;
      const uint32_t cp = decodeUtf8(p, end);
      if (cp == '\n') {
        lineEnd = charStart;
        next = p;
        lineWidth = width;
        break;
      }

      const int kern = hasPrev ? kerning(font, prev, cp) : 0;
      const int adv = advanceOf(font, cp);
      if (cp == ' ') {
        breakAt = charStart;
        breakNext = p;
        breakWidth = width;
      }

      if (maxWidth > 0 && charStart != lineStart &&
          width + kern + adv > maxWidth) {
        if (breakAt) {
          lineEnd = breakAt;
          next = breakNext;
          lineWidth = breakWidth;
        } else {
          // No space to break at: split the word before this character.
          lineEnd = charStart;
          next = charStart;
          lineWidth = width;
        }
        break;
      }

      width += kern + adv;
      prev = cp;
      hasPrev = true;
    }

    if (lineWidth < 0) lineWidth = width;
    onLine(lineStart, lineEnd, lineWidth);
    ++lines;
    p = next;
  }
  return lines;
}

/// Measures text without drawing it. maxWidth <= 0 disables wrapping.
inline TextBox measureText(const Font& font, const char* begin, const char* end,
                           int maxWidth = 0) {
  TextBox box;
  box.lines = wrapText(font, begin, end, maxWidth,
                       [&box](const char*, const char*, int w) {
                         if (w > box.width) box.width = w;
                       });
  box.height = box.lines * font.lineHeight();
  return box;
}

}  // namespace tinyfonts
