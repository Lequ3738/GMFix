# GMFix

这是一个为 GameMaker 8.0 设计的模组，修复了 GameMaker 8.0 导出的 exe 的许多问题。使其更加兼容新的 Windows 系统版本。

实际上，该插件所做的事情和 [gm8x_fix](https://github.com/skyfloogle/gm8x_fix) 一样，只不过 gm8x_fix 是要你自己在导出游戏后手动修补，该插件是可以在 GameMaker 8.0 直接运行游戏或者导出 exe 时自动应用补丁。

该插件自动应用 gm8x_fix 中与 GameMaker 8.0 兼容的所有补丁。

## 如何使用

1. 请检查你的 GameMaker 8.0 安装目录，目录下是否有 `FoxPluginLoader.dll` 和 `FoxPlugin.txt`。该插件靠这两个文件进行加载。
2. 到 Release 下载最新版本的二进制发布，将 `GMFix.dll` 放在 `(安装目录)\FoxPlugin` 文件夹下（或你能找到的磁盘上的任意地方）。
3. 打开 `FoxPlugin.txt`，新建一行并写入 `"FoxPlugin\GMFix.dll"`（或被双引号包裹的其他路径）。并保存该文件。
4. 打开 GameMaker 8.0，你现在直接运行或者导出的游戏都应用上了补丁。

## 如何编译

使用 Visual Studio 2022 进行编译，编译时平台选择 `x86`。