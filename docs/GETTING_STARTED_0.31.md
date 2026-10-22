# M1n1Le55 0.31 新玩家上手指南

[简体中文](GETTING_STARTED_0.31.md) · [English](GETTING_STARTED_0.31.en.md)

适用：M5Stack Tab5 + M-VAVE FM-1。先用原厂 FM-1 的音色工作台，再按需换固件。**不需要为了开始使用先刷 V15，也不需要先连接蓝牙。**

本指南对应 `0.31 / firmware-grid-1` 候选版，含两列固件卡片界面及 r2 的 V15 中转修正。从 [GitHub Releases](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1) 下载并解压 `M1n1Le55-0.31-grid-1.zip`。页面自动生成的 Source code 压缩包仅供开发，不是刷机安装包。旧的仅含应用镜像安装包不适合首次安装。本版已完成离线模拟、工具测试和构建；尚未对新的出厂 Tab5 做完整首装实测，不能将候选包当成所有硬件批次已验收的稳定版。

## 1. 准备物品

- Tab5、FM-1 和稳定电源；标准 Tab5 不一定附带电池。
- 电脑到 Tab5 **USB-C** 的数据线，用于安装；Tab5 **USB-A** 到 FM-1 的 USB 数据线，用于日常控制。只有充电功能的线无法识别设备。
- FM-1 的耳机或音频输出接耳机、音箱或声卡。当前程序不把 FM-1 的音频传到 Tab5 扬声器。
- 如需固件管理、备份或采样：一张已有 **FAT32** 文件系统的 microSD 卡和读卡器。仅连接原厂音色工作台不要求 SD。程序和准备工具都不会自动格式化卡。
- 本指南按 Windows 电脑编写，安装 Python 3.10 或更新版本。命令中的 `python` 若不可用，可改用 `py -3`。SD 自动准备工具目前仅支持 Windows FAT32 盘符根目录。

