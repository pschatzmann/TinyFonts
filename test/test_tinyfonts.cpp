// Host unit tests for TinyFonts. Builds a small font in memory using the
// documented binary layout, then checks parsing, layout and rendering.
#include <TinyFonts.h>

#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace tinyfonts;

static int failures = 0;

#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                          \
    }                                                                      \
  } while (0)

#define CHECK_EQ(a, b)                                                     \
  do {                                                                     \
    long long va_ = (a), vb_ = (b);                                        \
    if (va_ != vb_) {                                                      \
      std::printf("%s:%d: %s == %s failed (%lld vs %lld)\n", __FILE__,     \
                  __LINE__, #a, #b, va_, vb_);                             \
      ++failures;                                                          \
    }                                                                      \
  } while (0)

// ---- Font builder (writes the format documented in format.h) --------------

namespace {

struct Writer {
  std::vector<uint8_t> d;
  void u8(unsigned v) { d.push_back(static_cast<uint8_t>(v & 0xFF)); }
  void u16(unsigned v) { u8(v); u8(v >> 8); }
  void u32(uint32_t v) { u16(v & 0xFFFF); u16(v >> 16); }
  void i8(int v) { u8(static_cast<uint8_t>(static_cast<int8_t>(v))); }
  void i16(int v) { u16(static_cast<uint16_t>(static_cast<int16_t>(v))); }
};

struct GlyphSpec {
  uint32_t cp;
  int w, h, left, top, adv;
  std::vector<int> levels;  // row-major, already quantized to 0..2^bpp-1
};

struct KernSpec {
  uint32_t left, right;
  int value;
};

std::vector<uint8_t> buildFont(int bpp, int ascent, int descent, int gap,
                               uint32_t fallback,
                               const std::vector<GlyphSpec>& glyphs,
                               const std::vector<KernSpec>& kerns) {
  Writer glyphTable, bitmaps;
  for (const GlyphSpec& g : glyphs) {
    const int stride = (g.w * bpp + 7) / 8;
    std::vector<uint8_t> bits(stride * g.h, 0);
    for (int y = 0; y < g.h; ++y)
      for (int x = 0; x < g.w; ++x) {
        const int bit = x * bpp;
        bits[y * stride + bit / 8] |= static_cast<uint8_t>(
            g.levels[y * g.w + x] << (8 - bpp - bit % 8));
      }
    glyphTable.u32(g.cp);
    glyphTable.u32(static_cast<uint32_t>(bitmaps.d.size()));
    glyphTable.u8(g.w);
    glyphTable.u8(g.h);
    glyphTable.i8(g.left);
    glyphTable.i8(g.top);
    glyphTable.u16(g.adv);
    glyphTable.u16(0);
    bitmaps.d.insert(bitmaps.d.end(), bits.begin(), bits.end());
  }

  Writer kernTable;
  for (const KernSpec& k : kerns) {
    kernTable.u32(k.left);
    kernTable.u32(k.right);
    kernTable.i16(k.value);
  }

  const uint32_t bitmapBase = static_cast<uint32_t>(
      kHeaderSize + glyphTable.d.size() + kernTable.d.size());
  Writer out;
  out.d = {'T', 'N', 'F', '1'};
  out.u8(bpp);
  out.u8(ascent);
  out.u8(descent);
  out.u8(gap);
  out.u16(static_cast<unsigned>(glyphs.size()));
  out.u16(0);  // reserved
  out.u32(static_cast<uint32_t>(kerns.size()));
  out.u32(fallback);
  out.u32(bitmapBase);
  out.d.insert(out.d.end(), glyphTable.d.begin(), glyphTable.d.end());
  out.d.insert(out.d.end(), kernTable.d.begin(), kernTable.d.end());
  out.d.insert(out.d.end(), bitmaps.d.begin(), bitmaps.d.end());
  return out.d;
}

// Test font: 2 bpp, ascent 8, descent 2, line gap 1, fallback '?'.
//   ' ' adv 4 (empty), '?' 2x2 adv 5, 'A' 3x3 adv 6, 'V' 3x3 adv 6,
//   'é' 2x1 adv 3 (levels 1 and 2), kern A-V = -2.
std::vector<uint8_t> testFont() {
  std::vector<GlyphSpec> glyphs = {
      {32, 0, 0, 0, 0, 4, {}},
      {63, 2, 2, 0, 2, 5, {3, 3, 3, 0}},
      {65, 3, 3, 0, 3, 6, {0, 3, 0, 3, 0, 3, 3, 3, 3}},
      {86, 3, 3, 0, 3, 6, {3, 0, 3, 3, 0, 3, 0, 3, 0}},
      {233, 2, 1, 0, 2, 3, {1, 2}},
  };
  std::vector<KernSpec> kerns = {{65, 86, -2}};
  return buildFont(2, 8, 2, 1, 63, glyphs, kerns);
}

// Records every pixel written by the renderer.
struct Recorder : public FontDisplay {
  std::map<std::pair<int, int>, int> pixels;
  void drawPixel(int x, int y, uint8_t coverage) override {
    pixels[std::make_pair(x, y)] = coverage;
  }
  int count() const { return static_cast<int>(pixels.size()); }
  bool at(int x, int y) const { return pixels.count(std::make_pair(x, y)) != 0; }
};

}  // namespace

// ---- Tests -----------------------------------------------------------------

static void testFormat() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  CHECK(font.valid());
  CHECK(!Font().valid());
  CHECK_EQ(font.bitsPerPixel(), 2);
  CHECK_EQ(font.ascent(), 8);
  CHECK_EQ(font.descent(), 2);
  CHECK_EQ(font.lineGap(), 1);
  CHECK_EQ(font.lineHeight(), 11);
  CHECK_EQ(font.glyphCount(), 5);
  CHECK_EQ(font.kerningCount(), 1);
  CHECK_EQ(font.fallbackCodepoint(), 63);

  Glyph g;
  CHECK(findGlyph(font, 'A', g));
  CHECK_EQ(g.width, 3);
  CHECK_EQ(g.height, 3);
  CHECK_EQ(g.top, 3);
  CHECK_EQ(g.advance, 6);

  CHECK(findGlyph(font, 'Z', g));  // missing: falls back to '?'
  CHECK_EQ(g.codepoint, 63);

  CHECK(!findGlyph(Font(), 'A', g));
}

