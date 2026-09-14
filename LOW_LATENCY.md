# 低延迟模式 / Low-latency mode / 低遅延モード

[简体中文](#zh) · [English](#en) · [日本語](#ja)

<a id="zh"></a>

## 简体中文

新典的可选显示优化，默认关闭，可独立于练习功能使用。

**使用：**退出游戏，在启动器的新典详情页勾选“以低延迟模式启动（实验性）”，再启动游戏。画面设置推荐 **Borderless**，也支持 **Window**；高刷显示器请在 Windows 中设置所需刷新率。取消勾选并重启游戏即可关闭。已设为默认启动时，可右键游戏条目进入详细设置。

**机制：**将原生 D3D11 拷贝式交换链换成双缓冲 **Flip Discard**，以 `Present(0)` 允许撕裂的方式提交画面。在输入采样前等待显示队列可用，并将最大帧延迟设为 1。满足条件时可使用 Independent Flip，减少桌面合成与显示等待。实现同时处理渲染目标重新绑定和窗口尺寸变化。

**高刷：**新典 1.03 的独占 **Full Screen** 会请求 60 Hz，因此推荐无边框模式沿用桌面高刷新率。游戏逻辑仍默认 60 FPS，操作判定、游戏速度和录像格式保持原样。VRR 是否生效取决于显示器与驱动设置，本功能不自动开启 G-SYNC／FreeSync。

**注意：**画面可能出现撕裂，实际改善取决于显示环境；叠加层或录屏工具也可能影响效果。不支持的配置使用原生显示方式；如遇显示异常，关闭选项并重启游戏即可。

<a id="en"></a>

## English

An optional display optimization for New Classic, off by default and independent of practice features.

**Use:** Close the game, enable “Launch with low latency (experimental)” in the launcher's New Classic details page, then launch again. **Borderless** is recommended; **Window** is also supported. Select your preferred high refresh rate in Windows. To disable the feature, uncheck it and restart the game. If direct launch is enabled, right-click the game entry to open its settings.

**How it works:** The native D3D11 copy-based swap chain is replaced with a double-buffered **Flip Discard** chain using `Present(0)` with tearing allowed. The game waits for the display queue before sampling input, with maximum frame latency set to 1. Where supported, Independent Flip reduces desktop composition and display waiting. The implementation also handles render-target rebinding and window resizing.

**High refresh rates:** New Classic 1.03 requests 60 Hz in exclusive **Full Screen**, so Borderless is recommended to retain the desktop refresh rate. Game logic remains at 60 FPS by default; input rules, game speed and replay format are unchanged. VRR depends on the display and driver settings; this feature does not automatically enable G-SYNC or FreeSync.

**Notes:** Tearing may occur, and the improvement depends on the display setup. Overlays or recording tools may also affect the result. Unsupported configurations use the native display path. If display problems occur, disable the option and restart the game.

<a id="ja"></a>

## 日本語

新典向けの任意の表示最適化です。初期設定はオフで、練習機能とは独立して使用できます。

**使い方：**ゲームを終了し、ランチャーの新典の詳細設定で「低遅延モードで起動（試験的機能）」を有効にして起動します。画面設定は **Borderless** を推奨し、**Window** にも対応します。高リフレッシュレートは Windows 側で設定してください。無効にするにはチェックを外してゲームを再起動します。直接起動が設定されている場合は、ゲームの項目を右クリックして詳細設定を開けます。

**仕組み：**元の D3D11 のコピー方式のスワップチェーンを、ダブルバッファーの **Flip Discard** に置き換え、ティアリングを許可した `Present(0)` で画面を提示します。入力取得前に表示キューの空きを待ち、最大フレーム遅延を 1 に設定します。条件が整えば Independent Flip が利用され、デスクトップ合成と表示の待ち時間を減らせます。描画先の再バインドやウィンドウサイズ変更にも対応しています。

**高リフレッシュレート：**新典 1.03 は排他的な **Full Screen** で 60 Hz を要求するため、デスクトップのリフレッシュレートを維持するには Borderless を推奨します。ゲームロジックは標準で 60 FPS のままで、操作判定・ゲーム速度・リプレイ形式は変わりません。VRR の動作はディスプレイとドライバーの設定に依存し、本機能が G-SYNC／FreeSync を自動で有効にすることはありません。

**注意：**ティアリングが発生する場合があり、効果は表示環境によって異なります。オーバーレイや録画ツールも影響することがあります。非対応の設定では元の表示方式を使用します。表示に問題がある場合は、設定を無効にしてゲームを再起動してください。