Tab5 的接口、16 MB Flash / 32 MB PSRAM 规格以及下载模式操作见 [M5Stack 官方说明](https://docs.m5stack.com/en/core/Tab5)。不要将本包刷到其他 ESP32-P4 开发板、Tab5 替代机型或 FM-1 上。

本次镜像头限定 ESP32-P4 芯片修订 **v0.1–v1.99**，应用还限定 eFuse block 版本不高于 0.99。新批次若超出该范围，不属于本包支持范围；安装工具不会添加 `--force` 绕过检查。芯片修订与屏幕型号是不同条件。

## 2. 首次安装到 Tab5

在安装包解压目录打开终端。第一次需要联网安装烧录依赖，随后本包的安装过程不下载固件：

```powershell
python -m pip install esptool==4.11.0
python install.py
```

第二条命令只检查本地文件并显示计划，不接触串口。首装包包含 `bootloader`、分区表和应用三部分，不需要安装 ESP-IDF 或复制开发者的目录。

1. 暂时断开 Tab5 USB-A 上的 FM-1，将电脑接到 Tab5 USB-C。
2. 在 Windows 设备管理器中查看 Tab5 对应串口；下面的 `COM7` 只是例子，请换成自己的端口。
3. 如果自动进入下载模式失败，按住 Tab5 Reset 约 2 秒，直到内部绿灯快速闪烁后松开。操作依据 [官方恢复指南](https://docs.m5stack.com/en/guide/tab5/restore_factory)。
4. 执行一次安装命令：

```powershell
python install.py --mode first-install --port COM7 --board m5stack-tab5 --install
```

首装会替换 Tab5 当前系统和分区表；保留的旧分区数据不等于可以恢复原系统。工具会检查芯片、物理 Flash 容量与包内容，不使用整片擦除，不操作 FM-1、SD 卡或 Tab5 内部 ESP32-C6 的固件。等待烧录完成并重启，画面应显示 M1n1Le55 0.31。

已经安装过同一布局的 M1n1Le55，可使用 `--mode app-update` 只更新应用；工具先读回比较引导程序和分区表，不匹配时停止。首次安装不要只把应用写到 `0x10000`。

## 3. 第一次连接和出声

1. 给 Tab5 和 FM-1 稳定供电，将 FM-1 的 USB 数据接口接到 Tab5 USB-A。
2. 打开 **设备 → 连接**，确认 USB 已连接且读到 FM-1 固件身份。
3. 打开 **工作台**。原厂固件会显示对应的 FM / VOICE 等页面；读取成功后显示当前音色，控件才可编辑。
4. 在 FM-1 自身播放或通过已有 MIDI 控制器触发音符，从 FM-1 的音频输出听声音。修改一个参数，确认效果后即可继续。

USB 不识别时先换数据线、核对 USB-A 接口和供电，再重插 FM-1。无需通过刷固件来解决线材或端口问题。USB 与蓝牙可能连接不同的 FM-1，采样前应核对当前蓝牙目标。

四个入口的分工：**固件管理**切换 FM-1 固件；**工作台**控制当前固件；**音色**进入音色/采样相关操作；**设备**只有连接、备份和记录。

## 4. 准备 SD 卡和固件库

只用原厂音色功能可以跳过这一步。需要换固件时，建议准备 **官方 V15 + 想安装的目标固件**，系统在必要时自动使用 V15 中转。已有 V15 且路径允许时直接安装目标，不要求手动分两次刷。

从原作者发布页下载固定版本 `.fwsc`，不要选择“最新版本”后假定一定兼容：

| 固件 | 0.31 固定版本 | 下载来源 |
| --- | --- | --- |
| 原厂 | V15 | [M-VAVE 下载页](https://www.cuvave.com/download) |
| Felucca | 1.6.1 | [作者发布页](https://github.com/hugelton/Felucca/releases/tag/v1.6.1) |
| SLOOP | 2.7 | [作者发布页](https://github.com/isod89/sloop-fm1/releases/tag/v2.7) |
| FM-1+VA | 101 | [作者安装页](https://baudgirl.com/work/FM-1%2BVA/install) |
| FoMni | 0.4 | [作者发布页](https://github.com/charlesvestal/fm1-fomni/releases/tag/v0.4) |
| X0X | 1.1.0 | [作者发布页](https://github.com/charlesvestal/fm1-x0x/releases/tag/v1.1.0) |
| Jangada | 1.0 | [作者发布页](https://github.com/zednaked/jangada/releases/tag/v1.0) |
| FM1 Open | V12 | [作者发布页](https://github.com/m-vave/fm1_open/releases/tag/V12) |

0.31 另有本地 Probe V912 / Diag V913 实验配置，不作为新玩家推荐安装项。以上固件的安装适配不代表每个功能和每条迁移路线均做过本轮实机验证。原作者若替换了同名包，摘要不匹配时工具会停止；不要绕过检查。

将卡插入电脑读卡器，在首次安装包目录执行。以下 `E:\` 代表卡的根目录，请先核对盘符：

```powershell
python prepare_sd_card.py --sd-root E:\ --package .\FM-1.fwsc --package .\sloop-2.7.fwsc
python prepare_sd_card.py --sd-root E:\ --package .\FM-1.fwsc --package .\sloop-2.7.fwsc --write
```

第一条只查看复制计划；第二条执行。工具核对内置目录中的身份、大小和 SHA-256，创建所需目录并生成 `.fwsc.sha256` 校验文件。V15 保存为自动中转所需的 `M-VAVE-FM-1-V15.fwsc`，其他包按身份命名。不会格式化、删除或覆盖不同内容的已有文件。不必自己复制摘要或手动制作校验文件。包内容由用户从作者处取得，本安装包不附带第三方 FM-1 固件。

安全弹出卡，将卡插入**关机状态**的 Tab5 后开机。**固件管理**应显示导入的固件。目录如下：

```text
fm1/
  firmware/   固件 .fwsc 与对应 .fwsc.sha256
  backups/    原厂当前音色备份
  samples/    .fm1s 采样库与采样备份
  exchange/   固件专属对象交换文件
```

手动复制 `.fwsc` 而没有校验文件时，0.31 的触屏固件列表会阻止安装；改用随包准备工具即可。开机时未插卡、卡无法挂载或使用 exFAT 时，插入可读 FAT32 卡后需重启 Tab5。不要在操作过程中拔卡。

## 5. 更换 FM-1 固件

在 **固件管理**左侧两列卡片中选择目标 → 查看右侧当前来源与目标 → 点击顶部 **安装该固件**，一次确认即可。需要 V15 时由程序自动中转，SD 缺少 V15 会在开始前说明。等待完成并读到目标身份，再进入 **工作台**；页面结构会随所刷固件切换。

不要在传输中拔线、断电或再次启动安装。失败不等于已回滚；记录页保留失败原因。若仍能识别 OTA 加载器，选择与它匹配的包恢复；黑屏或 ROM 下载状态不属于此工具的救砖范围。

备份范围要分清：原厂备份保存的是**当前 155 字节音色**，不含全部预设、效果或 FM-1 整机固件；当前没有触屏一键恢复该 JSON 音色副本的功能。自定义固件对象备份取决于该固件开放的协议。重要预设、工程和采样应使用该固件支持的导出方法另存。

## 6. 采样功能是可选扩展

新 FM-1 的原厂固件**不支持本程序的 Open FM1S 采样传输协议**。连上蓝牙不代表能读写采样；不要因为出现采样入口就重刷原厂或反复尝试上传。

仅对实现 Open app protocol 4 和采样命令的固件使用该功能。本项目的实机采样记录来自本地 V913 实验版，不能外推为所有 Open / Felucca 系固件均支持。Tab5 的 C6 固件也需要兼容 ESP-Hosted；已有记录为 C6 1.4.1，新出厂批次仍需验证。蓝牙不可用不妨碍 USB 原厂工作台和固件管理。

已有有效 `.fm1s` 文件时放到 `/fm1/samples/`，在 **设备 → 连接**连接正确的蓝牙 FM-1，然后进入 **音色 → 打开采样管理**。程序先检测协议、备份有效旧库，再执行全部八槽整库替换。默认快速完成；完整读回可选。断线失败不会自动回滚：同一连接可选 **准备恢复备份**，重连后选 **选择恢复文件**，选择 SD 上保存的 `backup-*.fm1s` 并确认当前目标。WAV 不能直接拖入使用；首次安装包不包含实验采样固件和转换工具链，仅有 WAV 的玩家暂不能用此包直接完成采样导入。

## 遇到问题时

| 现象 | 处理 |
| --- | --- |
| 找不到串口 / 烧录无法连接 | 核对数据线、USB-C、正确端口，关闭占用串口的程序，手动进入下载模式 |
| 芯片或 Flash 容量不符合 | 停止，核对是否为原版 Tab5；不要强制跳过 |
| 烧录后屏幕不亮 / 触控异常 | 记录背面屏幕驱动型号；本包支持代码包含三代驱动，但不是三代都经过实测。可按官方恢复指南恢复出厂固件 |
| SD 不可用 | 核对 FAT32、卡是否插好；重启。程序不自动格式化 |
| 固件不可安装 / 摘要不匹配 | 使用表中固定版本和随包准备工具；未知来源版本会被阻止，不能仅改文件名绕过 |
| 原厂可编辑但不能采样 | 属于协议能力差异；继续使用 USB 工作台 |
| 效果数值显示 `--` | 固件未提供读回，该值未知，不表示效果关闭 |
| 更换固件后功能按钮不可用 | 查看当前固件身份和该页协议说明；“能安装”不等于所有功能都有控制协议 |

反馈问题时提供：Tab5 屏幕型号、M1n1Le55 版本/修订、FM-1 固件身份、操作步骤和错误文字即可。公开反馈前遮住串口日志中的蓝牙地址、个人目录、音色/工程内容；不要上传完整 Flash、NVS 或个人 SD 备份。

## 固件提供者致谢

感谢以下 FM-1 固件提供者：[M-VAVE](https://www.cuvave.com/download)、[hugelton / Felucca](https://github.com/hugelton/Felucca)、[isod89 / SLOOP](https://github.com/isod89/sloop-fm1)、[Baud Girl / FM-1+VA](https://baudgirl.com/work/FM-1%2BVA/install)、[Charles Vestal / FoMni](https://github.com/charlesvestal/fm1-fomni) 与 [X0X](https://github.com/charlesvestal/fm1-x0x)、[zednaked / Jangada](https://github.com/zednaked/jangada)、[FM1 Open](https://github.com/m-vave/fm1_open)。这些第三方 FM-1 固件需从原作者处取得，下载时请使用本版列明的兼容版本。

第三方许可与来源见[第三方声明](../THIRD_PARTY_NOTICES.md)。
