# ZygiskFrida

> [Frida](https://frida.re) 是面向开发人员、逆向工程师和安全研究人员的动态检测工具包

> [Zygisk](https://github.com/topjohnwu/Magisk) Magisk 的一部分允许您在每个 Android 应用程序的进程中运行代码


## 介绍

[ZygiskFrida](README.md) 是一个 zygisk 模块，允许你以更隐秘的方式将 frida 小工具注入 Android 应用程序中.

- 该小工具并未嵌入到 APK 本身。因此 APK 完整性/签名检查仍将通过.
- 该进程不像 frida-server 那样被 ptrace 跟踪。避免基于 ptrace 的检测.
- 控制小工具的注射时间.
- 允许您将多个任意库加载到进程中.

如果您仍在使用带有旧版 magisk 而非 zygisk 的 riru，此 repo 还提供了 [Riru](https://github.com/RikkaApps/Riru) 风格。

## 如何使用模块

### 先决条件
- 已root的设备/模拟器
- Zygisk可用且已启用

### 快速开始
- 从发布页面下载最新版本[Release Page](https://github.com/lwjjike/ZygiskFrida/releases)\
  如果您使用的是riru而不是zygisk，请选择 riru-release。否则，请选择普通版本
- 将ZygiskFrida.zip文件传输到您的设备并通过 Magisk 安装.
- 安装后重启
- 创建配置文件并将包名称调整为您的目标应用程序（在命令中替换“app的包名”）
```shell
adb shell 'su -c cp /data/local/tmp/xbs.fgg/config.json.example /data/local/tmp/xbs.fgg/config.json'
adb shell 'su -c sed -i s/com.example.package/your.target.application/ /data/local/tmp/xbs.fgg/config.json'
```
- 启动你的应用。它会在启动时暂停，以便你附加
  例如. `frida -U -N your.target.application` or `frida -U -n Gadget`

假设您没有运行任何其他 Frida 服务器（例如，使用 MagiskFrida）。
您仍然可以将其与 Frida 服务器一起运行，但您必须配置该小工具
以使用其他端口.

### 配置

该模块还支持添加启动延迟，可以延迟小工具的注入，以避免在启动时运行检查、加载任意库和子门控.

请查看此[配置指南](docs/advanced_config.md).

## 如何构建

- 检出项目
- 运行 `./gradlew :module:assembleRelease`
- 构建的 Magisk 模块应该位于“out”目录中.

您还可以使用以下命令直接构建模块并将其安装到您的设备上 `./gradlew :module:flashAndRebootZygiskRelease`

## 注意事项

- 对于模拟器，这将在原生环境中启动小工具。这意味着您可以钩住 Java 函数，但无法钩住原生函数。.

