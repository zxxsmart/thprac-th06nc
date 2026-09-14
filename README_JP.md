# thprac-th06nc

[English](README.md) | [简体中文](README_CN.md)

[thprac](https://github.com/touhouworldcup/thprac) の**非公式フォーク**です。Steam 版『東方紅魔郷：New Classic』の練習機能を追加します。

**[最新版をダウンロード](https://github.com/zxxsmart/thprac-th06nc/releases/latest)** · [不具合報告](https://github.com/zxxsmart/thprac-th06nc/issues)

ソースのバージョン: **1.1.0**。ベース: thprac **2.3.1.1**。

## 使い方

1. Releases から `thprac-th06nc.exe` をダウンロードします。DLL の別途ダウンロードは不要です。
2. ランチャーで New Classic の Steam インスタンスを選択し、thprac を適用して起動します。
3. ゲームで **Practice Start → 難易度・機体・ステージ → 練習設定 → Z** と進みます。

ゲームがすでに起動している場合は、ランチャーのツール画面から実行中のゲームに thprac を適用できます。適用後はゲームのメインメニューに戻り、Practice Start を選択してください。

**試験的な低遅延表示：**起動前に「低遅延モードで起動」を有効にします。初期設定はオフで、練習機能とは独立して使用できます。画面設定は **Borderless** を推奨し、**Window** にも対応します。高リフレッシュレートは Windows 側で設定してください。表示待ちを減らしますが、ティアリングが発生する場合があります。ゲーム速度とリプレイの仕様は変わりません。排他的な Full Screen など非対応の表示モードでは通常の表示方式に戻ります。ランチャーの設定変更後はゲームを起動し直してください。

仕組み・高リフレッシュレート・制約については[低遅延モードの詳細（中国語）](LOW_LATENCY.md)を参照してください。

対応: **Windows x64 / Steam th06nc 1.03**。ゲーム EXE のハッシュを確認します。ゲーム更新後は新しい対応版が必要になる場合があります。ゲーム本体は別途購入・インストールしてください。

ボス選択 73 項目、道中 40 区間、追加 Extra スペルカード 3 枚に対応。ランクは標準でゲーム本来の固定値を使います。Esc でポーズ、Backspace で F1–F7 の状態表示、F11 で速度設定。言語はランチャーの設定に従います。

開始前の設定画面では数値入力中でも Z で値を確定して練習を開始できます。数値入力中の Backspace は文字の削除のみ行います。

リプレイはゲーム本来の保存確認から保存します。`.rpy` と同名の `.rpy.thprac-nc` を一緒に保管してください。

## 更新とビルド

更新は**手動**です。このフォークの Releases から新しい EXE を入手し、ゲームとランチャーを終了してから置き換えてください。バックグラウンドの更新確認や自動置換は行いません。

Visual Studio 2022 C++ / ClangCL / Windows SDK、CMake 3.24+、Python 3.8+ を使用します。

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -SingleFile -Python python
```

出力: `build/single/thprac-th06nc.exe`。

## ライセンス

元の thprac は Ack および貢献者による [MIT ライセンス](LICENCE)のソフトウェアです。[第三者ライセンス](THIRD_PARTY_NOTICES.txt)も参照してください。本ソフトウェアは FreeType Team の成果を利用し、FreeType を FTL に従って使用しています。

このツールは非公式のファン制作物です。ゲーム開発者・販売元、thprac 上流、TWC の公式製品・公認ビルドではありません。ゲーム本体・画像・音楽・フォントは配布物に含みません。ゲームに関する権利は上海アリス幻樂団および各権利者に帰属します。

NC 対応の開発には AI 支援を使用しました。問題はこのフォークの Issues に報告してください。[変更履歴](CHANGELOG.md)。