static void testKerning() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  CHECK_EQ(kerning(font, 'A', 'V'), -2);
  CHECK_EQ(kerning(font, 'V', 'A'), 0);
  CHECK_EQ(kerning(font, 'A', 'A'), 0);
}

static void testUtf8() {
  const char* s;
  const char* e;

  s = "\xC3\xA9";  // é
  e = s + 2;
  CHECK_EQ(decodeUtf8(s, e), 0xE9);
  CHECK(s == e);

  s = "\xE2\x82\xAC";  // €
  e = s + 3;
  CHECK_EQ(decodeUtf8(s, e), 0x20AC);

  s = "\xF0\x9F\x98\x80";  // U+1F600
  e = s + 4;
  CHECK_EQ(decodeUtf8(s, e), 0x1F600);

  s = "\xC3";  // truncated
  e = s + 1;
  CHECK_EQ(decodeUtf8(s, e), kReplacementChar);

  s = "\xC0\x80";  // overlong
  e = s + 2;
  CHECK_EQ(decodeUtf8(s, e), kReplacementChar);

  s = "\xED\xA0\x80";  // surrogate
  e = s + 3;
  CHECK_EQ(decodeUtf8(s, e), kReplacementChar);
}

static void testMeasure() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  const char* s = "AV";
  CHECK_EQ(measureText(font, s, s + 2).width, 6 + 6 - 2);
  s = "AVA";
  CHECK_EQ(measureText(font, s, s + 3).width, 6 + 6 - 2 + 6);
  s = "A\nAV";
  TextBox box = measureText(font, s, s + 4);
  CHECK_EQ(box.lines, 2);
  CHECK_EQ(box.width, 10);
  CHECK_EQ(box.height, 22);
}

static void testWrap() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  const char* s = "A A A A";
  std::vector<std::string> lines;
  std::vector<int> widths;
  int n = wrapText(font, s, s + 7, 16,
                   [&](const char* b, const char* e, int w) {
                     lines.push_back(std::string(b, e));
                     widths.push_back(w);
                   });
  CHECK_EQ(n, 2);
  CHECK(lines.size() == 2 && lines[0] == "A A" && lines[1] == "A A");
  CHECK(widths.size() == 2 && widths[0] == 16 && widths[1] == 16);

  // A single word wider than the limit is split between characters.
  s = "AAA";
  lines.clear();
  n = wrapText(font, s, s + 3, 8,
               [&](const char* b, const char* e, int) {
                 lines.push_back(std::string(b, e));
               });
  CHECK_EQ(n, 3);
}

