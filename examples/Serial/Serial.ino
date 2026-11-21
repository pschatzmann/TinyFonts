// TinyFonts: renders text into a small coverage canvas and prints it to the
// Serial Monitor as ASCII shading. Needs no display, so it runs on any board.
//
// Lato12.h was generated with tools/tinyfonts_convert.py:
//   --font Lato-Regular.ttf --size 12 --bpp 2 --name Lato12
//   --out src/TinyFonts/Fonts/Lato12.h

#include <string.h>
#include <TinyFonts.h>
#include <TinyFonts/Fonts/Lato12.h>

constexpr int kWidth = 80;
constexpr int kHeight = 16;

// Coverage canvas: 0 = background, 255 = fully covered.
static uint8_t canvas[kHeight][kWidth];

class CanvasDisplay : public tinyfonts::FontDisplay {
public:
  void drawPixel(int x, int y, uint8_t coverage) override {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) return;
    // Simple "max" blend: overlapping glyphs keep the stronger coverage.
    if (coverage > canvas[y][x]) canvas[y][x] = coverage;
  }
};

CanvasDisplay display;
tinyfonts::TextRenderer text(Lato12, display);

void printCanvas() {
  static const char shades[] = " .:-=+*#%@";
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      Serial.print(shades[canvas[y][x] * 9 / 255]);
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
  }

  memset(canvas, 0, sizeof(canvas));
  const char* message = "AVATAR Hello!";
  tinyfonts::TextBox box = text.drawText(0, 0, message);
  printCanvas();

  Serial.print("Text width: ");
  Serial.println(box.width);
}

void loop() {
}
