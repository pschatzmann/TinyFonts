// TinyFonts - header-only font rendering for Arduino and plain C++.
//
// Include this single header. Fonts are generated with
// tools/tinyfonts_convert.py and are plain byte arrays, so they can live in
// flash (PROGMEM) or RAM. See README.md for details.
#pragma once

/**
 * @namespace tinyfonts
 * @brief Font rendering: Font, TextRenderer, FontDisplay, UTF-8 and layout helpers.
 */

#include "TinyFonts/config.h"
#include "TinyFonts/format.h"
#include "TinyFonts/utf8.h"
#include "TinyFonts/layout.h"
#include "TinyFonts/render.h"

// Make the tinyfonts namespace visible, so fonts can be used as Lato16 etc.
// Define TINYFONTS_NO_USING_NAMESPACE before including this header to avoid it
// (then write tinyfonts::Lato16).
#ifndef TINYFONTS_NO_USING_NAMESPACE
using namespace tinyfonts;
#endif
