# Tab5 固件 / Tab5 firmware

**0.31 · firmware-grid-1 — 预发布 / Pre-release**

[下载完整安装包 / Download the installation bundle](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1)

下载 `M1n1Le55-0.31-grid-1.zip` 和 `SHA256SUMS.txt`，解压后按[中文新玩家说明](../docs/GETTING_STARTED_0.31.md)安装。GitHub 自动生成的 Source code ZIP 是源码，不是安装包。固件与安装工具集中在 Releases，本目录不重复存放二进制文件。

Download `M1n1Le55-0.31-grid-1.zip` and `SHA256SUMS.txt`, extract the bundle, and follow the [English getting-started guide](../docs/GETTING_STARTED_0.31.en.md). GitHub's automatic Source code ZIP is for development, not installation. Firmware and installation tools are distributed through Releases.

| 镜像 / Image | 地址 / Offset |
| --- | --- |
| `images/bootloader.bin` | `0x2000` |
| `images/partition-table.bin` | `0x8000` |
| `images/M1n1Le55-0.31.bin` | `0x10000` |

三份镜像组成 Tab5 P4 首装包，不包含 C6、第三方 FM-1 固件或个人备份。安装器默认只检查本地文件；烧录需要显式指定端口、主板和 `--install`。

These three images form the Tab5 P4 installation bundle. It contains no C6 firmware, third-party FM-1 firmware or personal backups. The installer checks local files by default; flashing requires an explicit port, board and `--install`.

## 第三方组件与致谢 / Third-party components and acknowledgments

感谢以下项目的作者与贡献者，为 M1n1Le55 提供硬件支持、开源组件、字体和 FM-1 研究成果：

- **硬件与界面：** [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5)、[ESP-IDF 5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2)、[Tab5 BSP 1.3.2](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2)、[ESP-Hosted 2.12.9](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9)、[LVGL 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1)。
- **音色与解码参考：** [Ben Sparrow / FM-1 编辑器](https://github.com/benny-sparra/fm1-dx7-patch-importer)、[Dani Biro / SEA codec](https://github.com/Daninet/sea-codec)、[M-VAVE / FM1 Open](https://github.com/m-vave/fm1_open)。
- **字体与字形：** [Adobe / 思源黑体](https://github.com/adobe-fonts/source-han-sans)、[Viznut / Unscii](https://viznut.fi/unscii/)、[Montserrat](https://github.com/JulietaUla/Montserrat)、[Font Awesome](https://fontawesome.com/)。
- **运行时与安装工具：** [GCC](https://gcc.gnu.org/)、[Newlib](https://sourceware.org/newlib/)、[Python](https://www.python.org/)、[esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0)。Python 与 esptool 由用户另行安装。

同时感谢 FM-1 固件作者：[M-VAVE](https://www.cuvave.com/download)、[hugelton / Felucca](https://github.com/hugelton/Felucca)、[isod89 / SLOOP](https://github.com/isod89/sloop-fm1)、[Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install)、[Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) 与 [X0X](https://github.com/charlesvestal/fm1-x0x)、[zednaked / Jangada](https://github.com/zednaked/jangada)、[FM1 Open](https://github.com/m-vave/fm1_open)。这些第三方 FM-1 固件需从原作者处取得，下载时请使用本版列明的兼容版本。

更多底层组件、锁定版本、用途和许可见[完整第三方清单](../THIRD_PARTY_NOTICES.md)；安装包内也附有中英声明及许可原文。各项目保留原有版权和许可，致谢不表示官方背书。

Thank you to the authors and contributors whose hardware support, open-source components, fonts and FM-1 research make M1n1Le55 possible:

- **Hardware and UI:** [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), [ESP-IDF 5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2), [Tab5 BSP 1.3.2](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2), [ESP-Hosted 2.12.9](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9), [LVGL 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1).
- **Sound and decoding references:** [Ben Sparrow / FM-1 editor](https://github.com/benny-sparra/fm1-dx7-patch-importer), [Dani Biro / SEA codec](https://github.com/Daninet/sea-codec), [M-VAVE / FM1 Open](https://github.com/m-vave/fm1_open).
- **Fonts and glyphs:** [Adobe / Source Han Sans](https://github.com/adobe-fonts/source-han-sans), [Viznut / Unscii](https://viznut.fi/unscii/), [Montserrat](https://github.com/JulietaUla/Montserrat), [Font Awesome](https://fontawesome.com/).
- **Runtime and installation tools:** [GCC](https://gcc.gnu.org/), [Newlib](https://sourceware.org/newlib/), [Python](https://www.python.org/), [esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0). Python and esptool are installed separately.

Thanks also to the FM-1 firmware authors: [M-VAVE](https://www.cuvave.com/download), [hugelton / Felucca](https://github.com/hugelton/Felucca), [isod89 / SLOOP](https://github.com/isod89/sloop-fm1), [Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install), [Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x), [zednaked / Jangada](https://github.com/zednaked/jangada), [FM1 Open](https://github.com/m-vave/fm1_open). Obtain these third-party FM-1 firmware releases from their original publishers and use the compatible versions listed for this release.

See the [complete third-party inventory](../THIRD_PARTY_NOTICES.en.md) for lower-level components, pinned versions, usage and licensing. The bundle also includes Chinese/English notices and original license texts. All projects retain their original copyrights and terms; acknowledgment does not imply endorsement.
