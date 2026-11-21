# Desktop build of the TinyGPU example (opens an SDL2 window).
find_package(SDL2 REQUIRED)

arduino_sketch(tinyfonts-tinygpu tinygpu.ino LIBRARIES TinyFonts TinyGPU SDL2::SDL2)
target_compile_definitions(tinyfonts-tinygpu PRIVATE TINYGPU_AUTO_NAMESPACE)
