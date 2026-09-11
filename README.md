# thprac-th06nc

[简体中文](README_CN.md) | [日本語](README_JP.md)

An **unofficial fork of [thprac](https://github.com/touhouworldcup/thprac)** with practice support for the Steam edition of **Touhou Koumakyou: New Classic**.

**[Download the latest release](https://github.com/zxxsmart/thprac-th06nc/releases/latest)** · [Report an issue](https://github.com/zxxsmart/thprac-th06nc/issues)

Current fork version: **1.0.1**. Based on thprac **2.3.1.1**, upstream commit `55e6ed1336621e3099e7df1138acc360cf0500fa`.

## Install and use

1. Download `thprac-th06nc.exe` from this fork's Releases. No additional DLL downloads are needed.
2. Run it, select the New Classic Steam instance, enable thprac, and launch.
3. In the game, select **Practice Start → difficulty, character, stage → practice settings → Z**.

Supported game: **Steam th06nc 1.03**, Windows x64. The exact executable SHA-256 is checked before injection:

```text
07850c8c6e469c0e82c13423e6d0d096a88d693455bdacacbb44c0aa3bcce473
```

Game updates may require a new adapter release. You must own and install the game separately.

## Practice features

- TH06-style stage portions and boss attacks: 73 boss selections and 40 stage portions, including the three new Extra spells.
- Normal stage-practice flow, with later attacks continuing after the selected spell.
- Starting resources, optional custom Rank, dialogue and supported attack phases. Rank defaults to the game's native fixed value.
- Esc: resume, exit, retry or edit settings. Backspace: compact keyboard-only F1–F7 status panel. F12: game speed.
- Chinese, English and Japanese UI follows the launcher's language. New Classic's official English/Japanese spell names are used.
- Native replay-save confirmation after exiting or finishing practice. Keep both `.rpy` and `.rpy.thprac-nc`; NC replay metadata uses v7.

Other supported games use the upstream thprac modules.

## Updates

Updates are **manual**. The launcher opens this fork's Releases page in your browser. Download the new EXE, close the game and launcher, and replace your copy. There are no background version requests, automatic downloads or executable replacement.

The EXE embeds its native module, bridge, dependencies, documentation and licenses. When launching New Classic, it extracts verified files to `%LOCALAPPDATA%\thprac\th06nc\<content-hash>\`. Windows fonts are read locally at runtime.

## Source and build

Use Visual Studio 2022 with C++ tools, ClangCL and Windows SDK, CMake 3.24+, and Python 3.8+:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -SingleFile -Python python
```

Output: `build/single/thprac-th06nc.exe`. The build includes both the Win32 launcher and x64 NC module. Extracted game files are not required to compile the adapter.

[Detailed usage/build notes](README_NC.md) · [Release notes](CHANGELOG.md)

## Attribution and licenses

Original thprac: Copyright (c) 2022 Ack and thprac contributors, [MIT License](LICENCE). Third-party components retain their own licenses; see [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt). This software is based in part on the work of the FreeType Team and uses FreeType under the FTL.

Touhou Project and the supported game belong to Team Shanghai Alice and their respective rights holders. This is an unofficial fan-made practice tool, not endorsed by the game developers, publisher, thprac maintainers or TWC. No game executable, archive, graphics, music or font files are distributed with the tool.

The NC adapter was developed with AI assistance and local runtime testing. This fork has its own issue tracker and releases; upstream contribution policies remain documented in [the upstream contribution guide](docs/upstream/CONTRIBUTING.md).
