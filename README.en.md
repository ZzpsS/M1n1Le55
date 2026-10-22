# M1n1Le55

[简体中文](README.md) · [English](README.en.md)

**An M5Stack Tab5 touchscreen firmware manager and extended control console for the M-VAVE FM-1.** (Have you heard of an independent record label called miniless?)

Choose firmware, install it, then open a workspace organized around that firmware's interface. M1n1Le55 brings firmware management, sound controls and device operations to one touchscreen, with a minimal layout, pixel icons and a palette inspired by SLOOP.

**Current version: 0.31 · firmware-grid-1 (pre-release)**

[Download](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1) · [Getting started](docs/GETTING_STARTED_0.31.en.md) · [Release notes](docs/RELEASE_0.31_GRID.en.md) · [Build from source](docs/BUILDING.en.md)

![M1n1Le55 firmware management: two-column icon cards and the installation button](assets/screens/firmware-management.png)

*Firmware management preview from the offline simulator. English instructions identify the corresponding Chinese device menu labels.*

## One screen, four sections

| Section | Purpose |
| --- | --- |
| **Firmware management / 固件管理** | Browse two-column cards, inspect the selection and install; a V15 bridge is handled automatically when required |
| **Workspace / 工作台** | Control the current firmware's exposed sound, effect, performance, arrangement and project functions through matching page structures |
| **Sounds / 音色** | Sound controls and optional sample management; samples require compatible Open/FM1S firmware |
| **Device / 设备** | Connections, backups and operation records |

## This update

- **Recognizable firmware:** publisher icons where available, otherwise pixel icons based on the firmware's visual style; names appear below the icons.
- **Compact browsing:** eight regular firmware entries fit on screen in two columns; experimental entries extend the list with row snapping.
- **Direct installation:** Refresh SD, Show experimental firmware and Install selected firmware sit together at the top; selection details remain on the right.

## Start using it

Prepare an **M5Stack Tab5**, an **M-VAVE FM-1**, data cables and stable power. Add a **FAT32 microSD card** when changing FM-1 firmware.

1. **Install on Tab5.** Download and extract the bundle, connect Tab5's USB-C to a computer, and follow [Getting started](docs/GETTING_STARTED_0.31.en.md#2-install-on-tab5). The bundle includes the bootloader, partition table and application; ESP-IDF is not needed.
2. **Connect FM-1.** Connect it to Tab5's USB-A. Confirm detection under Device → Connections (**设备 → 连接**), then open Workspace (**工作台**).
3. **Change firmware when needed.** Obtain a supported release from its original publisher, prepare the card with the bundled SD tool, then select and install it in Firmware management (**固件管理**).

You can use the stock FM-1 workspace **without first flashing V15, connecting Bluetooth or inserting an SD card**. When a firmware change requires V15 as a bridge, place V15 and the target package in the library; the application arranges the sequence.

Audio comes from FM-1's audio output to headphones, speakers or an audio interface. Tab5 handles management and control.

## Supported releases

The catalog targets these exact versions: **stock V15, Felucca 1.6.1, SLOOP 2.7, FM-1+VA 101, FoMni 0.4, X0X 1.1.0, Jangada 1.0 and FM1 Open V12**.

Workspace functions depend on each firmware's exposed protocol. Installation support does not mean every function is remotely controllable or every migration route has been tested on hardware. See [download sources and SD preparation](docs/GETTING_STARTED_0.31.en.md#4-prepare-the-sd-card-and-firmware-library). Third-party FM-1 firmware is not included.

## Repository contents

```text
firmware/              Download instructions; installation bundle in Releases
assets/screens/        Homepage screenshot
main/                  ESP-IDF firmware source and generated font/icon data
tools/prepare_build.py Required build preparation entry point
docs/                  Chinese and English user, build and release guides
licenses/              Third-party terms and required SLOOP source artwork
dependencies.lock.template  Portable pinned dependency template
sdkconfig.defaults     Build settings
LICENSE                MIT license for project-owned code
```

## Status and compatibility

This is a pre-release. Builds, offline regression checks and one existing Tab5's upgrade/startup checks passed, including version retrieval, main-page switching and SD library reading. Factory-new installation, automatic V15 migration and other hardware batches still need hardware acceptance. See [release notes](docs/RELEASE_0.31_GRID.en.md).

The target is the original **M5Stack Tab5 (ESP32-P4, 16 MB Flash / 32 MB PSRAM) + M-VAVE FM-1**, not an arbitrary ESP32-P4 board. The image supports P4 revisions v0.1–v1.99 and requires an eFuse block revision no higher than 0.99. Display support code covers ILI9881C/GT911, ST7123 and ST7121/ST712x; existing hardware records cover only ST7121 / P4 v1.3.

Installation updates Tab5's P4, not its onboard C6 firmware. Bluetooth depends on compatible ESP-Hosted C6 firmware; the USB workspace works independently. The SD preparation tool currently supports Windows FAT32 drive roots only.

## Feedback and contributions

Reports should include the version, display model, FM-1 firmware identity, steps and error text. Remove device addresses, personal paths and sound/project contents before posting logs.

Start with [Build from source](docs/BUILDING.en.md). Runtime source is in [main/](main/); Python installer and SD-tool source is included in the [installation bundle](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1).

## License and credits

Project-owned code is licensed under [MIT](LICENSE); third-party material retains its original terms.

Thank you to the authors and contributors whose hardware support, open-source components, fonts and FM-1 research make M1n1Le55 possible:

- **Hardware and UI:** [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), [ESP-IDF 5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2), [Tab5 BSP 1.3.2](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2), [ESP-Hosted 2.12.9](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9), [LVGL 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1).
- **Sound and decoding references:** [Ben Sparrow / FM-1 editor](https://github.com/benny-sparra/fm1-dx7-patch-importer), [Dani Biro / SEA codec](https://github.com/Daninet/sea-codec), [M-VAVE / FM1 Open](https://github.com/m-vave/fm1_open).
- **Fonts and glyphs:** [Adobe / Source Han Sans](https://github.com/adobe-fonts/source-han-sans), [Viznut / Unscii](https://viznut.fi/unscii/), [Montserrat](https://github.com/JulietaUla/Montserrat), [Font Awesome](https://fontawesome.com/).
- **Runtime and installation tools:** [GCC](https://gcc.gnu.org/), [Newlib](https://sourceware.org/newlib/), [Python](https://www.python.org/), [esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0). Python and esptool are installed separately.

Thanks also to the FM-1 firmware authors: [M-VAVE](https://www.cuvave.com/download), [hugelton / Felucca](https://github.com/hugelton/Felucca), [isod89 / SLOOP](https://github.com/isod89/sloop-fm1), [Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install), [Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x), [zednaked / Jangada](https://github.com/zednaked/jangada), [FM1 Open](https://github.com/m-vave/fm1_open). Obtain these third-party FM-1 firmware releases from their original publishers and use the compatible versions listed for this release.

See the [complete third-party inventory](THIRD_PARTY_NOTICES.en.md) for lower-level components, pinned versions, usage and licensing. The bundle also includes Chinese/English notices and original license texts. All projects retain their original copyrights and terms; acknowledgment does not imply endorsement.
