# TinyFonts

Header-only font rendering for Arduino and plain C++. Fonts are converted on
your development machine into compact byte arrays. The device-side renderer
needs no heap and no display library.

## Features

- Anti-aliasing at 1, 2, 4 or 8 bits per pixel
- Kerning, proportional advances, word wrap and UTF-8 text
- Fonts in flash (PROGMEM) or RAM
- 13 ready-to-use fonts in `src/TinyFonts/Fonts/` (Lato, Arial- and Helvetica-compatible)
- Draws through a small `FontDisplay` interface, so any display library works
- Optional integration with [TinyGPU](https://github.com/pschatzmann/TinyGPU)

## Installation

### Arduino

Download the library as a zip and use **Sketch → Include Library → Add .ZIP
Library**. Or clone the project into your Arduino libraries folder:

```
cd ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/TinyFonts.git
```

Using git makes updates easy: run `git pull` in the project folder. The font
converter is a Python tool; see [docs/converter.md](docs/converter.md) for its
requirements.

## Documentation

- [Quick start](docs/quickstart.md): from a TTF file to text on screen
- [Getting started](docs/getting-started.md): the runtime API, measuring, clipping and limits
- [Font converter](docs/converter.md): options, character sets, sizes and kerning
- [Available fonts](docs/fonts.md): the generated fonts, sizes and licences
- [TinyGPU](docs/tinygpu.md): the desktop and ESP32 example and the integration header
- [CMake](docs/cmake.md): using, installing, testing and the desktop example build
- [Font format](docs/format.md): the binary layout of a generated font

## License

TinyFonts is licensed under the Apache License 2.0. See [LICENSE](LICENSE).

The generated fonts have their own licences; see [docs/fonts.md](docs/fonts.md).
