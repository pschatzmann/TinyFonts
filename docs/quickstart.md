# Quick start

This page takes you from a TTF file to text on screen in two steps. Installation
is covered in the [README](../README.md#installation).

## 1. Generate a font header

```sh
python3 tools/tinyfonts_convert.py --font Lato-Regular.ttf --size 16 \
    --bpp 2 --chars ascii,latin1,euro --name Lato16 --out Lato16.h
```

The header defines a `tinyfonts::Font` object called `Lato16`. See
[converter.md](converter.md) for all options.

## 2. Draw text

```cpp
#include <TinyFonts.h>
#include <TinyFonts/Fonts/Lato16.h>

class MyDisplay : public tinyfonts::FontDisplay {
 public:
  void drawPixel(int x, int y, uint8_t coverage) override {
    // blend `coverage` (0..255) with the background and write the pixel
  }
};

MyDisplay display;
tinyfonts::TextRenderer text(Lato16, display);
text.drawText(0, 0, "Grüße, TinyFonts!");
text.drawText(0, 40, "A longer paragraph that wraps", 120);
```

`drawPixel` receives one call per pixel. `coverage` is 0 for no ink and 255 for
full ink. Blend it with what is already on the display.

## Next steps

- [Getting started](getting-started.md): measuring, clipping and the other helpers
- Complete examples: [examples/Serial/](../examples/Serial/) prints the text as
  ASCII art, and [examples/tinygpu/](../examples/tinygpu/) draws on a TinyGPU
  display
