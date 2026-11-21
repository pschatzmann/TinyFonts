// TinyFonts - platform configuration.
#pragma once

#if defined(__AVR__)
#  include <avr/pgmspace.h>
#  define TF_HAS_PGM 1
#elif defined(ESP8266)
#  include <pgmspace.h>
#  define TF_HAS_PGM 1
#else
#  define TF_HAS_PGM 0
#  ifndef PROGMEM
#    define PROGMEM
#  endif
#endif
