# Available fonts

The fonts below are generated with `tools/tinyfonts_convert.py` and ship with the
library in `src/TinyFonts/Fonts/`. Include them with `#include <TinyFonts/Fonts/Lato16.h>`. To include all of them, use `#include <TinyFonts/Fonts/Fonts.h>`. Unused fonts add nothing to the compiled sketch. Each font is one header file that defines a `tinyfonts::Font`
object with the same name as the file. All use 2 bits per pixel.

| Font | Size | Characters | Kerning | Header | Blob size |
|---|---|---|---|---|---|
| `Lato8` | 8 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Lato8.h` | 16 KB |
| `Lato10` | 10 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Lato10.h` | 34 KB |
| `Lato12` | 12 px | ascii | yes | `src/TinyFonts/Fonts/Lato12.h` | 11 KB |
| `Lato16` | 16 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Lato16.h` | 45 KB |
| `Lato32` | 32 px | ascii | yes | `src/TinyFonts/Fonts/Lato32.h` | 31 KB |
| `Arial8` | 8 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Arial8.h` | 5 KB |
| `Arial10` | 10 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Arial10.h` | 6 KB |
| `Arial16` | 16 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Arial16.h` | 10 KB |
| `Arial32` | 32 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Arial32.h` | 25 KB |
| `Helvetica8` | 8 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Helvetica8.h` | 7 KB |
| `Helvetica10` | 10 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Helvetica10.h` | 8 KB |
| `Helvetica16` | 16 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Helvetica16.h` | 14 KB |
| `Helvetica32` | 32 px | ascii, latin1, euro | yes | `src/TinyFonts/Fonts/Helvetica32.h` | 34 KB |

## Licences

- **Lato** is licensed under the SIL Open Font License 1.1.
- **Arial** entries are generated from **Liberation Sans**, which is
  metric-compatible with Arial and licensed under the SIL Open Font License
  1.1. They are not Microsoft's Arial.
- **Helvetica** entries are generated from **Nimbus Sans** (URW base35), an
  open Helvetica-compatible font. Check its licence terms, which include a
  font exception, before you ship the generated data. They are not Linotype's
  Helvetica.

Check the licence of any font you convert before you ship the output.

## Regenerating

```sh
python3 tools/tinyfonts_convert.py --font /path/to/Font.ttf --size 16 --bpp 2 \
    --chars ascii,latin1,euro --name MyFont16 --out MyFont16.h
```
