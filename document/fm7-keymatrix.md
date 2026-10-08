# FM-7 キーボード キーマトリクス対応表

> 出典: ユーザー提供の「キーボード・インターフェース」配線図（スキャン線 × 読み取り線、確定版）
> 作成日: 2026-09-21 / 更新: 2026-10-04（↑↓の位置修正、GRAPH 組み合わせとホストモードを追記）

## マトリクス構成

- **スキャン線（SCAN）**: 0〜15（16本、出力）— コネクタ pin 1〜16
- **読み取り線（SENSE）**: 0〜7（8本、入力）— コネクタ pin 17〜24
- **BREAK**: 独立（コネクタ pin 25）

## SCAN × SENSE 対応表

空欄はキー未接続（そのSCAN×SENSEの交差点にキーがない）。

| SCAN → | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **SENSE 0** | ESC | PF1 | PF2 | PF3 | PF4 | PF5 | PF6 | PF7 | PF8 | PF9 | PF10 | | | | | |
| **SENSE 1** | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | - | ^ | ¥ | INS | CLS | |
| **SENSE 2** | Q | W | E | R | T | Y | U | I | O | P | @ | [ | BS | EL | DUP | CAP |
| **SENSE 3** | A | S | D | F | G | H | J | K | L | ; | : | ] | ENTER | DEL | HOME | CTRL |
| **SENSE 4** | Z | X | C | V | B | N | M | , | . | / | - | | | ↑ | ↓ | SHIFT |
| **SENSE 5** | TAB | | | | SPACE | = | Num9 | Num8 | Num7 | Num4 | Num5 | Num6 | Num, | ← | → | GRAPH |
| **SENSE 6** | | | | | | Num- | Num+ | Num/ | Num* | Num1 | Num2 | Num3 | NumENT | Num0 | Num. | カナ |
| **SENSE 7** | | | | | | | | | | | | | | | | |

## 注記

- **SENSE 7（コネクタ pin24）には、キーが1つも接続されていない（完全に空）。**
- SCAN 15 / SENSE 2 は確定版図面で **CAP**（CapsLock）と確認。DUP は SCAN 14 / SENSE 2。（2026-09-22：実機テスターでの追加検証を試みたが、CAPキー押下でSENSE0–6×SCAN0–15の全組み合わせを確認しても反応なく、実機での裏付けは取れず。図面記載どおりの値を採用）
- **↑ は SCAN 13 / SENSE 4、↓ は SCAN 14 / SENSE 4。** 図面ではそれぞれ SCAN 12 / SCAN 13 に書かれていたが、2026-10-04 に実機（Mac接続）で「↑キーで↓が出る／↓キーは無反応」となったため1列ずらして修正した（SCAN 12 / SENSE 4 は空き）。
- テンキーの数字（Num0〜Num9）とメインキーの数字（0〜9）は別のキースイッチ。
- SPACE は **SENSE 5, SCAN 4** の位置（キートップ無地のため図面上は空欄枠で表現）。
- SENSE 4 / SCAN 10 は「-」（マイナス）。メインの「-」（SENSE 1 / SCAN 10）とは別の物理キーだが、`fm7keymap.h`（変更しない）には「-」を出すHIDコードが 0x2D しかないため、このキーも同じ 0x2D を送出する。
- SENSE 5 / SCAN 12 は「,」（カンマ）。メインのカンマキー（SENSE 4 / SCAN 7）とは別の物理キーだが、`fm7keymap.h`（変更しない）には「,」を出すHIDコードが 0x36 しかないため、このキーも同じ 0x36 を送出する。

## USB HIDコード対応表（fm7emulator互換）

fm7emulator の `fm7keymap.h`（本リポジトリ未収録）にある `fm7usbcode[]`（USB HID Usage IDで直接インデックスされたFM-7変換テーブル）に合わせて、各物理キーに送出すべきHID Usage IDをまとめたもの。実装は [src/fm7_hid_map.h](../src/fm7_hid_map.h)。

