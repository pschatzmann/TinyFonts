// TinyFonts - glyph rendering into an application-provided display.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "format.h"
#include "layout.h"
#include "utf8.h"

namespace tinyfonts {

// Output target. Implement drawPixel() for TFT/OLED drivers, Adafruit GFX,
// LVGL, a framebuffer, or anything else. `coverage` is 0 (transparent) to
// 255 (fully covered); blend it with the background yourself.
class FontDisplay {
public:
  virtual ~FontDisplay() {}
  virtual void drawPixel(int x, int y, uint8_t coverage) = 0;
};

// Wraps any callable taking (int x, int y, uint8_t coverage).
template <typename Fn>
class LambdaDisplay : public FontDisplay {
public:
  explicit LambdaDisplay(Fn fn) : _fn(fn) {}
  void drawPixel(int x, int y, uint8_t coverage) override {
    _fn(x, y, coverage);
  }

private:
  Fn _fn;
};

template <typename Fn>
inline LambdaDisplay<Fn> makeDisplay(Fn fn) {
  return LambdaDisplay<Fn>(fn);
}

// Draws text from a Font onto a FontDisplay. Coordinates are in pixels;
// (x, y) is the top-left corner of the first line box, and each line's
// baseline is `ascent` pixels below its top.
class TextRenderer {
public:
  TextRenderer(const Font& font, FontDisplay& display)
      : _font(font), _display(display) {}

  // Restricts output to [x0, x1) x [y0, y1).
  void setClip(int x0, int y0, int x1, int y1) {
    _clip = true;
    _cx0 = x0;
    _cy0 = y0;
    _cx1 = x1;
    _cy1 = y1;
  }
  void clearClip() { _clip = false; }

  // Draws a single line (no wrapping, '\n' is not special). Returns the
  // advance width of the line.
  int drawLine(int x, int y, const char* begin, const char* end) {
    const int baseline = y + _font.ascent();
    int pen = x;
    uint32_t prev = 0;
    bool hasPrev = false;
    const char* p = begin;
    while (p < end) {
      const uint32_t cp = decodeUtf8(p, end);
      if (cp == '\n') break;
      if (hasPrev) pen += kerning(_font, prev, cp);
      Glyph g;
      if (findGlyph(_font, cp, g)) {
        renderGlyph(g, pen, baseline);
        pen += g.advance;
      }
      prev = cp;
      hasPrev = true;
    }
    return pen - x;
  }

  // Draws text, honouring '\n' and wrapping at maxWidth (<= 0: no wrap).
  // Returns the size of the drawn block.
  TextBox drawText(int x, int y, const char* begin, const char* end,
                   int maxWidth = 0) {
    const int lineHeight = _font.lineHeight();
    TextBox box;
    int line = 0;
    box.lines = wrapText(_font, begin, end, maxWidth,
                         [&](const char* b, const char* e, int w) {
                           drawLine(x, y + line * lineHeight, b, e);
                           if (w > box.width) box.width = w;
                           ++line;
                         });
    box.height = box.lines * lineHeight;
    return box;
  }

  // Convenience overload for a NUL-terminated string.
  TextBox drawText(int x, int y, const char* text, int maxWidth = 0) {
    const char* end = text;
    while (*end) ++end;
    return drawText(x, y, text, end, maxWidth);
  }

private:
  void renderGlyph(const Glyph& g, int penX, int baseline) {
    if (g.width == 0 || g.height == 0) return;

    const uint8_t bpp = _font.bitsPerPixel();
    const uint32_t maxValue = (1u << bpp) - 1u;
    const size_t stride = (static_cast<size_t>(g.width) * bpp + 7) / 8;
    const size_t base = _font.bitmapBase() + g.bitmapOffset;
    const int x0 = penX + g.left;
    const int y0 = baseline - g.top;

    for (int row = 0; row < g.height; ++row) {
      const int y = y0 + row;
      if (_clip && (y < _cy0 || y >= _cy1)) continue;
      const size_t rowBase = base + static_cast<size_t>(row) * stride;

      for (int col = 0; col < g.width; ++col) {
        const int x = x0 + col;
        if (_clip && (x < _cx0 || x >= _cx1)) continue;

        const size_t bit = static_cast<size_t>(col) * bpp;
        const uint8_t byte = _font.u8(rowBase + bit / 8);
        const uint8_t shift = static_cast<uint8_t>(8 - bpp - (bit % 8));
        const uint32_t value = (byte >> shift) & maxValue;
        if (value == 0) continue;

        const uint8_t coverage =
            static_cast<uint8_t>((value * 255u + maxValue / 2) / maxValue);
        _display.drawPixel(x, y, coverage);
      }
    }
  }

  const Font& _font;
  FontDisplay& _display;
  bool _clip = false;
  int _cx0 = 0, _cy0 = 0, _cx1 = 0, _cy1 = 0;
};

}  // namespace tinyfonts
