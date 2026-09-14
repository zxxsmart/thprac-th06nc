# thprac-th06nc 1.1.0：红魔乡新典非官方分支

[thprac 的非官方分支](https://github.com/zxxsmart/thprac-th06nc)，基于 [touhouworldcup/thprac](https://github.com/touhouworldcup/thprac) 2.3.1.1。新典的启动入口、选段设置和暂停菜单均接入现有 thprac；其他游戏使用原有模块。

## 使用

单文件发行版只需分发并运行 `thprac-th06nc.exe`，选择「东方红魔乡新典」的 Steam 实例，勾选应用 thprac 并启动。游戏中选择 **Practice Start → 难度、机体、单面 → thprac 练习选项 → Z 确认**。

启动前可独立勾选 **“以低延迟模式启动（实验性）”**，默认关闭，下一次启动游戏时生效。建议游戏画面设置使用 **Borderless**，也支持 **Window**；高刷显示器沿用 Windows 的桌面刷新率。此模式使用 Flip Model 和允许撕裂的画面提交，保留 60 FPS 游戏逻辑及原有录像格式。不支持的显示配置（包括独占 Full Screen）会回退到普通显示并提示。该选项不修改显卡驱动设置，不需要另行安装 vpatch。

机制、实现、高刷行为及风险见独立的[低延迟模式说明](https://github.com/zxxsmart/thprac-th06nc/blob/v1.1.0/LOW_LATENCY.md)。

选段使用正常关卡练习时间轴；选择符卡后仍保留该关卡后续阶段。Start、Extra Start、游戏自带的 Spell Practice 和无练习参数的录像使用原生流程。

游戏已经启动时，使用启动器中的 **工具 → 应用 thprac 至正在运行的游戏**，确认应用后回到游戏主菜单，再进入 Practice Start。也支持命令行 `thprac-th06nc.exe --attach PID`；附加时同样检查游戏版本，并通过 64 位桥接程序加载模块。

- 练习菜单沿用 TH06：关卡、道中分段、道中 Boss、关底 Boss、非符、符卡、指定帧，以及残机、Bomb、分数、火力、擦弹、点数。
- Rank 默认保持新典原生固定值：Easy 20，Normal／Hard／Lunatic 32，Extra 18。默认数值栏不可编辑；勾选“自定义 Rank”后可指定 0–99，首次开启从当前难度的原生值开始。暂停调整后重开生效，取消自定义并重开则恢复原生值。
- 四面支持假机体分支；Extra 原版最后一符支持阶段选择，并追加新典的三张新符卡；相应入口可选择保留对话。三面新增敌人包含在原有“道中后半 #3”中。
- Esc 打开练习暂停菜单，可继续、退出、重开或调整选项。调整后选择重开应用参数。
- 开始前编辑数字时，按 Z 提交数值并进入练习；编辑数字期间，Backspace 只删除文字，不切换辅助面板。
- F1–F7：无敌、锁残机、锁 Bomb、锁火力、锁定时间、自动 Bomb、重开保留音乐。资源锁保持开启时的当前数值。Backspace 仅显示这些开关；F11 调整速度。
- 暂停选择“结束游戏”或本次练习打完后，进入游戏原生的录像保存询问。选择“是”、槽位、名字并确认后，写入 `.rpy` 和同名 `.rpy.thprac-nc` 参数文件；选择“否”不保存。两者须一起保留，并用此分支播放。录像采用 v7 参数格式，记录原生／自定义 Rank 及起始数值，不兼容此前未发布的开发版录像。
- 练习设置、暂停和 F11 界面显示鼠标指针；Backspace 面板仅显示快捷键状态，不显示指针，宽度随文字收缩。暂停菜单对齐新典居中的游戏版面。关卡标题和开局无敌只保留在从头开始时；跳到后续段落时不再出现。
- 新典练习 UI 跟随启动器的中文、英语或日语设置。切换后重新启动或连接游戏即可应用。英日文游戏名称及符卡名称使用新典官方文本，包含新增的三张 Extra 符卡和原有符卡在新典中的改名；旧版 TH06 的名称保留原样。

单文件版内嵌桥接程序、模块、字体依赖、说明和许可证。启动新典时自动释放到 `%LOCALAPPDATA%\thprac\th06nc\<内容摘要>\`，逐项校验并恢复缺失或损坏的文件。用户不需要手动放置 DLL，程序也不将依赖释放到游戏安装目录。该目录是可重新生成的缓存；关闭游戏后可以清理。

启动器仍是原有 32 位 thprac，内嵌桥接程序负责加载新典所需的 64 位模块。开发用多文件目录 `build\thprac-nc` 也保留，其中的 `thprac.exe` 仍需要同目录的桥接程序和 DLL。

本分支只提供手动更新：程序中的“下载本分支更新”打开[本仓库 Releases](https://github.com/zxxsmart/thprac-th06nc/releases/latest)。下载新 EXE，关闭游戏和启动器后替换。单文件版和多文件版均不再检查、下载或安装上游更新。项目主页、反馈入口均指向本分支，上游链接另有明确标识。当前源代码版本为 1.1.0；上游版本号单独显示，保留原游戏录像版本语义。

原项目按 MIT 许可使用；完整版权与第三方许可见 `LICENCE` 和 `THIRD_PARTY_NOTICES.txt`，单文件版内嵌并可从许可证界面查阅。本软件部分基于 FreeType 团队的工作，按 FTL 使用 FreeType。本工具不代表游戏开发商、发行商、thprac 上游或 TWC，不包含游戏程序、资源包、图像、音乐或字体文件。新典适配使用了 AI 辅助开发。

当前适配本地 Steam **th06nc 1.03**，EXE SHA-256：`07850c8c6e469c0e82c13423e6d0d096a88d693455bdacacbb44c0aa3bcce473`。启动时核对完整哈希；游戏更新后需要重新适配地址及脚本映射。

## 构建与源代码

安装 Visual Studio 2022 C++、ClangCL、Windows SDK 和 CMake 3.24 或更新版本，在仓库根目录运行 `powershell -ExecutionPolicy Bypass -File .\build.ps1`。产物在 `build\thprac-nc`。

生成单文件版本还需 Python 3.8 或更新版本（只使用标准库）：`powershell -ExecutionPolicy Bypass -File .\build.ps1 -SingleFile -Python python`，产物为 `build\single\thprac-th06nc.exe`。已构建后可添加 `-PackageOnly` 仅重新打包。`tools/package_nc_single.py` 将依赖嵌入启动器的 PE 资源，不使用额外的自解压启动器。

- 现有启动器：`thprac.sln`，`ReleaseLLVM` / Win32。
- 新典模块：根目录 `CMakeLists.txt`；实现位于 `thprac/src/thprac/th06nc`。
- `practice_ui.inl`、`pause_ui.inl`、`practice_patches.inl`、`stage_warp.inl` 改编自原有 `thprac_th06.cpp`。
- `addresses.h` 集中列出新典的 RVA。`ecl_map.h` 保存原版与新典指令、子程序和时间轴的映射，补丁实际写入前会检查指令边界。
- `tools/generate_nc_ecl_map.py` 使用本地解析数据生成映射；仓库及构建产物不包含游戏脚本或其他游戏资源。

当前提供 73 个 Boss 选段和 40 个道中入口。版本变更见 [CHANGELOG.md](https://github.com/zxxsmart/thprac-th06nc/blob/main/CHANGELOG.md)。遇到问题时，请在本仓库 Issues 中附上练习器版本、游戏版本、语言、选段和复现步骤。
