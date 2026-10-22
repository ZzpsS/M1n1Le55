# 第三方许可与来源声明

[简体中文](THIRD_PARTY_NOTICES.md) · [English](THIRD_PARTY_NOTICES.en.md)

本仓库 M1n1Le55 自有代码按根目录 [MIT License](LICENSE) 发布，Copyright (c) 2026 M1n1Le55 contributors。第三方代码、字体、生成字形及固件中链接的库保留各自版权和许可；根 MIT 许可不替代或扩大第三方授权。

本说明对应 **0.31 / firmware-grid-1**，依赖版本沿用 onboarding-1；新增固件图标说明见下。依赖版本取自随源码提供的 `dependencies.lock.template` 和构建配置；构建辅助脚本将模板转换成本机 `dependencies.lock`。实际链接说明依据该版本的构建链接映射。档案包含成员并不代表档案所有功能均被使用，本表也不是对所有机器码逐条确认的法律审查结论。

## 固件识别图标

新增图标中，SLOOP 使用作者 GPL-3.0 SVG，M-VAVE 与 Baud Girl 使用各自发布者的品牌标志；它们不随项目 MIT 重新授权。其余像素图标为 M1n1Le55 原创。精简分发保留编译所需数据、[SLOOP 源图](licenses/firmware-icons/sloop.svg)和原许可，不附带其他图标原件或生成工具；具体来源与权利说明见 [图标声明](licenses/firmware-icons/NOTICES.md) 和 [SLOOP 原许可](licenses/firmware-icons/SLOOP-GPL-3.0.txt)。

## 仓库源码中的第三方部分