static void testUtf8Text() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());

  // Wrapping must never split a multi-byte sequence: "é é é" with 'é' 3 px,
  // space 4 px and maxWidth 10 gives the lines "é é" and "é".
  const char* s = "\xC3\xA9 \xC3\xA9 \xC3\xA9";
  std::vector<std::string> lines;
  wrapText(font, s, s + 8, 10, [&](const char* b, const char* e, int) {
    lines.push_back(std::string(b, e));
  });
  CHECK(lines.size() == 2);
  CHECK(lines.size() == 2 && lines[0] == "\xC3\xA9 \xC3\xA9");
  CHECK(lines.size() == 2 && lines[1] == "\xC3\xA9");

  // '€' (U+20AC) is not in the font: it is measured with the fallback '?'
  // glyph (advance 5), exactly like an unknown ASCII character.
  const char* euro = "\xE2\x82\xAC";
  CHECK_EQ(measureText(font, euro, euro + 3).width, 5);

  // A four-byte character (U+1F600) also uses the fallback glyph.
  const char* emoji = "\xF0\x9F\x98\x80";
  CHECK_EQ(measureText(font, emoji, emoji + 4).width, 5);

  // Mixed text: 'A' (6) + '€' fallback (5) + 'é' (3).
  const char* mixed = "A\xE2\x82\xAC\xC3\xA9";
  CHECK_EQ(measureText(font, mixed, mixed + 6).width, 6 + 5 + 3);
}

static void testRenderGlyph() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  Recorder rec;
  TextRenderer text(font, rec);

  // Baseline = 20 + ascent(8) = 28; 'A' top row = 28 - 3 = 25; x starts at 10.
  text.drawLine(10, 20, "A", "A" + 1);
  CHECK(rec.at(11, 25));
  CHECK(!rec.at(10, 25));
  CHECK_EQ(rec.pixels[std::make_pair(11, 25)], 255);
  CHECK(rec.at(10, 26) && rec.at(12, 26));
  CHECK(rec.at(10, 27) && rec.at(11, 27) && rec.at(12, 27));
  CHECK_EQ(rec.count(), 6);

  // Two coverage levels (1 of 3 and 2 of 3) map to 85 and 170.
  Recorder rec2;
  TextRenderer text2(font, rec2);
  const char* accent = "\xC3\xA9";
  text2.drawLine(0, 0, accent, accent + 2);
  CHECK_EQ(rec2.pixels[std::make_pair(0, 6)], 85);
  CHECK_EQ(rec2.pixels[std::make_pair(1, 6)], 170);
}

static void testKerningRender() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  Recorder rec;
  TextRenderer text(font, rec);
  // "AV" at (0,0): baseline 8, top row 5. V starts at pen 6 - 2 = 4.
  text.drawText(0, 0, "AV");
  CHECK(rec.at(4, 5));
  CHECK(rec.at(6, 5));
  CHECK(!rec.at(5, 5));
}

static void testClip() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  Recorder rec;
  TextRenderer text(font, rec);
  text.setClip(10, 0, 12, 100);
  text.drawLine(10, 20, "A", "A" + 1);
  CHECK_EQ(rec.count(), 4);
  CHECK(!rec.at(12, 26));
}

static void testDrawWrapped() {
  std::vector<uint8_t> blob = testFont();
  Font font(blob.data());
  Recorder rec;
  TextRenderer text(font, rec);
  TextBox box = text.drawText(0, 0, "A A A A", 16);
  CHECK_EQ(box.lines, 2);
  CHECK_EQ(box.height, 22);
  // Second line: top 11, baseline 19, 'A' top row 16; first row has pixel at x=1.
  CHECK(rec.at(1, 16));
}

static void testOneBitFont() {
  std::vector<GlyphSpec> glyphs = {
      {65, 2, 2, 0, 2, 3, {1, 0, 0, 1}},
  };
  std::vector<uint8_t> blob = buildFont(1, 2, 0, 0, 65, glyphs, {});
  Font font(blob.data());
  Recorder rec;
  TextRenderer text(font, rec);
  text.drawLine(0, 0, "A", "A" + 1);
  CHECK_EQ(rec.count(), 2);
  CHECK_EQ(rec.pixels[std::make_pair(0, 0)], 255);
  CHECK_EQ(rec.pixels[std::make_pair(1, 1)], 255);
}

int main() {
  testFormat();
  testKerning();
  testUtf8();
  testMeasure();
  testWrap();
  testUtf8Text();
  testRenderGlyph();
  testKerningRender();
  testClip();
  testDrawWrapped();
  testOneBitFont();

  if (failures) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
