# Changelog

## 1.0.2 — 2026-09-11

- Support attaching to an already running New Classic game through the launcher's Tools page and `--attach PID`.
- Recognize native 64-bit NC processes without relying on the original games' mutexes or 32-bit memory layout.
- Wait for the bridge to finish attaching before reporting success. Reattaching reuses the loaded module and synchronizes the launcher language.

## 1.0.1 — 2026-09-11

- Fix incorrect Extra spell names and out-of-bounds access in stage-four midboss, boss, nonspell and spell menus for all four shot types.
- Separate menu label indices from practice/replay section IDs, preserving v1.0.0 replay compatibility.
- Reset and normalize section selections when changing stage, shot branch or difficulty, including returning from a longer Extra menu.

## 1.0.0 — 2026-09-11

First public release of the unofficial thprac-th06nc fork, based on upstream
thprac 2.3.1.1 (`55e6ed1336621e3099e7df1138acc360cf0500fa`).

- Add Steam th06nc 1.03 support through the existing launcher and in-game UI.
- Provide 73 boss selections and 40 stage portions, including three new Extra spells.
- Use normal stage-practice progression and native replay-save confirmation.
- Keep native Rank by default; support explicit custom Rank and current-value resource locks.
- Center pause controls, compact the keyboard-only Backspace panel, and show the cursor in editable menus.
- Follow launcher language and use official New Classic English/Japanese spell names.
- Ship one EXE with verified native-module extraction and bundled license notices.
- Use independent version numbering, fork project/issues links and manual Releases-page updates.
- Remove upstream update checks, downloads, replacement and legacy update commands.

NC replay metadata: v7. Local IPC: v9. Earlier unpublished development replay
formats are not supported. Game EXE SHA-256 is checked before injection.
