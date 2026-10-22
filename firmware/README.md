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

## 固件提供者致谢 / Firmware provider acknowledgments

感谢以下 FM-1 固件提供者：[M-VAVE](https://www.cuvave.com/download)、[hugelton / Felucca](https://github.com/hugelton/Felucca)、[isod89 / SLOOP](https://github.com/isod89/sloop-fm1)、[Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install)、[Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) 与 [X0X](https://github.com/charlesvestal/fm1-x0x)、[zednaked / Jangada](https://github.com/zednaked/jangada)、[FM1 Open](https://github.com/m-vave/fm1_open)。这些第三方 FM-1 固件需从原作者处取得，下载时请使用本版列明的兼容版本。

第三方许可与来源见[第三方声明](../THIRD_PARTY_NOTICES.md)。

Thanks to the FM-1 firmware providers: [M-VAVE](https://www.cuvave.com/download), [hugelton / Felucca](https://github.com/hugelton/Felucca), [isod89 / SLOOP](https://github.com/isod89/sloop-fm1), [Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install), [Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) and [X0X](https://github.com/charlesvestal/fm1-x0x), [zednaked / Jangada](https://github.com/zednaked/jangada), [FM1 Open](https://github.com/m-vave/fm1_open). Obtain these third-party FM-1 firmware releases from their original publishers and use the compatible versions listed for this release.

Third-party terms and sources are documented in the [third-party notices](../THIRD_PARTY_NOTICES.en.md).
