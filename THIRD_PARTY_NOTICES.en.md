# Third-party licenses and provenance

[简体中文](THIRD_PARTY_NOTICES.md) · [English](THIRD_PARTY_NOTICES.en.md)

Project-owned M1n1Le55 code is licensed under the root [MIT License](LICENSE), Copyright (c) 2026 M1n1Le55 contributors. Third-party code, fonts, generated glyphs, marks and linked libraries retain their own terms. MIT does not replace or expand those permissions.

These notices cover **0.31 / firmware-grid-1**. Dependency pins are in the source distribution's `dependencies.lock.template` and build configuration. The [license-source index](licenses/SOURCE_INDEX.md) preserves the copied upstream texts, provenance and SHA-256 hashes. Archive membership does not mean every feature is used, and this inventory is not an instruction-by-instruction legal analysis of the binary.

## Acknowledgments and upstream projects

Thank you to the authors and contributors whose hardware support, open-source components, fonts and FM-1 research make M1n1Le55 possible:

- **Hardware and UI:** [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), [ESP-IDF 5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2), [Tab5 BSP 1.3.2](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2), [ESP-Hosted 2.12.9](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9), [LVGL 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1).
- **Sound and decoding references:** [Ben Sparrow / FM-1 editor](https://github.com/benny-sparra/fm1-dx7-patch-importer), [Dani Biro / SEA codec](https://github.com/Daninet/sea-codec), [M-VAVE / FM1 Open](https://github.com/m-vave/fm1_open).
- **Fonts and glyphs:** [Adobe / Source Han Sans](https://github.com/adobe-fonts/source-han-sans), [Viznut / Unscii](https://viznut.fi/unscii/), [Montserrat](https://github.com/JulietaUla/Montserrat), [Font Awesome](https://fontawesome.com/).
- **Runtime and installation tools:** [GCC](https://gcc.gnu.org/), [Newlib](https://sourceware.org/newlib/), [Python](https://www.python.org/), [esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0). Python and esptool are installed separately.

Thanks also to the FM-1 firmware authors: [M-VAVE](https://www.cuvave.com/download), [hugelton / Felucca](https://github.com/hugelton/Felucca), [isod89 / SLOOP](https://github.com/isod89/sloop-fm1), [Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install), [Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x), [zednaked / Jangada](https://github.com/zednaked/jangada), [FM1 Open](https://github.com/m-vave/fm1_open). Obtain these third-party FM-1 firmware releases from their original publishers and use the compatible versions listed for this release.

See the [complete third-party inventory](#linked-libraries-and-runtime) for lower-level components, pinned versions, usage and licensing. The bundle also includes Chinese/English notices and original license texts. All projects retain their original copyrights and terms; acknowledgment does not imply endorsement.

## Firmware icons

SLOOP's publisher SVG retains its GPL-3.0 terms. The preferred-form [source SVG](licenses/firmware-icons/sloop.svg), [license](licenses/firmware-icons/SLOOP-GPL-3.0.txt) and [artwork notices](licenses/firmware-icons/NOTICES.md) accompany both source and installation distributions. M-VAVE and Baud Girl marks remain their publishers' property; no separate MIT grant was located. Other pixel icons are original M1n1Le55 artwork under MIT. The minimal distribution keeps runtime data and required SLOOP source, and omits redundant image downloads and generators. No publisher endorsement is implied.

## Third-party content in project source

| Content | Origin and terms | Preserved text |
| --- | --- | --- |
| Algorithm diagrams in `main/fm1_algorithms.h` | Ben Sparrow, 2026, MIT; [FM-1 editor/importer](https://github.com/benny-sparra/fm1-dx7-patch-importer) | [MIT notice](licenses/fm1-editor-MIT.txt) |
| SEA decoding/tables in `main/m1_waveform.c` and `main/m1_sea_table.inc` | [SEA codec](https://github.com/Daninet/sea-codec), Dani Biro, 2025; M-VAVE FM1 adaptation, 2026. The selected material is explicitly MIT; reference implementation from [FM1 Open V12](https://github.com/m-vave/fm1_open/releases/tag/V12) | [SEA notice](licenses/sea-waveform.txt) |
| `m1_font_20/24/28` glyphs in `main/m1_fonts.c` | [Source Han Sans](https://github.com/adobe-fonts/source-han-sans), Adobe, 2014–2021, OFL 1.1; subset font data retains OFL and uses project font identifiers | [OFL text](licenses/m1n1le55-fonts/SourceHanSansSC.txt) |
| `m1_pixel_32/48` glyphs | [Unscii](https://viznut.fi/unscii/), public-domain `unscii-8.ttf` variant; not the GNU Unifont-containing `unscii-16-full` | [Upstream text](licenses/m1n1le55-fonts/unscii.html) |

Source font SHA-256 hashes: `SourceHanSansSC-Normal.otf`: `1ee89e1669362dee13851129c0a8a791a87521eb4148e5efbf5d26596738e25b`; `unscii-8.ttf`: `1be7ca5552beb67f6bf1bed4a0db92a53835f1c07cb0f643063d82ffb0dccba0`. Font data is not relicensed under project MIT. Historical research paths in `sea-waveform.txt` identify provenance; that research directory is not distributed.

## Linked libraries and runtime

The distribution retains license copies for ESP-IDF 5.4.2, LVGL 9.6.0~1, the pinned Espressif managed components, FreeRTOS, Newlib, lwIP, mbedTLS, NimBLE, TinyCrypt, protobuf-c, cJSON, SPIFFS, console components, TLSF, FatFs and the compiler runtime. The applicable terms are in each original text under [licenses/](licenses/), not uniformly MIT. In particular, the Espressif IPA/video licenses include hardware-specific terms, and GCC's runtime exception accompanies its GPL text. LVGL's third-party notices and built-in font licenses are retained as well.

### Lower-level component sources

Thank you to the maintainers of components integrated by ESP-IDF and LVGL. These links point to component directories in the SDK used by this release, or to their upstream projects. Usage and license scope remain as documented here and in the original texts.

[FreeRTOS](https://github.com/espressif/esp-idf/tree/v5.4.2/components/freertos), [lwIP](https://github.com/espressif/esp-idf/tree/v5.4.2/components/lwip), [cJSON](https://github.com/espressif/esp-idf/tree/v5.4.2/components/json), [Mbed TLS](https://github.com/espressif/esp-idf/tree/v5.4.2/components/mbedtls), [SPIFFS](https://github.com/espressif/esp-idf/tree/v5.4.2/components/spiffs), [NimBLE](https://github.com/espressif/esp-idf/tree/v5.4.2/components/bt/host/nimble), [TinyCrypt](https://github.com/espressif/esp-idf/tree/v5.4.2/components/bt/common/tinycrypt), [protobuf-c](https://github.com/espressif/esp-idf/tree/v5.4.2/components/protobuf-c), [linenoise / argtable3](https://github.com/espressif/esp-idf/tree/v5.4.2/components/console), [FatFs](https://github.com/espressif/esp-idf/tree/v5.4.2/components/fatfs), [TLSF](https://github.com/espressif/esp-idf/tree/v5.4.2/components/heap), [UBSAN](https://github.com/espressif/esp-idf/blob/v5.4.2/components/esp_system/ubsan.c). LVGL's built-in formatting implementation references [mpaland/printf](https://github.com/mpaland/printf); built-in font sources include [Montserrat](https://github.com/JulietaUla/Montserrat) and [Font Awesome](https://fontawesome.com/).

Pinned component versions, licensing and build-link observations:

| Component / source | Pinned version | Build-link observation | License |
| --- | --- | --- | --- |
| [`espressif/bmi270`](https://components.espressif.com/components/espressif/bmi270/versions/1.1.0) | `1.1.0` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__bmi270/license.txt) |
| [`espressif/cmake_utilities`](https://components.espressif.com/components/espressif/cmake_utilities/versions/0.5.3) | `0.5.3` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__cmake_utilities/license.txt) |
| [`espressif/eppp_link`](https://components.espressif.com/components/espressif/eppp_link/versions/1.1.6) | `1.1.6` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__eppp_link/LICENSE) |
| [`espressif/esp_cam_sensor`](https://components.espressif.com/components/espressif/esp_cam_sensor/versions/2.0.1) | `2.0.1` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_cam_sensor/LICENSE) |
| [`espressif/esp_codec_dev`](https://components.espressif.com/components/espressif/esp_codec_dev/versions/1.5.11) | `1.5.11` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_codec_dev/LICENSE) |
| [`espressif/esp_h264`](https://components.espressif.com/components/espressif/esp_h264/versions/1.0.4) | `1.0.4` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_h264/LICENSE) |
| [`espressif/esp_hosted`](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9) | `2.12.9` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_hosted/LICENSE) |
| [`espressif/esp_io_expander`](https://components.espressif.com/components/espressif/esp_io_expander/versions/1.2.1) | `1.2.1` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_io_expander/license.txt) |
| [`espressif/esp_io_expander_pi4ioe5v6408`](https://components.espressif.com/components/espressif/esp_io_expander_pi4ioe5v6408/versions/1.0.1) | `1.0.1` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_io_expander_pi4ioe5v6408/license.txt) |
| [`espressif/esp_ipa`](https://components.espressif.com/components/espressif/esp_ipa/versions/1.3.1) | `1.3.1` | Archive members linked in the application | Espressif hardware-specific license；[License text](licenses/managed_components/espressif__esp_ipa/LICENSE) |
| [`espressif/esp_lcd_ili9881c`](https://components.espressif.com/components/espressif/esp_lcd_ili9881c/versions/1.1.0) | `1.1.0` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lcd_ili9881c/license.txt) |
| [`espressif/esp_lcd_st7123`](https://components.espressif.com/components/espressif/esp_lcd_st7123/versions/1.0.2) | `1.0.2` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lcd_st7123/license.txt) |
| [`espressif/esp_lcd_touch`](https://components.espressif.com/components/espressif/esp_lcd_touch/versions/1.2.1) | `1.2.1` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lcd_touch/license.txt) |
| [`espressif/esp_lcd_touch_gt911`](https://components.espressif.com/components/espressif/esp_lcd_touch_gt911/versions/1.2.1) | `1.2.1` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lcd_touch_gt911/license.txt) |
| [`espressif/esp_lcd_touch_st7123`](https://components.espressif.com/components/espressif/esp_lcd_touch_st7123/versions/1.0.2) | `1.0.2` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lcd_touch_st7123/license.txt) |
| [`espressif/esp_lvgl_port`](https://components.espressif.com/components/espressif/esp_lvgl_port/versions/2.9.0) | `2.9.0` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_lvgl_port/license.txt) |
| [`espressif/esp_sccb_intf`](https://components.espressif.com/components/espressif/esp_sccb_intf/versions/0.0.9) | `0.0.9` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_sccb_intf/LICENSE) |
| [`espressif/esp_serial_slave_link`](https://components.espressif.com/components/espressif/esp_serial_slave_link/versions/1.1.2) | `1.1.2` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_serial_slave_link/LICENSE) |
| [`espressif/esp_video`](https://components.espressif.com/components/espressif/esp_video/versions/2.0.1) | `2.0.1` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_video/LICENSE) |
| [`espressif/esp_wifi_remote`](https://components.espressif.com/components/espressif/esp_wifi_remote/versions/1.3.1) | `1.3.1` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__esp_wifi_remote/LICENSE) |
| [`espressif/i2c_bus`](https://components.espressif.com/components/espressif/i2c_bus/versions/1.5.2) | `1.5.2` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__i2c_bus/license.txt) |
| [`espressif/m5stack_tab5`](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2) | `1.3.2` | Archive members linked in the application | Apache-2.0；[License text](licenses/managed_components/espressif__m5stack_tab5/LICENSE) |
| [`espressif/sensor_hub`](https://components.espressif.com/components/espressif/sensor_hub/versions/0.1.5) | `0.1.5` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__sensor_hub/license.txt) |
| [`espressif/usb`](https://github.com/espressif/esp-idf/tree/v5.4.2/components/usb) | ESP-IDF 5.4.2 local component override | Used at runtime; not the registry USB version | Apache-2.0；See ESP-IDF license |
| [`espressif/usb_host_uvc`](https://components.espressif.com/components/espressif/usb_host_uvc/versions/2.4.2) | `2.4.2` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[License text](licenses/managed_components/espressif__usb_host_uvc/LICENSE) |
| [`espressif/wifi_remote_over_eppp`](https://components.espressif.com/components/espressif/wifi_remote_over_eppp/versions/0.3.3) | `0.3.3` | Pinned build/transitive dependency; no archive members observed in the application | Apache-2.0；[ESP-IDF Apache-2.0 License text](licenses/esp-idf/LICENSE) |

Exact component hashes are in `dependencies.lock.template`. Sources: [ESP-IDF v5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2), [Espressif BSP](https://github.com/espressif/esp-bsp) and [LVGL component 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1). Original license copies are preserved even if upstream pages later change.

## Material not distributed

No third-party FM-1 `.fwsc` firmware, FM1 binary SDK, complete FM1 Open APP source, personal device backup or sample bank is included. Firmware names and protocol fields identify compatible products. FM1 Open APP has separate GPLv3/linking-exception and SDK terms; attribution of explicitly MIT SEA code does not grant permission to relicense that entire project or other firmware.

Installers require [Python](https://www.python.org/) and [esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0), installed separately. Neither is bundled. Normal source compilation uses the supplied generated font and icon data and does not require Pillow.

The index contains full copied license texts or complete upstream license comment blocks with provenance. This review is not a legal opinion covering every transitive file, precompiled instruction or future upstream change. Distributors must preserve applicable notices and revisit their obligations when changing dependencies, fonts, hardware targets or firmware attachments.
