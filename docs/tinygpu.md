# TinyGPU

TinyFonts works with [TinyGPU](https://github.com/pschatzmann/TinyGPU) in two
ways: an example sketch that runs on desktop and ESP32, and an optional
integration header.

## Example: `examples/tinygpu/tinygpu.ino`

The sketch draws several TinyFonts fonts into a TinyGPU framebuffer. It uses
the same source on an ESP32 display board and on the SDL2 desktop backend.
Desktop build:

```sh
cmake -S . -B build -DTINYFONTS_BUILD_EXAMPLES=ON \
      -DTINYFONTS_TINYGPU_DIR=/path/to/TinyGPU
cmake --build build --target tinyfonts-tinygpu
./build/examples/tinygpu/tinyfonts-tinygpu
```

See [cmake.md](cmake.md) for the other options. The ESP32 build compiles with
the arduino-esp32 core, but TinyGPU's headers currently fail to compile on
arduino-esp32 4.0.0, so that build is not yet verified.

## Integration: `TinyFonts/Integrations/TinyGPU.h`

TinyFonts does not depend on TinyGPU. Include this header only in projects
that use it:

```cpp
#include <TinyGPU.h>
#include <TinyFonts/Integrations/TinyGPU.h>
#include <TinyFonts/Fonts/Lato16.h>

tinyfonts::TinyGPUFont<RGB565> lato16(Lato16);   // a tinygpu::IFont
framebuffer.setFont(lato16);
framebuffer.drawText(8, 8, "Grüße", RGB565::fromRGB(255, 255, 0));
lato16.drawWrapped(framebuffer, 8, 40, 200, "Word wrapping",
                   RGB565::fromRGB(255, 255, 255));
```

`TinyGPUFont` implements TinyGPU's `IFont`, so any TinyGPU surface (Surface,
FrameBuffer, sprites) can use it through `setFont()` and `drawText()`.

- Text is alpha-blended over the existing pixels. With `opaque` the text box is
  filled with the background colour first.
- The clip rectangle (`pushClipRect`) is respected.
- `scale` makes each font pixel a `scale` x `scale` block.
- `spacing` and `lineSpacing` are accepted but ignored. TinyFonts uses each
  glyph's own advance and the font's line gap.
- Fonts are always proportional, so `setProportional()` does nothing.
- `drawWrapped()` and `measureWrapped()` add word wrapping. TinyGPU's
  `IFont::drawText` has no width parameter.
- `TinyGPUDisplay` is the blending adapter used internally. It can also be used
  directly with `tinyfonts::TextRenderer`.

The colour helper uses `getRed`, `getGreen`, `getBlue` and `fromRGB`, so other
TinyGPU colour types should work. Only `RGB565` is tested.
