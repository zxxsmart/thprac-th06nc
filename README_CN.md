# thprac-th06nc

[English](README.md) | [日本語](README_JP.md)

**[thprac](https://github.com/touhouworldcup/thprac) 的非官方分支**，加入 Steam《东方红魔乡：新典》的练习支持。

**[下载最新版](https://github.com/zxxsmart/thprac-th06nc/releases/latest)** · [反馈问题](https://github.com/zxxsmart/thprac-th06nc/issues)

当前源代码版本 **1.1.0**，基于 thprac **2.3.1.1**，上游基线 `55e6ed1336621e3099e7df1138acc360cf0500fa`。

## 使用

下载 Release 中的 `thprac-th06nc.exe`，运行后选择新典的 Steam 实例、勾选应用 thprac 并启动。无需另行下载 DLL。

游戏中选择 **Practice Start → 难度、机体、单面 → 练习选项 → Z 确认**。

已经启动游戏时，可在启动器中选择 **工具 → 应用 thprac 至正在运行的游戏**。应用完成后回到游戏主菜单，再进入 Practice Start。

**实验性低延迟显示：**启动前勾选“以低延迟模式启动”。默认关闭，可与练习功能独立使用。游戏画面设置建议选 **Borderless**，也支持 **Window**；高刷显示器请在 Windows 中设置所需刷新率。此功能减少显示等待，可能出现撕裂，不改变游戏速度和录像规则。不支持的显示模式（包括独占 Full Screen）回退到普通显示。更改启动器选项后需重新启动游戏。

机制、实现、高刷行为及风险见独立的[低延迟模式说明](LOW_LATENCY.md)。

当前支持 **Windows x64、Steam th06nc 1.03**，启动时核对完整 EXE 哈希。游戏更新后可能需要新的适配版本。游戏须自行购买和安装。

- 正常关卡练习流程；73 个 Boss 选段、40 个道中入口，包含三张新增 Extra 符卡。
- 初始资源可调，Rank 默认保持新典原生值，也可手动开启自定义 Rank。
- Esc 暂停；Backspace 显示紧凑的 F1–F7 状态面板，不显示鼠标指针；F11 调整速度。
- 开始前编辑数字时，按 Z 提交数值并进入练习；编辑数字期间，Backspace 只删除文字。
- 练习 UI 跟随启动器中英日语言，英日符卡名称使用新典官方文本。
- 退出或完成练习后使用游戏原生录像保存确认。`.rpy` 和同名 `.rpy.thprac-nc` 须一起保留；参数格式为 v7。

## 更新和构建

更新采用**手动下载**：点击“下载本分支更新”打开本仓库 Releases，下载新 EXE，关闭游戏和启动器后替换。没有后台版本检查、自动下载或自动替换。

启动新典时，内嵌依赖释放并校验到 `%LOCALAPPDATA%\thprac\th06nc\<内容摘要>\`。字体从本机 Windows 读取。

安装 Visual Studio 2022 C++、ClangCL、Windows SDK、CMake 3.24+、Python 3.8+ 后运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -SingleFile -Python python
```

产物：`build/single/thprac-th06nc.exe`。构建已有适配不需要游戏解包文件。

[详细说明](README_NC.md) · [版本记录](CHANGELOG.md)

## 来源与许可

原版 thprac 版权所有者为 Ack 和 thprac 贡献者，采用 [MIT 许可证](LICENCE)。第三方声明见 [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt)。本软件部分基于 FreeType 团队的工作，按 FTL 使用 FreeType。

本工具是非官方同人练习工具，不代表游戏开发商、发行商、thprac 上游或 TWC，也不宣称获得比赛使用认可。东方 Project 及游戏相关权利归上海爱丽丝幻乐团等相应权利人所有。发行文件不包含游戏 EXE、资源包、图像、音乐或字体文件。

新典适配开发使用了 AI 辅助。问题请提交到本分支的 Issues；其他游戏模块保留上游实现。
