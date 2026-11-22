# TinyFonts

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![CMake](https://img.shields.io/badge/CMake-Supported-blue.svg)](https://github.com/pschatzmann/TinyFonts/blob/main/CMakeLists.txt)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://github.com/pschatzmann/TinyFonts/blob/main/LICENSE)

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
converter is a Python tool; see [Font converter](https://github.com/pschatzmann/TinyFonts/wiki/Font-Converter) for its
requirements.

## Documentation

- [Class documentation](https://pschatzmann.github.io/TinyFonts/annotated.html): Doxygen API reference for all classes and functions
- [Quick start](https://github.com/pschatzmann/TinyFonts/wiki/Quick-Start): from a TTF file to text on screen
- [Getting started](https://github.com/pschatzmann/TinyFonts/wiki/Getting-Started): the runtime API, measuring, clipping and limits
- [Font converter](https://github.com/pschatzmann/TinyFonts/wiki/Font-Converter): options, character sets, sizes and kerning
- [Available fonts](https://github.com/pschatzmann/TinyFonts/wiki/Available-Fonts): the generated fonts, sizes and licences
- [TinyGPU](https://github.com/pschatzmann/TinyFonts/wiki/TinyGPU): the desktop and ESP32 example and the integration header
- [CMake](https://github.com/pschatzmann/TinyFonts/wiki/CMake): using, installing, testing and the desktop example build
- [Font format](https://github.com/pschatzmann/TinyFonts/wiki/Font-Format): the binary layout of a generated font

## License

TinyFonts is licensed under the Apache License 2.0. See [LICENSE](LICENSE).

The generated fonts have their own licences; see [Available fonts](https://github.com/pschatzmann/TinyFonts/wiki/Available-Fonts).