| 内容 | 来源与许可 | 随仓库保留的文本 |
| --- | --- | --- |
| `main/fm1_algorithms.h` 的算法图数据 | Ben Sparrow，2026，MIT；来自 [FM-1 编辑器/导入器项目](https://github.com/benny-sparra/fm1-dx7-patch-importer) | [fm1-editor-MIT.txt](licenses/fm1-editor-MIT.txt) |
| `main/m1_waveform.c`、`main/m1_sea_table.inc` 的 SEA 解码和量化/映射数据 | [SEA codec](https://github.com/Daninet/sea-codec)，Dani Biro，2025；M-VAVE 的 FM1 适配，2026。选用部分明确按 MIT 许可；参考 [FM1 Open V12](https://github.com/m-vave/fm1_open/releases/tag/V12) 的便携解码器与元数据 | [sea-waveform.txt](licenses/sea-waveform.txt) |
| `main/m1_fonts.c` 中 `m1_font_20/24/28` 的字形 | [Source Han Sans](https://github.com/adobe-fonts/source-han-sans)，Adobe，2014–2021；OFL 1.1。生成的子集字体仍保留 OFL 条款，使用项目字体标识而非保留字体名称 | [SourceHanSansSC.txt](licenses/m1n1le55-fonts/SourceHanSansSC.txt) |
| `main/m1_fonts.c` 中 `m1_pixel_32/48` 的字形 | [Unscii](http://viznut.fi/unscii/)，使用 `unscii-8.ttf` 的公有领域变体；不是含 GNU Unifont 的 `unscii-16-full` | [上游说明原文](licenses/m1n1le55-fonts/unscii.html) |

生成字形的源字体 SHA-256：`SourceHanSansSC-Normal.otf` 为 `1ee89e1669362dee13851129c0a8a791a87521eb4148e5efbf5d26596738e25b`；`unscii-8.ttf` 为 `1be7ca5552beb67f6bf1bed4a0db92a53835f1c07cb0f643063d82ffb0dccba0`。字体不随项目 MIT 重新授权。已有 `sea-waveform.txt` 的研究目录说明记录开发时的原始文件出处，不表示该研究目录包含在本仓库。

## 随 Tab5 固件分发的库和运行时

- **LVGL `9.6.0~1`**：主体 MIT，Copyright (c) 2025 LVGL Kft；[许可全文](licenses/lvgl/LICENCE.txt)、[上游第三方清单](licenses/lvgl/COPYRIGHTS.md)。本版本实际使用的内置 Montserrat 14/16 字体含 Montserrat 与 Font Awesome 5 字形，分别保留 [Montserrat OFL](licenses/lvgl/scripts/generators/built_in_font/font_license/Montserrat/OFL.txt) 和 [Font Awesome 5 许可](licenses/lvgl/scripts/generators/built_in_font/font_license/FontAwesome5/LICENSE.txt)。LVGL 内置 printf 的 [MIT 声明](licenses/lvgl/src/stdlib/builtin/LICENSE_SPRINTF.txt) 一并保留。COPYRIGHTS.md 还列出 LVGL 可选库，不表示这些库均已启用或随本固件链接。
- **ESP-IDF `5.4.2`**：主体 Apache-2.0；[总许可](licenses/esp-idf/LICENSE)、[上游版权清单](licenses/esp-idf/docs/en/COPYRIGHT.rst)。它包含许可各异的第三方部分；随附 notices 包括 FreeRTOS、lwIP、cJSON、Mbed TLS、SPIFFS、NimBLE、tinycrypt、protobuf-c、console、newlib、FatFs、TLSF 和 UBSAN。对应原文均收录在 `licenses/esp-idf/`，不能将 SDK 的所有第三方部分概括为 Apache-2.0。
- **ESP-Hosted protobuf-c**：其 BSD 原文另存 [protobuf-c-LICENSE](licenses/managed_components/espressif__esp_hosted/protobuf-c-LICENSE)。
- **GCC runtime / Newlib**：构建工具链为 Espressif `esp-14.2.0_20241119`，GCC `14.2.0`。成品链接了 libgcc、libstdc++、libc/libm 的运行时代码；保留 [GPLv3](licenses/toolchain/gcc/COPYING3)、[GCC Runtime Library Exception 3.1](licenses/toolchain/gcc/COPYING.RUNTIME) 和 [Newlib 条款合集](licenses/toolchain/newlib/COPYING.NEWLIB)。运行库例外与新库原条款适用，不因此把项目自有代码改成 GPL。编译器程序本身未放入仓库。
- **ESP IPA `1.3.1`**：成品实际链接 `libesp_ipa.a`。其 [ESPRESSIF MIT License](licenses/managed_components/espressif__esp_ipa/LICENSE) 明确限定用于 Espressif Systems 产品，**不是标准 MIT 许可**。Tab5 的 ESP32-P4 属此硬件范围；移植到其他厂商硬件不能援引项目 MIT 许可扩张该授权。这个预编译库的源代码不在本仓库，构建时由依赖管理器获取。

## 锁定组件与获取方式

`managed_components/` 不作为项目源码整体随仓库复制。ESP-IDF Component Manager 按 `main/idf_component.yml` 和 `dependencies.lock` 下载所需组件；ESP-IDF 本身由开发者安装。下表列出锁文件中的 Espressif 组件，区分当前应用实际链接和仅锁定的传递依赖。各顶层许可保留在 `licenses/managed_components/`。没有单独顶层许可文件的 `wifi_remote_over_eppp` 在上游组件清单及源码中声明 Apache-2.0。

| 组件 / 上游注册表 | 锁定版本 | 此版本的使用证据 | 许可 |
| --- | --- | --- | --- |
| [`espressif/bmi270`](https://components.espressif.com/components/espressif/bmi270/versions/1.1.0) | `1.1.0` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__bmi270/license.txt) |
| [`espressif/cmake_utilities`](https://components.espressif.com/components/espressif/cmake_utilities/versions/0.5.3) | `0.5.3` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__cmake_utilities/license.txt) |
| [`espressif/eppp_link`](https://components.espressif.com/components/espressif/eppp_link/versions/1.1.6) | `1.1.6` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__eppp_link/LICENSE) |
| [`espressif/esp_cam_sensor`](https://components.espressif.com/components/espressif/esp_cam_sensor/versions/2.0.1) | `2.0.1` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_cam_sensor/LICENSE) |
| [`espressif/esp_codec_dev`](https://components.espressif.com/components/espressif/esp_codec_dev/versions/1.5.11) | `1.5.11` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_codec_dev/LICENSE) |
| [`espressif/esp_h264`](https://components.espressif.com/components/espressif/esp_h264/versions/1.0.4) | `1.0.4` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_h264/LICENSE) |
| [`espressif/esp_hosted`](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9) | `2.12.9` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_hosted/LICENSE) |
| [`espressif/esp_io_expander`](https://components.espressif.com/components/espressif/esp_io_expander/versions/1.2.1) | `1.2.1` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_io_expander/license.txt) |
| [`espressif/esp_io_expander_pi4ioe5v6408`](https://components.espressif.com/components/espressif/esp_io_expander_pi4ioe5v6408/versions/1.0.1) | `1.0.1` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_io_expander_pi4ioe5v6408/license.txt) |
| [`espressif/esp_ipa`](https://components.espressif.com/components/espressif/esp_ipa/versions/1.3.1) | `1.3.1` | 当前应用链接档案含成员 | Espressif 限定硬件许可；[原文](licenses/managed_components/espressif__esp_ipa/LICENSE) |
| [`espressif/esp_lcd_ili9881c`](https://components.espressif.com/components/espressif/esp_lcd_ili9881c/versions/1.1.0) | `1.1.0` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lcd_ili9881c/license.txt) |
| [`espressif/esp_lcd_st7123`](https://components.espressif.com/components/espressif/esp_lcd_st7123/versions/1.0.2) | `1.0.2` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lcd_st7123/license.txt) |
| [`espressif/esp_lcd_touch`](https://components.espressif.com/components/espressif/esp_lcd_touch/versions/1.2.1) | `1.2.1` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lcd_touch/license.txt) |
| [`espressif/esp_lcd_touch_gt911`](https://components.espressif.com/components/espressif/esp_lcd_touch_gt911/versions/1.2.1) | `1.2.1` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lcd_touch_gt911/license.txt) |
| [`espressif/esp_lcd_touch_st7123`](https://components.espressif.com/components/espressif/esp_lcd_touch_st7123/versions/1.0.2) | `1.0.2` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lcd_touch_st7123/license.txt) |
| [`espressif/esp_lvgl_port`](https://components.espressif.com/components/espressif/esp_lvgl_port/versions/2.9.0) | `2.9.0` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_lvgl_port/license.txt) |
| [`espressif/esp_sccb_intf`](https://components.espressif.com/components/espressif/esp_sccb_intf/versions/0.0.9) | `0.0.9` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_sccb_intf/LICENSE) |
| [`espressif/esp_serial_slave_link`](https://components.espressif.com/components/espressif/esp_serial_slave_link/versions/1.1.2) | `1.1.2` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_serial_slave_link/LICENSE) |
| [`espressif/esp_video`](https://components.espressif.com/components/espressif/esp_video/versions/2.0.1) | `2.0.1` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_video/LICENSE) |
| [`espressif/esp_wifi_remote`](https://components.espressif.com/components/espressif/esp_wifi_remote/versions/1.3.1) | `1.3.1` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__esp_wifi_remote/LICENSE) |
| [`espressif/i2c_bus`](https://components.espressif.com/components/espressif/i2c_bus/versions/1.5.2) | `1.5.2` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__i2c_bus/license.txt) |
| [`espressif/m5stack_tab5`](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2) | `1.3.2` | 当前应用链接档案含成员 | Apache-2.0；[原文](licenses/managed_components/espressif__m5stack_tab5/LICENSE) |
| [`espressif/sensor_hub`](https://components.espressif.com/components/espressif/sensor_hub/versions/0.1.5) | `0.1.5` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__sensor_hub/license.txt) |
| `espressif/usb` | ESP-IDF 5.4.2 本地组件覆盖 | 运行时使用；不是注册表 USB 版本 | Apache-2.0；见 ESP-IDF 总许可 |
| [`espressif/usb_host_uvc`](https://components.espressif.com/components/espressif/usb_host_uvc/versions/2.4.2) | `2.4.2` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[原文](licenses/managed_components/espressif__usb_host_uvc/LICENSE) |
| [`espressif/wifi_remote_over_eppp`](https://components.espressif.com/components/espressif/wifi_remote_over_eppp/versions/0.3.3) | `0.3.3` | 锁定的构建/传递依赖；当前应用链接档案未见成员 | Apache-2.0；[ESP-IDF Apache-2.0 原文](licenses/esp-idf/LICENSE) |

LVGL 注册表来源：[lvgl/lvgl 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1)。ESP-IDF 固定来源：[v5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2)。Tab5 BSP 源项目：[Espressif esp-bsp](https://github.com/espressif/esp-bsp)。精确组件内容摘要以锁文件为准；上述网页将来可能调整，随仓库的许可副本用于保存本次构建所见原文。

## 本仓库没有分发的内容

本仓库固件目录面向 Tab5，未附带 M-VAVE/FM1、Felucca、SLOOP、Jangada、FoMni、X0X 等第三方 `.fwsc` 固件、FM1 二进制 SDK、完整 FM1 Open APP 源码或设备个人备份。固件名称、协议字段与链接用于标识兼容对象，不表示作者背书。

FM1 Open APP 自有源码另按 GPLv3 与其链接例外发布，SDK 还有独立条款。这里对明确 MIT 的 SEA 部分作署名，不代表项目拥有重新授权整个 FM1 Open 项目或其他 FM1 固件的权利。后续若增加第三方固件附件，须按其实际版本重新核对许可、对应源码和声明要求。

安装工具依赖 Python 和 `esptool==4.11.0`，由用户另行安装；本仓库未打包 Python 解释器或 esptool 程序。精简源码已经包含生成好的字体和图标数据，普通编译不需要 Pillow。

## 许可副本与核查范围

[许可文本索引](licenses/SOURCE_INDEX.md) 列出原文出处及 SHA-256。完整文件原样复制；少数许可仅存在于源文件顶部，因此保存完整注释块并注明出处。TLSF 的完整上游 BSD-3-Clause 声明取自所用 LVGL 分发包的 `lv_tlsf.h`；ESP-IDF TLSF 源码标注同一 Matthew Conte 版权及 BSD-3-Clause，另外保留 ESP-IDF 的版权清单。

本次整理核对了明确源码署名、当前构建配置、锁文件、链接档案与本机上游许可原文。它不是对所有传递依赖每个文件、所有预编译机器码和未来上游变更的完整法律审查。发布者仍需对自己提交的原创内容有权授权，并在更换依赖、字体、目标硬件或固件附件时更新本说明及许可副本。
