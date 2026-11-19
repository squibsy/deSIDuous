# Third-party notices

deSIDuous is free software. It is distributed under the **GNU Affero General Public License, version 3**
(see `LICENSE`). Copyright (C) 2026 Ivan O'Donoghue.

It contains or is built from the third-party software listed below. Their licence texts are in the `licenses`
folder of the download and in the source archive.

## SID chip emulation: reSIDfp

- **Component:** reSIDfp, from the libsidplayfp project (`libs/libsidplayfp`, upstream
  https://github.com/libsidplayfp/libsidplayfp). The files built into deSIDuous are under
  `src/builders/residfp-builder/residfp`.
- **Licence:** GNU General Public License, version 2 or (at your option) any later version.
  Full text: `licenses/GPL-2.0.txt` (the `COPYING` file of libsidplayfp).
- **Copyright:** (C) 2011-2025 Leandro Nini; (C) 2007-2010 Antti Lankila; (C) 2004 Dag Lem.
  libsidplayfp also credits Simon White (2000) and Michael Schwendt. See `libs/libsidplayfp/AUTHORS`.
- **How the licences fit:** a GPL "version 2 or later" work may be used under version 3 of the GPL, and
  section 13 of the GPL version 3 and of the AGPL version 3 allow a work under one to be combined with a work
  under the other. deSIDuous is therefore distributed as a whole under the AGPL version 3.

## Plugin framework: JUCE

- **Component:** JUCE 8.0.15 (`libs/JUCE`, upstream https://github.com/juce-framework/JUCE).
- **Licence:** used under its AGPLv3 option (JUCE is dual-licensed under the AGPLv3 and a commercial licence).
  Full text: `licenses/AGPL-3.0.txt` (the `LICENSE` file of this project) and `licenses/JUCE-LICENSE.md`.
- **Copyright:** (C) Raw Material Software Limited.

### Components bundled inside JUCE

These ship inside the JUCE modules that deSIDuous uses. Which of them end up in your copy depends on the build,
so check this list against the binary you ship before publishing.

| Component | Licence | Licence file (in `libs/JUCE/modules/...`) |
|---|---|---|
| zlib | zlib | `juce_core/zip/zlib/README` |
| libpng | zlib | `juce_graphics/image_formats/pnglib/LICENSE` |
| IJG JPEG library | Independent JPEG Group licence | `juce_graphics/image_formats/jpglib/README` |
| HarfBuzz | Old MIT | `juce_graphics/fonts/harfbuzz/COPYING` |
| SheenBidi | Apache 2.0 | `juce_graphics/unicode/sheenbidi/LICENSE` |
| FLAC | BSD | `juce_audio_formats/codecs/flac/Flac Licence.txt` |
| Ogg Vorbis | BSD | `juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt` |

## VST3 interface: Steinberg VST3 SDK

- **Component:** VST3 SDK headers, included in JUCE
  (`juce_audio_processors_headless/format_types/VST3_SDK`).
- **Licence:** MIT.

```
MIT License

Copyright (c) 2025, Steinberg Media Technologies GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Trademarks

- VST is a trademark of Steinberg Media Technologies GmbH.
- Commodore, Commodore 64 and MOS Technology are trademarks of their respective owners. They are used here only to
  describe the SID sound chip that deSIDuous emulates. deSIDuous is not affiliated with or endorsed by them.
- deSIDuous™ and its logo are trademarks of Ivan O'Donoghue. The licence of the source code does not give permission
  to use the name or logo for modified versions, which must carry a different name.

## Source code

The complete source code for every released version, including the build files, is available from
https://github.com/squibsy/deSIDuous (tags `v1.xx`) and is included as a ZIP with each download.
You may study, change and share it under the AGPLv3.
