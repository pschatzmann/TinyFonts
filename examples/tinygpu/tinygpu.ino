/**
 * @file tinygpu.ino
 * @brief TinyFonts used through TinyGPU's own font API - the same source runs
 * on an ESP32 display board and on the SDL2 desktop backend.
 *
 * Each generated TinyFonts font is wrapped in a TinyGPUFont, which
 * implements TinyGPU's IFont interface. After setFont(), the framebuffer's
 * drawText() renders TinyFonts text (with alpha blending and clipping), just
 * like TinyGPU's own bitmap fonts. drawWrapped() adds word wrapping.
 *
 * Desktop: built by the CMake target `tinyfonts-tinygpu` (opens an SDL2
 * window). ESP32: compile with the arduino-esp32 core; the board class below
 * matches the ESP32 Cheap Yellow Display used by TinyGPU's examples.
 */
#include <Arduino.h>
// TinyGPU's ESP32 board and touch headers use SPI and Wire without including
// them; include them first so the ESP32 build compiles with current cores.
#include <SPI.h>
#include <Wire.h>
#include <TinyGPU.h>
#include <TinyGPU/Boards/LCDBoards.h>
// Including the umbrella header first lets the Arduino toolchain find the library.
#include <TinyFonts.h>
#include <TinyFonts/Integrations/TinyGPU.h>

// All bundled fonts (Lato, Arial- and Helvetica-compatible). Each one is a
// Font object named after its file, e.g. Lato16 or Arial16.
#include <TinyFonts/Fonts/Fonts.h>

#ifdef ESP32
LCDBoardGuitionESP32_LVGL_2_4Display board;
#else
LCDBoardDesktopSDL board(240, 320);
#endif

FrameBuffer<RGB565> framebuffer(board.width(), board.height(), FontRGB565);

// One TinyGPU font object per TinyFonts font. setFont() keeps a reference, so
// these must outlive the framebuffer's use of them (globals do).
TinyGPUFont<RGB565> title(Lato32);
TinyGPUFont<RGB565> body(Lato16);
TinyGPUFont<RGB565> small10(Lato10);
TinyGPUFont<RGB565> small8(Lato8);
TinyGPUFont<RGB565> arial16(Arial16);
TinyGPUFont<RGB565> helvetica16(Helvetica16);

const char* kHeading = "TinyFonts";
// UTF-8 text: multi-byte characters (ü, é, ï, €) come from the Lato16 font,
// which was generated with those character sets.
const char* kParagraph =
    "Grüße from café! Naïve text: €5. Lines wrap at the display "
    "edge, and kerning tightens pairs such as AV, To and Wa.";
const char* kMoving = "AVATAR To Wave";
const char* kSmallSample = "10 and 8 px: Grüße from café! €5";

int movingSpan = 0;  // horizontal travel of the moving line, in pixels

void drawFrame(uint32_t elapsedMs) {
  framebuffer.clear(RGB565::fromRGB(0, 0, 0));
  const int width = static_cast<int>(board.width());

  framebuffer.setFont(title);
  framebuffer.drawText(8, 8, kHeading, RGB565::fromRGB(255, 200, 0));

  body.drawWrapped(framebuffer, 8, 60, width - 16, kParagraph,
                   RGB565::fromRGB(120, 200, 255));

  // The same sample line in the two small sizes.
  const RGB565 gray = RGB565::fromRGB(180, 180, 180);
  framebuffer.setFont(small10);
  framebuffer.drawText(8, 170, kSmallSample, gray);
  framebuffer.setFont(small8);
  framebuffer.drawText(8, 186, kSmallSample, gray);

  // Arial-compatible and Helvetica-compatible faces at 16 px.
  const RGB565 white = RGB565::fromRGB(255, 255, 255);
  framebuffer.setFont(arial16);
  framebuffer.drawText(8, 202, "Arial 16 px: Grüße AVATAR", white);
  framebuffer.setFont(helvetica16);
  framebuffer.drawText(8, 220, "Helvetica 16 px: Grüße AVATAR", white);

  // Bounce the moving line between the left and right margins.
  const int period = 2 * movingSpan;
  int offset = period > 0 ? static_cast<int>((elapsedMs / 25) % period) : 0;
  if (offset > movingSpan) offset = period - offset;
  framebuffer.setFont(body);
  framebuffer.drawText(8 + offset, 240, kMoving, white);

  board.display().writeData(framebuffer);
}

void setup() {
  Serial.begin(115200);
  board.begin();
  framebuffer.begin();

  const int width = static_cast<int>(board.width());
  const size_t textWidth = body.measureTextWidth(kMoving);
  movingSpan = width - 16 - static_cast<int>(textWidth);
  if (movingSpan < 0) movingSpan = 0;

  const TextBox box = body.measureWrapped(kParagraph, width - 16);
  Serial.print("paragraph lines: ");
  Serial.println(box.lines);
}

void loop() {
  static const uint32_t start = millis();
  drawFrame(millis() - start);
  delay(30);
}
