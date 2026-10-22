# 从源码构建 0.31

[简体中文](BUILDING.md) · [English](BUILDING.en.md)

目标为原版 M5Stack Tab5 / ESP32-P4。先安装并激活 **ESP-IDF 5.4.2**，在其终端中进入本仓库根目录：

```text
python tools/prepare_build.py
idf.py build
```

准备脚本检查 SDK 版本，按当前 `IDF_PATH` 生成本机 `dependencies.lock`，并在 `sdkconfig` 不存在时复制 `config/sdkconfig.release`。已有配置会保留；恢复发布配置前请另存自己的配置。构建过程不连接设备或刷机。

依赖固定为 M5Stack Tab5 BSP 1.3.2、LVGL 9.6.0~1、ESP-Hosted 2.12.9 和 ESP WiFi Remote 1.3.1，完整版本及摘要见 `dependencies.lock.template`。首次配置需要联网获取 Espressif 组件，或使用已有缓存。USB 使用 ESP-IDF 5.4.2 内置组件；`main/CMakeLists.txt` 排除 ESP-Hosted 的自动初始化单元，由应用启动后初始化。

构建输出：

- `build/tab5_fm1.bin`
- `build/bootloader/bootloader.bin`
- `build/partition_table/partition-table.bin`

字体和固件图标数据已经包含在 `main/`，普通编译不需要图片原件、字体生成工具或 Pillow。第三方来源与许可见 [第三方声明](../THIRD_PARTY_NOTICES.md)。修改文案超出现有字体字集时，需要相应更新字体数据。

精简仓库不分发开发期测试、检测、打包和素材生成脚本。安装器及 SD 准备工具的 Python 源码随 [Releases 安装包](https://github.com/ZzpsS/M1n1Le55/releases/tag/v0.31-grid-1)提供。安装当前发布版请按[新玩家说明](GETTING_STARTED_0.31.md)操作。

已在独立目录使用固定 SDK 和现有依赖缓存构建通过；另一台电脑无缓存下载依赖尚未实测。重编译文件会受时间、路径和工具环境影响，摘要可能与发布镜像不同。`.gitattributes` 保留原始换行，避免改变已发布文件和许可证的字节内容。
