# 高级配置

之前各种文件的配置方法，参见[simple config](simple_config.md).
它仍然是一种有效的配置方法，但这里指定的结构化配置方法是首选
未来方法也支持更多功能.

两种配置均受支持，如果应用程序同时出现在两个配置中，则高级配置优先.

## 配置文件

该模块通过位于`/data/local/tmp/xbs.fgg/config.json`的json配置进行配置.
首先，你可以复制示例配置
```shell
adb shell 'su -c cp /data/local/tmp/xbs.fgg/config.json.example /data/local/tmp/xbs.fgg/config.json'
```

配置示例
```json
{
  "targets": [
    {
      "app_name" : "com.xbs.exmaple",
      "enabled" : true,
      "start_up_delay_ms": 0
    }
  ]
}
```

配置包含一个目标数组。每个目标包含要使用 Frida 注入的应用程序的配置。.

如果事情没有按预期进行，请检查`adb logcat -s ZygiskFrida`以查看是否记录了错误.

## 目标配置.

### app_name
您想要注入frida的应用程序的bundle id.

### enabled
如果设置为false，那么该模块将忽略该配置.
如果您想在维护配置的同时暂时禁用目标，这很有用.


### start_up_delay_ms
库注入延迟了此毫秒数.

有时您可能需要延迟小工具的注入。某些应用程序可能会在启动时运行检查，延迟注入可以帮助避免这些情况.

### injected_libraries
这些是将注入到进程中的库。此处指定的库将按照数组的顺序加载.

该模块包含捆绑的Frida小工具 `/data/local/tmp/xbs.fgg/libgadget.so`.\
`libgadget.so`默认架构始终是您的设备的架构.

为了方便起见，该模块还在`/data/local/tmp/xbs.fgg/libgadget32.so`处安装了一个小工具，用于注入到仅支持 32 位的 64 位设备上的应用程序中.

您可以根据官方的[Gadget Doc](https://frida.re/docs/gadget/)调整小工具的配置

如果您想使用其他 Frida 版本或其他版本，您可以将其替换为您自己的小工具的路径.

使用此功能，您还可以在小工具旁边注入任意库，或者在小工具之外注入任意库（如果您删除它）.
确保您在此处提供的库已设置正确的文件权限，并且应用本身可以访问.

该模块在安装时会设置完整`xbs.fgg`目录中的文件权限。如果您怀疑存在文件权限问题，一个简单的检查方法是将您的库文件放入`xbs.fgg`目录中，然后重新安装该模块（无需卸载）。.

创建如下的小工具配置`/data/local/tmp/xbs.fgg/libxbs.config.so`.
请参阅[Gadget Doc](https://frida.re/docs/gadget/)以供参考.
```
{
    "targets": [
        {
            "app_name" : "com.xunmeng.pinduoduo",
            "inject_mode" : "listen",
            "enabled" : true,
            "start_up_delay_ms": 0
        }
    ]
}
```

请注意 `on_port_conflict: pick-next`，当父进程分叉多个子进程时，这一点非常重要.

由于这是一个非默认端口的小工具，您可以查看“adb logcat -s Frida”来查看子小工具在哪些端口上启动。.

然后您可以通过例如以下方式连接
```shell
adb forward tcp:27042 tcp:28000
frida -R gadget -l hook.js
```
