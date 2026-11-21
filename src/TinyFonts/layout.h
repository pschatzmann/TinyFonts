// TinyFonts - text measurement, kerning and word wrapping.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "format.h"
#include "utf8.h"

namespace tinyfonts {

// Size of a laid-out block of text, in pixels.
struct TextBox {
  int width = 0;
  int height = 0;
  int lines = 0;
};

// Advance of one codepoint (0 when neither it nor the fallback exists).
inline int advanceOf(const Font& font, uint32_t codepoint) {
  Glyph g;
  return findGlyph(font, codepoint, g) ? g.advance : 0;
}

// Splits [begin, end) into lines and calls onLine(lineBegin, lineEnd, width)
// for each one. Lines end at '\n', and at a space or character that would
// exceed maxWidth (maxWidth <= 0 disables wrapping). Returns the line count.
// Each call makes progress, so the loop always terminates.
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

// Measures text without drawing it. maxWidth <= 0 means no wrapping.
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
