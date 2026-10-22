# M1n1Le55 0.31 — Getting started

[简体中文](GETTING_STARTED_0.31.md) · [English](GETTING_STARTED_0.31.en.md)

For M5Stack Tab5 + M-VAVE FM-1. Start with the stock FM-1 sound workspace and change firmware when needed. **You do not need to flash V15 or connect Bluetooth to get started.**

This guide covers `0.31 / firmware-grid-1`, including the two-column firmware cards and the r2 V15 migration fix. Download `M1n1Le55-0.31-grid-1.zip` from [GitHub Releases](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1) and extract it. The automatically generated Source code archives are for development, not installation. An application-only image is not a complete first-install package. Offline checks and builds have passed, but a complete installation on a factory-new Tab5 has not been verified; this is a pre-release.

## 1. What you need

- Tab5, FM-1 and stable power. A standard Tab5 may not include a battery.
- A data cable from the computer to Tab5 **USB-C** for installation, and a USB data cable from Tab5 **USB-A** to FM-1 for normal use. Charging-only cables will not work.
- Headphones, speakers or an audio interface connected to FM-1's audio output. This application does not route FM-1 audio to Tab5's speaker.
- For firmware management, backups or samples: an already-formatted **FAT32 microSD card** and reader. The stock USB workspace does not require SD. Neither the application nor the preparation tool automatically formats a card.
- A Windows computer and Python 3.10 or newer. If `python` is unavailable, use `py -3`. Automated SD preparation currently supports Windows FAT32 drive roots only.