| SCAN → | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **SENSE 0** | 0x29 | 0x3A | 0x3B | 0x3C | 0x3D | 0x3E | 0x3F | 0x40 | 0x41 | 0x42 | 0x43 | | | | | |
| **SENSE 1** | 0x1E | 0x1F | 0x20 | 0x21 | 0x22 | 0x23 | 0x24 | 0x25 | 0x26 | 0x27 | 0x2D | 0x2E | 0x89 | 0x49 | 0x4E | |
| **SENSE 2** | 0x14 | 0x1A | 0x08 | 0x15 | 0x17 | 0x1C | 0x18 | 0x0C | 0x12 | 0x13 | 0x2F | 0x30 | 0x2A | 0x4B | 0x4D | 0x39 |
| **SENSE 3** | 0x04 | 0x16 | 0x07 | 0x09 | 0x0A | 0x0B | 0x0D | 0x0E | 0x0F | 0x33 | 0x34 | 0x31 | 0x28 | 0x4C | 0x4A | mod:Ctrl |
| **SENSE 4** | 0x1D | 0x1B | 0x06 | 0x19 | 0x05 | 0x11 | 0x10 | 0x36 | 0x37 | 0x38 | 0x2D | | | 0x52 | 0x51 | mod:Shift |
| **SENSE 5** | 0x2B | | | | 0x2C | 0x53 | 0x61 | 0x60 | 0x5F | 0x5C | 0x5D | 0x5E | 0x36 | 0x50 | 0x4F | mod:Alt(GRAPH) |
| **SENSE 6** | | | | | | 0x56 | 0x57 | 0x54 | 0x55 | 0x59 | 0x5A | 0x5B | 0x58 | 0x62 | 0x63 | 0x88 |
| **SENSE 7** | | | | | | | | | | | | | | | | |

BREAK（独立ピン）: `0x48`（Pause）。

**GRAPH との組み合わせキー**（2026-10-04 追加）：GRAPH を押しながら EL で **F12（`0x45`）**、GRAPH を押しながら DUP で **F11（`0x44`）** を送る（全モード共通）。単体の EL・DUP は表のとおり PageUp・End のまま。GRAPH+カナ・GRAPH+SPACE はホストモードによって送るコードが変わる。

| 操作 | FM7モード | Windowsモード | Macモード |
|---|---|---|---|
| GRAPH+カナ | LANG1（`0x90`、Mac の「かな」） | 半角/全角（`0x35`） | LANG1（`0x90`） |
| GRAPH+SPACE | LANG2（`0x91`、Mac の「英数」） | 半角/全角（`0x35`） | LANG2（`0x91`） |

LANG1／LANG2 は切り替え式ではなく、押せば必ずそのモードになる。半角/全角は Windows 側でトグルとして扱われる。fm7emulator では GRAPH+SPACE も単なるスペースなので、置き換えによる機能の欠落はない。fm7emulator はメニューの表示と終了を F12 で切り替えるため、FM-7 キーボードから GRAPH+EL でメニューを操作できるようにした。組み合わせが成立している間は GRAPH の modifier（Alt）をレポートから外し、ホストには単なる F11/F12 として届ける。実装は [src/fm7_hid_map.h](../src/fm7_hid_map.h) の `fm7GraphCombos` と [src/main.cpp](../src/main.cpp) の `buildReport()`。

**カナ単体**は FM7 モードでは fm7emulator 用に International2（`0x88`）を送る。Windows／Mac モードではキーコードではなく GUI 修飾キー（Windows キー／Command）として送る。ホストモードは CTRL+GRAPH+PF1／PF2／PF3 で切り替える（[設計仕様書 §5.3](./fm7-usb-keyboard.md#53-キーマトリクススキャン)、[取扱説明書 第5章](./fm7-usb-keyboard-product-manual.md#5-ホストモードの切替)）。

SENSE4/SCAN10 と SENSE5/SCAN12 は、それぞれメインの「-」（SENSE1/SCAN10, 0x2D）・「,」（SENSE4/SCAN7, 0x36）と同じHIDコードを送出する（`fm7keymap.h`は変更しないため、既存のコードを再利用する）。CTRL/SHIFT/GRAPHはキーコードではなく HIDレポートの modifier バイトで送る。
