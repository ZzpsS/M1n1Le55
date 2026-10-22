# Build 0.31 from source

[简体中文](BUILDING.md) · [English](BUILDING.en.md)

The target is the original M5Stack Tab5 / ESP32-P4. Install and activate **ESP-IDF 5.4.2**, then run these commands from the repository root in its terminal:

```text
python tools/prepare_build.py
idf.py build
```

The preparation script checks the SDK version, generates a local `dependencies.lock` from `IDF_PATH`, and copies `config/sdkconfig.release` when `sdkconfig` does not already exist. Existing configuration is preserved; save it elsewhere before restoring the release configuration. Building does not connect to or flash a device.

Dependencies are pinned to M5Stack Tab5 BSP 1.3.2, LVGL 9.6.0~1, ESP-Hosted 2.12.9 and ESP WiFi Remote 1.3.1. Complete versions and hashes are in `dependencies.lock.template`. Initial configuration requires access to Espressif's component registry or an existing cache. USB uses ESP-IDF 5.4.2's built-in component. `main/CMakeLists.txt` excludes ESP-Hosted's automatic initialization unit; the application initializes it after startup.

Build outputs:

- `build/tab5_fm1.bin`
- `build/bootloader/bootloader.bin`
- `build/partition_table/partition-table.bin`

Generated fonts and icon data are already in `main/`. Normal compilation does not require original image files, font-generation tools or Pillow. See [third-party notices](../THIRD_PARTY_NOTICES.en.md). UI text that introduces characters outside the existing font subset also requires updated font data.

The minimal repository does not distribute development tests, diagnostic scripts, packaging scripts or artwork generators. Python source for the installer and SD preparation tool is included in the [Releases installation bundle](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1). To install the published firmware, follow [Getting started](GETTING_STARTED_0.31.en.md).

An independent-directory build passed using the pinned SDK and an existing dependency cache. A first dependency download on a different computer has not been tested. Build time, paths and tool environments may change binary hashes. `.gitattributes` preserves file bytes by disabling automatic line-ending conversion.