See [M5Stack's Tab5 documentation](https://docs.m5stack.com/en/core/Tab5) for ports, 16 MB Flash / 32 MB PSRAM specifications and download mode. Do not install this bundle on another ESP32-P4 board, a different Tab5-like product or FM-1.

The images accept ESP32-P4 chip revisions **v0.1–v1.99**; the application also requires eFuse block revision ≤0.99. A newer batch outside these ranges is unsupported. Do not bypass a rejection with `--force`. Chip revision and display model are separate compatibility conditions.

## 2. Install on Tab5

Open a terminal in the extracted installation bundle. The initial dependency installation needs internet access; flashing then uses the bundled images:

```powershell
python -m pip install esptool==4.11.0
python install.py
```

The second command checks local files and shows the plan without opening a serial port. The bundle contains the bootloader, partition table and application; ESP-IDF is not required.

1. Disconnect FM-1 from Tab5 USB-A temporarily. Connect the computer to Tab5 USB-C.
2. Find Tab5's serial port in Windows Device Manager. `COM7` below is only an example; replace it with your port.
3. If automatic download mode fails, hold Tab5 Reset for about two seconds until its internal green LED flashes rapidly, then release it, following the [official recovery guide](https://docs.m5stack.com/en/guide/tab5/restore_factory).
4. Run the installation command once:

```powershell
python install.py --mode first-install --port COM7 --board m5stack-tab5 --install
```

First installation replaces Tab5's current system and partition table. Data left in old partitions is not a recovery method for the previous system. The installer checks the chip, physical flash size and bundle, does not erase the entire chip, and does not modify FM-1, the SD card or Tab5's ESP32-C6 firmware. Wait for flashing and restart to finish; the screen should show M1n1Le55 0.31.

If the same M1n1Le55 layout is already installed, `--mode app-update` updates only the application after reading and comparing the existing bootloader and partition table. A mismatch stops the operation. Do not perform a first install by writing only the application at `0x10000`.

## 3. Connect FM-1 and hear sound

1. Power both devices reliably and connect FM-1's USB data port to Tab5 USB-A.
2. Open **Device → Connections / 设备 → 连接**. Confirm USB connection and a detected firmware identity.
3. Open **Workspace / 工作台**. Stock firmware uses its corresponding FM / VOICE pages; controls become editable after the current sound is read successfully.
4. Play FM-1 itself or trigger notes through your MIDI controller. Listen through FM-1's audio output, then change one parameter and confirm the result.

If USB is not detected, check the data cable, USB-A connection and power before reconnecting. Flashing firmware will not fix a cable or port problem. USB and Bluetooth may connect to different FM-1 devices; verify the Bluetooth target before sample operations.

The four sections are **Firmware management / 固件管理**, **Workspace / 工作台**, **Sounds / 音色**, and **Device / 设备**. Device contains only connections, backups and records.

## 4. Prepare the SD card and firmware library

Skip this section if you only need stock sound controls. For firmware changes, obtain **official V15 and your target firmware**. M1n1Le55 uses V15 automatically when a supported migration needs it; you do not have to perform two manual installations.

Use the exact `.fwsc` versions below, not an unspecified latest release:

| Firmware | Pinned version | Publisher download |
| --- | --- | --- |
| Stock | V15 | [M-VAVE downloads](https://www.cuvave.com/download) |
| Felucca | 1.6.1 | [Release](https://github.com/hugelton/Felucca/releases/tag/v1.6.1) |
| SLOOP | 2.7 | [Release](https://github.com/isod89/sloop-fm1/releases/tag/v2.7) |
| FM-1+VA | 101 | [Installation page](https://baudgirl.com/work/FM-1%2BVA/install) |
| FoMni | 0.4 | [Release](https://github.com/charlesvestal/fm1-fomni/releases/tag/v0.4) |
| X0X | 1.1.0 | [Release](https://github.com/charlesvestal/fm1-x0x/releases/tag/v1.1.0) |
| Jangada | 1.0 | [Release](https://github.com/zednaked/jangada/releases/tag/v1.0) |
| FM1 Open | V12 | [Release](https://github.com/m-vave/fm1_open/releases/tag/V12) |

Local Probe V912 / Diag V913 profiles are experimental and are not recommended starter installations. Catalog support does not mean every feature or migration route has been hardware-tested. If a publisher replaces a package and its hash changes, the preparation tool stops; do not bypass this check.

Insert the card into a computer reader and run these commands in the installation bundle directory. Check that `E:\` is your card's drive root, and substitute your downloaded package paths:

```powershell
python prepare_sd_card.py --sd-root E:\ --package .\FM-1.fwsc --package .\sloop-2.7.fwsc
python prepare_sd_card.py --sd-root E:\ --package .\FM-1.fwsc --package .\sloop-2.7.fwsc --write
```

The first command shows the plan; the second applies it. The tool checks identity, size and SHA-256 against its catalog, creates the required directories and writes `.fwsc.sha256` files. V15 is named `M-VAVE-FM-1-V15.fwsc` for automatic bridging; other packages use their identities. It does not format the card, delete files or overwrite different existing content. Third-party FM-1 firmware is not included in this bundle.

Safely eject the card, insert it into a **powered-off Tab5**, then power on. Open Firmware management (**固件管理**) to see the imported packages:

```text
fm1/
  firmware/   .fwsc packages and matching .fwsc.sha256 files
  backups/    Stock current-voice backups
  samples/    .fm1s sample banks and backups
  exchange/   Firmware-specific object exchange files
```

Copying `.fwsc` files without their checksum sidecars blocks touchscreen installation in 0.31; use the bundled preparation tool. If no card was present at startup, mounting failed, or the card uses exFAT, insert a readable FAT32 card and restart Tab5. Do not remove it during operations.

## 5. Change FM-1 firmware

Select a target in the two-column cards under **Firmware management / 固件管理**, review the source and target on the right, then press **Install selected firmware / 安装该固件** at the top. One installation action starts the supported route; the application handles a required V15 bridge and reports a missing V15 package before starting. Wait for completion and the target identity, then open Workspace (**工作台**).

Do not unplug, interrupt power or start another installation during transfer. Failure does not mean rollback; Records retains the reason. If the OTA loader is still detected, select a package matching that loader for recovery. A black screen or ROM download state is outside this tool's recovery scope.

Stock backup saves only the **current 155-byte voice**, not all presets, effects or complete FM-1 firmware. There is no touchscreen one-click restoration of that JSON voice backup. Custom-firmware object backups depend on its exposed protocol. Export important presets, projects and samples using that firmware's supported tools.

## 6. Samples are optional

A new FM-1's stock firmware **does not support this application's Open/FM1S sample-transfer protocol**. A Bluetooth connection alone does not imply sample support; the sample menu is not a reason to repeatedly flash stock firmware or retry uploads.

Use sample management only with firmware implementing Open app protocol 4 and the sample commands. Existing hardware records use a local V913 experimental build; they do not establish support for all Open or Felucca-family firmware. Tab5's C6 also needs compatible ESP-Hosted firmware; historical records used C6 1.4.1, and factory batches remain unverified. USB stock controls and firmware management work independently of Bluetooth.

Place a valid `.fm1s` bank in `/fm1/samples/`. Connect the correct Bluetooth FM-1 under **设备 → 连接**, then open **音色 → 打开采样管理** (Sounds → Sample management). The application checks the protocol, backs up an existing valid bank, then replaces the entire eight-slot bank. Fast completion is the default; full readback is optional.

A failed transfer does not automatically roll back. On the same connection, use **准备恢复备份** (Prepare backup recovery). After reconnecting, use **选择恢复文件** (Select recovery file), choose the saved `backup-*.fm1s` on SD, and confirm the target. WAV files cannot be imported directly. This bundle does not include experimental sample firmware or a WAV conversion toolchain.

## Troubleshooting

| Symptom | Action |
| --- | --- |
| No serial port / flashing cannot connect | Check USB-C, data cable and port; close software using the port; enter download mode manually |
| Wrong chip or flash size | Stop and confirm an original Tab5; do not force the write |
| Blank screen / touch problems after flashing | Record the display-driver model; three driver families are present in code, but only one has hardware records. Use official factory recovery if needed |
| SD unavailable | Check FAT32 and card insertion, then restart; the application does not format cards |
| Package blocked / hash mismatch | Use the exact release and preparation tool; renaming cannot make an unknown package acceptable |
| Stock editing works but samples do not | This is a protocol limitation; continue using the USB workspace |
| An effect value shows `--` | No readback is available; the value is unknown, not necessarily off |
| Controls unavailable after a firmware change | Check its identity and the page's protocol notes; installation support is not full remote-control support |

For feedback, provide the Tab5 display model, M1n1Le55 version/revision, FM-1 firmware identity, steps and error text. Remove Bluetooth addresses, personal paths and sound/project contents from public logs. Do not upload full Flash, NVS or personal SD backups.

## Firmware provider acknowledgments

Thanks to the FM-1 firmware providers: [M-VAVE](https://www.cuvave.com/download), [hugelton / Felucca](https://github.com/hugelton/Felucca), [isod89 / SLOOP](https://github.com/isod89/sloop-fm1), [Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install), [Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x), [zednaked / Jangada](https://github.com/zednaked/jangada), [FM1 Open](https://github.com/m-vave/fm1_open). Obtain these third-party FM-1 firmware releases from their original publishers and use the compatible versions listed for this release.

Third-party terms and sources are documented in the [third-party notices](../THIRD_PARTY_NOTICES.en.md).
