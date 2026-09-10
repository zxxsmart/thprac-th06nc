# thprac：红魔乡新典分支

本地 fork 基于 [touhouworldcup/thprac](https://github.com/touhouworldcup/thprac) 的 `55e6ed1336621e3099e7df1138acc360cf0500fa`，分支为 `feature/th06nc`。新典的入口、游戏内选段菜单、暂停菜单和练习参数均接入现有 thprac；旧游戏仍使用原有模块。

## 使用

运行本目录的 `thprac.exe`，选择「东方红魔乡新典」的 Steam 实例，勾选应用 thprac 并启动。游戏中选择 **Practice Start → 难度、机体、单面 → thprac 练习选项 → Z 确认**。

选段使用正常关卡练习时间轴；选择符卡后仍保留该关卡后续阶段。Start、Extra Start、游戏自带的 Spell Practice 和无练习参数的录像使用原生流程。

- 练习菜单沿用 TH06：关卡、道中分段、道中 Boss、关底 Boss、非符、符卡、指定帧，以及残机、Bomb、分数、火力、擦弹、点数、Rank、Rank 锁定。
- 四面支持假机体分支；Extra 最后一符支持阶段选择；相应入口可选择保留对话。
- Esc 打开练习暂停菜单，可继续、退出、重开或调整选项。调整后选择重开应用参数。
- F1–F7：无敌、无限残机、无限 Bomb、无限火力、锁定时间、自动 Bomb、重开保留音乐。Backspace 显示这些开关；F12 调整速度。
- 快捷菜单中的「保存录像」写入原生 `.rpy` 和同名 `.rpy.thprac-nc` 参数文件。两者须一起保留，并用此分支播放。

需要将 `thprac.exe`、`thprac_bridge64.exe`、`thprac_th06nc.dll`、`freetype.dll` 放在同一个目录。桥接程序负责让原有 32 位启动器加载新典所需的 64 位模块。

当前适配本地 Steam **th06nc 1.03**，EXE SHA-256：`07850c8c6e469c0e82c13423e6d0d096a88d693455bdacacbb44c0aa3bcce473`。启动时核对完整哈希；游戏更新后需要重新适配地址及脚本映射。

## 构建与源代码

安装 Visual Studio 2022 C++、ClangCL、Windows SDK 和 CMake 3.24 或更新版本，在仓库根目录运行 `powershell -ExecutionPolicy Bypass -File .\build.ps1`。产物在 `build\thprac-nc`。

- 现有启动器：`thprac.sln`，`ReleaseLLVM` / Win32。
- 新典模块：根目录 `CMakeLists.txt`；实现位于 `thprac/src/thprac/th06nc`。
- `practice_ui.inl`、`pause_ui.inl`、`practice_patches.inl`、`stage_warp.inl` 改编自原有 `thprac_th06.cpp`。
- `addresses.h` 集中列出新典的 RVA。`ecl_map.h` 保存原版与新典指令、子程序和时间轴的映射，补丁实际写入前会检查指令边界。
- `tools/generate_nc_ecl_map.py` 使用本地解析数据生成映射；仓库及构建产物不包含游戏脚本或其他游戏资源。

已完成 Steam 启动、选面后的内嵌 UI、70 个 Boss 选段、40 个道中入口、特殊分支、模式隔离及练习录像的验证。具体覆盖范围、复现方法和限制见 [VALIDATION_NC.md](VALIDATION_NC.md)。
