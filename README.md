# fm7-usb-keyboard

富士通 FM-7 のキーボードを USB HID キーボードに変換するファームウェアです。
Raspberry Pi Pico（RP2040）を FM-7 キーボードの 34 ピンコネクタにつなぎ、
PC / Mac や fm7emulator からそのまま使えるようにします。

- USB の製造元 / 製品名: `Fusion Elements` / `FM7USBKB_001`
- 3.3V だけで配線しています（レベルシフタは不要です）

> **最初にお読みください**
>
> 1. **Pico と PC は USB ケーブルで直接つないでください。** ハブやドック経由では、書き込み後や起動時に認識されないことがあります。
> 2. **PC 側のキーボード配列を日本語（JIS、106/109 キー）にしてください。** 英語配列のままだと、記号が刻印どおりに入力されず、半角/全角の切替もできません。設定方法は[取扱説明書](document/fm7-usb-keyboard-product-manual.md)の冒頭を参照してください。

## 特長

- **3 つのホストモード**: Fm7（fm7emulator 用）/ Windows / Mac。キー操作で切り替えられ、選んだモードは電源を切っても保持されます。
- **ゴースト対策**: FM-7 のキーボードにはキーごとのダイオードがありません。そのため、押していないキーが誤って入力される組み合わせを検出し、その間は直前のレポートを保持します。
- **LED 連動**: ホストからの LED 状態に合わせて CAPS / KANA ランプを点灯します。
- **診断ファームウェア**: キーマトリクスの配線をシリアル経由で確認できます。

## ドキュメント

| 資料 | 内容 |
| --- | --- |
| [製品仕様書兼取扱説明書](document/fm7-usb-keyboard-product-manual.md) | 製品仕様、接続方法、モード切替、キー割当、困ったときは |
| [設計仕様書](document/fm7-usb-keyboard.md) | 回路、GPIO 割当、ソフトウェア仕様、PCB 仕様 |
| [キーマトリクス対応表](document/fm7-keymatrix.md) | SCAN×SENSE のキー配置と、送信する HID コード |

## 必要なもの

- Raspberry Pi Pico（RP2040）
- FM-7 キーボードユニット
- CAPS / KANA LED 駆動用の NMOS FET 2 個（Q1, Q2）と 470Ω 抵抗 2 本
- [PlatformIO](https://platformio.org/)（VS Code 拡張機能、または CLI）

## 配線

| FM-7 コネクタ | 信号 | Pico GPIO | 備考 |
| --- | --- | --- | --- |
| 1–16 | SCAN0–15 | GP0–GP15 | オープンドレイン駆動（アクティブ low） |
| 17–23 | SENSE0–6 | GP16–GP22 | 内部プルアップ（アクティブ low） |
| 24 | SENSE7 | — | 未配線（キーが接続されていないため） |
| 25 | BREAK | GP27 | マトリクスとは独立。内部プルアップ |
| 29 | INSERT LED | — | 未配線。INS 状態は Pico のオンボード LED（GP25）に表示 |
| 30 | KANA LED | GP28 | NMOS（Q2）+ 470Ω 経由。GPIO high で点灯 |
| 31 | CAPS LED | GP26 | NMOS（Q1）+ 470Ω 経由。GPIO high で点灯 |

ピン割り当ての定義は [src/pins.h](src/pins.h) にあります。

## ビルドと書き込み

`platformio.ini` には 3 つの環境があります。

| 環境 | 内容 |
| --- | --- |
| `pico` | 変換器ファームウェア。6KRO のブートプロトコルレポート（**デフォルト**） |
| `pico_nkro` | 変換器ファームウェア。21 バイトの NKRO ビットマップレポート |
| `pico_diag` | シリアル経由のマトリクス診断。USB HID なし |

```sh
# ビルド
pio run -e pico

# 書き込み（Pico の BOOTSEL ボタンを押しながら USB を接続しておく）
pio run -e pico -t upload
```

fm7emulator は 8 バイトのブートレポート（`hid_keyboard_report_t`）を前提にしています。fm7emulator で使う場合は `pico` 環境を使ってください。
`pico_nkro` は 6 キーの同時押し上限を嫌うホスト向けです。ただしダイオードのないマトリクスのため、真の N キーロールオーバーにはなりません。

## 使い方

### ホストモードの切り替え

**CTRL + GRAPH + PF1 / PF2 / PF3** で切り替えます。切り替えると CAPS / KANA ランプが点滅し、点滅の回数でモードを確認できます。起動時にも同じ点滅で現在のモードを表示します。

| 操作 | モード | 点滅回数 | KANA キー | GRAPH キー |
| --- | --- | --- | --- | --- |
| CTRL+GRAPH+PF1 | Fm7 | 1 回 | International2（fm7emulator でカナ切り替え） | Alt |
| CTRL+GRAPH+PF2 | Windows | 2 回 | Windows キー | Alt |
| CTRL+GRAPH+PF3 | Mac | 3 回 | Command | Option |

### GRAPH との組み合わせ

| 操作 | Fm7 | Windows | Mac |
| --- | --- | --- | --- |
| GRAPH + EL | F12 | F12 | F12 |
| GRAPH + DUP | F11 | F11 | F11 |
| GRAPH + KANA | LANG1（かな） | 半角/全角 | LANG1（かな） |
| GRAPH + SPACE | LANG2（英数） | 半角/全角 | LANG2（英数） |

組み合わせを押している間、GRAPH（Alt）自体はホストに送りません。fm7emulator のメニューは GRAPH + EL（F12）で開閉できます。

### その他のキー

- **BREAK** は Pause キーとして送られます。
- そのほかのキーの対応は [src/fm7_hid_map.h](src/fm7_hid_map.h) を参照してください。

### LED

| ランプ | Fm7 モード | Windows / Mac モード |
| --- | --- | --- |
| CAPS | Caps Lock | Caps Lock |
| KANA | Num Lock（fm7emulator がカナ状態を送る） | HID の Kana ビット |
| INS（オンボード LED） | Scroll Lock | Scroll Lock |

## 診断ファームウェア

キーボードの配線やキーマトリクスの対応を確認するときに使います。

```sh
pio run -e pico_diag -t upload
pio device monitor -e pico_diag   # 115200 bps
```

キーを押すと、押した / 離したタイミングでスキャン位置とキー名が表示されます。シリアルから次のコマンドを入力できます。

| キー | 動作 |
| --- | --- |
| `d` | 現在のマトリクスをグリッド表示 |
| `g` | ゴーストテストの説明を表示 |
| `h` | ヘルプを表示 |

## ファイル構成

```
document/             製品仕様書・設計仕様書・キーマトリクス対応表
src/
├── main.cpp          変換器 / 診断ファームウェアの本体
├── pins.h            コネクタと GPIO の対応
├── fm7_matrix.*      マトリクススキャン、チャタリング除去、ゴースト検出
├── fm7_usb.*         USB HID レポートの送受信（6KRO / NKRO）
├── fm7_hid_map.h     キー位置 → HID キーコードの変換表、モード定義
└── fm7_key_names.h   診断用のキー名表
```
