# M1n1Le55

[简体中文](README.md) · [English](README.en.md)

**为M-vave FM-1  打造 M5Stack Tab5触屏固件管理器与扩展控制台。** (你听说过一家叫做miniless的独立唱片厂牌吗)

选择固件、完成安装，再进入与该固件结构对应的工作台。M1n1Le55 将固件管理、音色控制与设备操作集中在一块触屏上，以极简布局、像素图标和受 SLOOP 启发的配色，呈现当前设备与可用功能。

**当前版本：0.31 · firmware-grid-1（预发布）**

[下载 Tab5 安装包](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1) · [新玩家说明](docs/GETTING_STARTED_0.31.md) · [版本说明](docs/RELEASE_0.31_GRID.md) · [构建说明](docs/BUILDING.md)

![M1n1Le55 0.31 固件管理页面：两列图标卡片与顶部安装按钮](assets/screens/firmware-management.png)

*固件管理界面预览，来自离线模拟。*

## 一块屏幕，四个入口

| 入口 | 可以做什么 |
| --- | --- |
| **固件管理** | 用两列图标卡片浏览固件，选中后查看详情并安装；需要 V15 中转时自动处理 |
| **工作台** | 页面结构跟随当前固件切换，控制已有协议支持的音色参数、效果、演奏、编排与工程功能 |
| **音色** | 集中音色与可选采样功能；采样需要实现 Open/FM1S 协议的兼容固件 |
| **设备** | 查看连接、进行备份、查阅操作记录 |

## 这次更新

- **固件更容易辨认：** 优先使用发布者图标，其余按对应固件的界面风格绘制像素图标；图标下方直接显示名称。
- **列表更紧凑：** 两列卡片，八款正式固件同屏；展开实验固件后按行吸附滚动。
- **安装入口更直接：** “刷新 SD → 显示实验固件 → 安装该固件”集中在顶部，右侧保留所选固件的详情。

## 开始使用

准备一台 **M5Stack Tab5**、一台 **M-VAVE FM-1**、数据线与稳定供电。更换 FM-1 固件时，再准备一张 **FAT32 microSD 卡**。

