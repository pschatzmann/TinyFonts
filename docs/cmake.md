# CMake

TinyFonts is a header-only library. The CMake project provides an
`INTERFACE` target, `TinyFonts::TinyFonts`, plus install rules and a package
config file.

## Using TinyFonts in your CMake project

After installing (see below):

```cmake
find_package(TinyFonts 1 REQUIRED)
target_link_libraries(my_app PRIVATE TinyFonts::TinyFonts)
```

Or add the repository as a subdirectory:

```cmake
add_subdirectory(TinyFonts)
target_link_libraries(my_app PRIVATE TinyFonts::TinyFonts)
```

## Building and testing

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The host tests build a small font in memory from the documented format. They
check parsing, kerning, UTF-8 decoding, measuring, wrapping, rendering,
clipping, 1-bit output and UTF-8 text. Pass `-DTINYFONTS_BUILD_TESTS=OFF` to
skip them.

## Installing

```sh
cmake --install build --prefix /some/prefix
```

This installs the headers under `include/` and the package config under
`lib/cmake/TinyFonts/`.

## Desktop examples

The desktop examples need Arduino-Emulator, which provides `setup()`, `loop()`
and the Arduino API for desktop builds. Enable them with
`-DTINYFONTS_BUILD_EXAMPLES=ON`:

| Variable | Purpose |
|---|---|
| `TINYFONTS_BUILD_EXAMPLES` | build the desktop examples (default OFF) |
| `TINYFONTS_TINYGPU_DIR` | path to a TinyGPU checkout (default `../TinyGPU`) |
| `FETCHCONTENT_SOURCE_DIR_ARDUINO_EMULATOR` | use an existing Arduino-Emulator checkout instead of downloading it |

See [tinygpu.md](tinygpu.md) for the TinyGPU example.