1. **安装到 Tab5。** 下载并解压安装包，用电脑连接 Tab5 的 USB-C，按[新玩家说明](docs/GETTING_STARTED_0.31.md#2-首次安装到-tab5)完成安装。包内包含引导程序、分区表和应用，无须安装 ESP-IDF。
2. **连接 FM-1。** 将 FM-1 接到 Tab5 的 USB-A，在“设备 → 连接”确认识别，再打开“工作台”。
3. **按需换固件。** 从原作者处取得受支持的固件，用随包 SD 工具准备卡，在“固件管理”中选择并安装。

从原厂 FM-1 开始使用工作台，**不需要先刷 V15，也不需要先连蓝牙或插 SD 卡**。换固件需要 V15 中转时，将 V15 与目标包一并放入固件库，程序自动安排顺序。

声音由 FM-1 的音频输出送到耳机、音箱或声卡；Tab5 负责管理与控制。

## 当前适配

当前适配以下固定版本：**原厂 V15、Felucca 1.6.1、SLOOP 2.7、FM-1+VA 101、FoMni 0.4、X0X 1.1.0、Jangada 1.0、FM1 Open V12**。

工作台能力以各固件实际开放的协议为准。安装适配不代表每项功能都能远程控制，也不代表所有迁移路线均已完成实机验证。下载来源和准备步骤见[新玩家说明](docs/GETTING_STARTED_0.31.md#4-准备-sd-卡和固件库)；本仓库不附带这些第三方 FM-1 固件。

## 仓库内容

```text
firmware/              安装包下载入口；完整安装包在 GitHub Releases
assets/screens/        首页界面预览
main/                  ESP-IDF 固件源码与生成好的字体
tools/prepare_build.py  必需的构建准备入口
docs/                  新玩家说明、构建说明和版本说明
licenses/              第三方许可证原文
dependencies.lock.template  不含开发者本机路径的依赖锁模板
sdkconfig.defaults     构建配置
LICENSE                项目自有代码的 MIT 许可证
```

## 版本状态与兼容性

本版为预发布，已完成构建、离线回归，以及一台已有 M1n1Le55 的 Tab5 升级启动检查：版本读取、四个主页面切换和 SD 固件库读取通过。全新出厂设备首装、自动 V15 中转及不同硬件批次仍需进一步实机验收，具体记录见[版本说明](docs/RELEASE_0.31_GRID.md)。

目标硬件是原版 **M5Stack Tab5（ESP32-P4，16 MB Flash / 32 MB PSRAM）+ M-VAVE FM-1**，不是任意 ESP32-P4 开发板。镜像支持的 P4 修订为 v0.1–v1.99；应用要求 eFuse block 版本不高于 0.99。屏幕代码覆盖 ILI9881C/GT911、ST7123 和 ST7121/ST712x，目前已有实机记录只覆盖 ST7121 / P4 v1.3。

首装只更新 Tab5 的 P4，不更新其 C6 无线固件。蓝牙依赖 C6 的 ESP-Hosted 兼容性；USB 工作台可独立使用。SD 准备工具目前仅支持 Windows FAT32 盘符根目录。详细范围见 [版本说明](docs/RELEASE_0.31_GRID.md)。

## 反馈与参与

欢迎提交问题复现、兼容性记录与功能改进。反馈时请附版本、屏幕型号、FM-1 固件身份、操作步骤和错误文字；公开日志前移除设备地址、个人路径与音色内容。

开发从[构建说明](docs/BUILDING.md)开始，源码位于 [main/](main/)。安装器与 SD 准备工具随 [Releases 安装包](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1)提供。

## 许可证与致谢

项目自有代码采用 [MIT](LICENSE)，第三方内容保留各自许可。

感谢以下项目的作者与贡献者，为 M1n1Le55 提供硬件支持、开源组件、字体和 FM-1 研究成果：

- **硬件与界面：** [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5)、[ESP-IDF 5.4.2](https://github.com/espressif/esp-idf/tree/v5.4.2)、[Tab5 BSP 1.3.2](https://components.espressif.com/components/espressif/m5stack_tab5/versions/1.3.2)、[ESP-Hosted 2.12.9](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.9)、[LVGL 9.6.0~1](https://components.espressif.com/components/lvgl/lvgl/versions/9.6.0~1)。
- **音色与解码参考：** [Ben Sparrow / FM-1 编辑器](https://github.com/benny-sparra/fm1-dx7-patch-importer)、[Dani Biro / SEA codec](https://github.com/Daninet/sea-codec)、[M-VAVE / FM1 Open](https://github.com/m-vave/fm1_open)。
- **字体与字形：** [Adobe / 思源黑体](https://github.com/adobe-fonts/source-han-sans)、[Viznut / Unscii](https://viznut.fi/unscii/)、[Montserrat](https://github.com/JulietaUla/Montserrat)、[Font Awesome](https://fontawesome.com/)。
- **运行时与安装工具：** [GCC](https://gcc.gnu.org/)、[Newlib](https://sourceware.org/newlib/)、[Python](https://www.python.org/)、[esptool 4.11.0](https://github.com/espressif/esptool/tree/v4.11.0)。Python 与 esptool 由用户另行安装。

同时感谢 FM-1 固件作者：[M-VAVE](https://www.cuvave.com/download)、[hugelton / Felucca](https://github.com/hugelton/Felucca)、[isod89 / SLOOP](https://github.com/isod89/sloop-fm1)、[Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install)、[Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) 与 [X0X](https://github.com/charlesvestal/fm1-x0x)、[zednaked / Jangada](https://github.com/zednaked/jangada)、[FM1 Open](https://github.com/m-vave/fm1_open)。这些第三方 FM-1 固件需从原作者处取得，下载时请使用本版列明的兼容版本。

更多底层组件、锁定版本、用途和许可见[完整第三方清单](THIRD_PARTY_NOTICES.md)；安装包内也附有中英声明及许可原文。各项目保留原有版权和许可，致谢不表示官方背书。
