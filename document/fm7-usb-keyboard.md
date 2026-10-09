# FM-7 キーボード USB 変換基板 仕様書

| 項目 | 内容 |
| --- | --- |
| 製品名 | FM-7 Keyboard to USB Converter Type RPPico |
| 製品番号 | FM7USBKB-001 |
| ブランド | Fusion Elements |
| 基板版数 | Rev 0.5 |
| 文書版数 | 0.5 |
| 関連資料 | [キーマトリクス対応表](./fm7-keymatrix.md)、[製品仕様書兼取扱説明書](./fm7-usb-keyboard-product-manual.md) |

## 1. 概要

FM-7 実機のキーボードユニット（34 ピンフラットケーブル接続）を、純正 Raspberry Pi Pico を用いて USB HID キーボードに変換する基板。FM-7 の筐体内への組み込み、または単体基板としての使用を想定する。

| 項目 | 仕様 |
| --- | --- |
| 制御モジュール | 純正 Raspberry Pi Pico（RP2040）、ソケット実装 |
| キー入力 | SCAN 16 本 × SENSE 7 本（SENSE0–6）＋ BREAK |
| ランプ | CAPS・かなの 2 灯（INSERT は非対応） |
| ファームウェア | arduino-pico ＋ Adafruit TinyUSB（PlatformIO） |
| 電源 | USB バスパワー（VBUS 5V）のみ |

## 2. システム構成

```
FM-7キーボードユニット
   │ (34ピン・フラットケーブル)
   ▼
[J3] 34ピンボックスヘッダ
   │
   ├─ SCAN0-15 ──┐
   ├─ SENSE0-6 ──┤   Raspberry Pi Pico (RP2040)
   ├─ BREAK ─────┤   [J1][J2] 2×1x20 メスヘッダでソケット実装
   ├─ GND ───────┤
   └─ LED(CAPS/かな) ─ NMOSドライバ×2 ─┘
                                          │ USB (micro-B, Pico実装)
                                          ▼
                                      ホストPC（HIDキーボードとして認識）
```

## 3. ハードウェア仕様

### 3.1 Raspberry Pi Pico

- 純正 Raspberry Pi Pico（RP2040）を使用する。クローン・互換ボードは対象外。
- 基板に直付けせず、2×1x20 2.54mm ピッチのメスヘッダ（J1・J2）で着脱可能に実装する。
- 電源は USB バスパワー（VBUS 5V）のみ。外部電源コネクタは設けない。
- 信号はすべて 3.3V 系で扱う。キーマトリクスは受動部品（スイッチ）のみで構成されるため、Pico の内部プルアップで 3.3V 動作させる。レベルシフタは不要。

### 3.2 FM-7 キーボードコネクタ（J3）

34 ピンボックスヘッダ（2 列×17、2.54mm ピッチ、IDC 標準ピン配置）。

| ピン | 役割 | 本数 |
| --- | --- | --- |
| 1–16 | SCAN0–15（出力） | 16 |
| 17–23 | SENSE0–6（入力、プルアップ） | 7 |
| 24 | SENSE7 — **未接続**（キーが割り当てられていない。[キーマトリクス対応表](./fm7-keymatrix.md)参照） | – |
| 25 | BREAK（入力、プルアップ） | 1 |
| 26–28 | GND | 3 |
| 29 | INSERT LED 制御 — **未接続** | – |
| 30 | かな LED 制御（シンク、0 で点灯） | 1 |
| 31 | CAPS LED 制御（シンク、0 で点灯） | 1 |
| 32–34 | LED 用 +5V | 3（すべて結線） |

FM-7 本体の回路では pin24 は SENSE7 に結線されているが、SENSE7 にはキーが 1 つも接続されていない。そのため本基板では pin24 を GPIO に接続せず、その分の GPIO を LED 制御に充てる。

### 3.3 LED（CAPS・かな）

- CAPS LOCK・かなロックの 2 灯を駆動する。INSERT（pin29）は駆動しない。
- キーボード側の LED はユニットに内蔵されており、コネクタ経由のシンク駆動（0 で点灯）。アノード側は pin32–34 から +5V（VBUS）を給電する。
- **pin32/33/34 は 3 本とも VBUS に結線する。** ユニット内部でこれらが共通か LED ごとに別かが確定できないため、1 本だけの結線で点灯しなくなるリスクを避ける。
- RP2040 の GPIO で直接駆動すると、3.3V の High と 5V 給電との電位差（約 1.7V）で消灯時に微点灯する恐れがある。そのため、各 LED を NMOS（2N7000）のオープンドレインで駆動する。
  - GPIO High → MOSFET ON → LED 制御ピンを GND にシンク → 点灯
  - GPIO Low → MOSFET OFF → LED 制御ピンはハイインピーダンス → 消灯
- 周辺抵抗：
  - ゲート直列抵抗 100Ω（R1・R2、突入電流対策）
  - LED 電流制限抵抗 470Ω（R3・R4、**必須**。キーボードユニット内の LED には電流制限抵抗がない）
  - プルアップ抵抗 10kΩ（R5・R6、+5V へ。消灯時のちらつき対策。3.3V へのプルアップでは微点灯するため不可）

```
LED駆動回路（CAPS / かな 各1回路、計2回路）:

  GPxx ── 100Ω ──┤ Gate
                   │ 2N7000
             GND ──┤ Source
                   │
  J3 pinYY ─ 470Ω ┤ Drain

  J3 pin32–34 ─── +5V (VBUS)

  CAPS: GP26 → Q1 → 470Ω → J3 pin31
  かな:  GP28 → Q2 → 470Ω → J3 pin30
```

### 3.4 BREAK キー

- コネクタ pin25。キーマトリクスから独立した入力。
- 内部プルアップで読み取り、USB HID の Pause（`0x48`）として送る。

## 4. GPIO 割当

Raspberry Pi Pico のヘッダで使える GPIO は 26 本（GP0–22、GP26–28）。必要な信号は SCAN 16 本、SENSE 7 本、BREAK 1 本、LED 2 本の計 26 本で、ちょうど収まる（予備 GPIO なし）。

| 信号 | Pico GPIO | コネクタピン |
| --- | --- | --- |
| SCAN0–15 | GP0–15 | 1–16 |
| SENSE0–6 | GP16–22 | 17–23 |
| LED_CAPS | GP26 | 31（NMOS ＋ 470Ω 経由） |
| BREAK | GP27 | 25 |
| LED_KANA | GP28 | 30（NMOS ＋ 470Ω 経由） |
| オンボード LED | GP25 | —（変換ファームでは INS 表示、診断ファームではステータス表示） |

INSERT LED（pin29）は未接続のため、INS 状態は Pico のオンボード LED で代用表示する。

## 5. ソフトウェア仕様

実装は [src/](../src/)、ビルド設定は [platformio.ini](../platformio.ini)。

### 5.1 開発環境

arduino-pico（Earle Philhower コア）＋ Adafruit TinyUSB。ビルドは PlatformIO で行う。

| 環境 | 内容 |
| --- | --- |
| `pico` | 変換ファーム。6KRO（ブートプロトコル互換）。**既定** |
| `pico_nkro` | 変換ファーム。NKRO（21 バイト＝modifier 1 ＋ ビットマップ 20、usage 0x00–0x9F） |
| `pico_diag` | シリアル診断ファーム（USB HID なし） |

USB の製造元名は `Fusion Elements`、製品名は `FM7USBKB-001`。

### 5.2 USB HID

既定のレポート形式は 6KRO とする。理由は次の 2 点。

1. **fm7emulator はブートプロトコルのレポートしか解釈しない。** fm7emulator は `hid_keyboard_report_t`（8 バイト、レポート ID なし、キー 6 個）を読み、LED 状態をレポート 0 の 1 バイトとして送る。
2. **HID インターフェースを 2 つ同時に公開すると、二重入力になる。** 通常の OS はキーボードが 2 台あると解釈するため、ビルド時にどちらか一方だけを選ぶ。

FM-7 のキーマトリクスにキーごとのダイオードがあることは確認できていない。そのため NKRO でも 3 キー以上の同時押しではゴースト抑止が必要になる。`pico_nkro` は 6 キー上限を避けたいホスト向けの選択肢であり、ロールオーバーの制約を解消するものではない。

#### LED 状態の受信

実装は [src/fm7_usb.cpp](../src/fm7_usb.cpp)、[src/main.cpp](../src/main.cpp) の `applyHostLeds()`。

| Output Report のビット | 意味 | 割当 |
| --- | --- | --- |
| bit0 Num Lock | fm7emulator はかな状態を Num Lock で送る | FM7 モード：`LED_KANA`（GP28） |
| bit1 Caps Lock | CAPS | `LED_CAPS`（GP26） |
| bit2 Scroll Lock | fm7emulator は INS 状態を Scroll Lock で送る | オンボード LED（GP25） |
| bit4 Kana | HID 標準の Kana | Windows／Mac モード：`LED_KANA`（GP28） |

Windows／Mac モードでは Num Lock を本来の意味で使うため、かなランプは Kana ビットに従う。

### 5.3 キーマトリクススキャン

実装は [src/fm7_matrix.cpp](../src/fm7_matrix.cpp)。

- **オープンドレイン駆動**：待機中の SCAN 線は INPUT（ハイインピーダンス）とし、走査中の 1 本だけ OUTPUT LOW にする。待機を High 出力にすると、2 キーが同じ SENSE 行を共有したときに駆動中の線とぶつかる。
- **整定待ち**：1 本あたり 50µs。16 本の走査は約 0.8ms。
- **デバウンス**：同じ読み値が 4 走査連続したら確定する。
- **ゴースト検出**：2 本の SCAN 線が 2 本以上の SENSE を共有したら、読み値が曖昧と判断する。その間はマトリクス部分を直前の確定状態で保持し、押されていないキーを送らない。BREAK はマトリクス外なので保持の対象外。
- **ロールオーバー**：6KRO でキーが 6 個を超えたら、全スロットを `0x01`（ErrorRollOver）にする。
- **キーコード**：対応表は [src/fm7_hid_map.h](../src/fm7_hid_map.h)（[キーマトリクス対応表](./fm7-keymatrix.md)と同一）。Shift による面替えは行わず、HID Usage ID をそのまま送る。CTRL／SHIFT／GRAPH は modifier バイトで送る。

#### ホストモード

`CTRL+GRAPH+PF1／PF2／PF3` で FM7／Windows／Mac モードを切り替える。モードはフラッシュに保存し、電源を切っても保持する。切替時と起動時には、CAPS・かなランプがモードに応じて 1／2／3 回点滅する。切替に使った PF キーはホストに送らない。

| キー・操作 | FM7 | Windows | Mac |
| --- | --- | --- | --- |
| KANA | International2（`0x88`） | GUI（Windows キー） | GUI（Command） |
| GRAPH | Alt | Alt | Alt（Option） |
| GRAPH＋KANA | LANG1（`0x90`） | 半角/全角（`0x35`） | LANG1（`0x90`） |
| GRAPH＋SPACE | LANG2（`0x91`） | 半角/全角（`0x35`） | LANG2（`0x91`） |
| GRAPH＋EL | F12（`0x45`） | F12 | F12 |
| GRAPH＋DUP | F11（`0x44`） | F11 | F11 |

GRAPH との組み合わせが成立している間は、GRAPH の modifier（Alt）をレポートから外す。

### 5.4 BREAK キー

コネクタ pin25 の独立入力。マトリクス外なのでゴーストの影響を受けない。HID Usage `0x48`（Pause）を送る。

### 5.5 診断ファームウェア

`pio run -e pico_diag -t upload` で書き込み、115200bps のシリアルで操作する。

- キーの押下・解放ごとに、SCAN／SENSE／キー名／HID Usage を出力する。[キーマトリクス対応表](./fm7-keymatrix.md)にない位置が押されると `UNDOCUMENTED POSITION` と表示する。
- `d`：マトリクス全体をグリッド表示する。
- `g`：ゴーストテストの手順を表示する。長方形を作る 3 キー（例：Q・W・A）を同時に押して `GHOSTING DETECTED` が出れば、キーごとのダイオードがないと判断できる。
- `h`：ヘルプを表示する。

## 6. PCB 仕様

| 項目 | 仕様 |
| --- | --- |
| 基板サイズ | 53.34 × 97.79 mm（2100 × 3850 mil） |
| 外形 | 四隅 R3.1mm（122 mil）の角丸。円弧の中心は取付穴の中心と一致 |
| 層数 | 2 層 |
| 実装方式 | 全スルーホール（表面実装部品なし） |
| 取付穴 | M3 用 φ3.2mm（126 mil）、非メッキ（NPTH）、ランドなし、4 か所 |
| 取付穴の位置 | 中心座標 x = 422・2278 mil、y = 28・-3578 mil（外形から 3.1mm 内側、穴縁から基板端まで 1.5mm） |
| ネジ頭・ワッシャ | 外径 φ7.5mm 以下（下側取付穴と J3 本体の距離 3.78mm から決まる） |
| ベタ GND | 表裏とも。スティッチングビア 62 個（250 mil 格子） |

### 6.1 主要部品

| 記号 | 部品 | 備考 |
| --- | --- | --- |
| J1・J2 | Pico ソケット（2.54mm ピッチ 1×20 メスヘッダ × 2） | |
| J3 | 34 ピンボックスヘッダ（Sullins SBH11-NBPC-D17-ST-BK） | 本体外形 50.68 × 9.11 mm。基板の左右に 1.33mm ずつの余裕 |
| Q1・Q2 | NMOS 2N7000（TO-92） | Q1：CAPS 用、Q2：かな用 |
| R1・R2 | 100Ω（PCF14JT100R / LCSC C2587286） | ゲート直列抵抗 |
| R3・R4 | 470Ω（PCF14JT470R / LCSC C2585244） | LED 電流制限抵抗（必須） |
| R5・R6 | 10kΩ（PCF14JT10K0 / LCSC C2587295） | LED プルアップ（+5V へ） |

抵抗はすべて PCF14JT シリーズ（1/4W 炭素皮膜、本体 φ2.3 × 6.0mm、ピッチ 10.00mm、穴径 0.6mm）。最大消費電力は R3・R4 の約 19mW で、定格に対して十分な余裕がある。

### 6.2 部品配置（上から下）

1. J1・J2：Pico ヘッダ（y = 0 〜 -1900 mil）
2. CAPS 段：R1・Q1・R3・R5（y = -2400 mil）
3. かな段：R2・Q2・R4・R6（y = -2800 mil）
4. J3：34 ピンヘッダ（y = -3200 / -3300 mil）

各段は左から「ゲート直列抵抗 → NMOS → 電流制限抵抗 → プルアップ抵抗」の順に並ぶ。SCAN／SENSE 線は抵抗列の間にできる幅約 300 mil の通路を通す。

### 6.3 シルク（表面）

基板右端に 90° 回転（下から上に読む向き）で縦に配置する。

| 文字列 | 内容 | サイズ |
| --- | --- | --- |
| `FUSION ELEMENTS` | ブランド名 | 40 mil |
| `FM-7 Keyboard to USB Converter Type RPPico` | 製品名 | 45 mil |
| `FM7USBKB-001` | 製品番号 | 36 mil |
| `Rev 0.5   2026-09-23` | 版数と日付 | 36 mil |

### 6.4 Rev 0.5 で未搭載の部品

- J3 の 5V 電源用デカップリングコンデンサ（100nF ＋ 10µF）
- SENSE0–6・BREAK の外付けプルアップ抵抗（Pico の内部プルアップを使用）
- INSERT LED の駆動回路

### 6.5 検証結果

- EasyEDA の DRC：エラー・警告とも 0 件
- 全 34 ネットの連結を確認。未接続は pin24（SENSE7）と pin29（INSERT LED）のみで、いずれも設計どおり

### 6.6 設計データ

EasyEDA Pro プロジェクト `FM7 USB Keyboard`。製造用ガーバーは [fab/](../fab/) にある。

| 階層／フィールド | 値 | 対応するシルク |
| --- | --- | --- |
| Project | `FM-7 Keyboard to USB Converter Type RPPico` | 製品名 |
| Board | `FM7USBKB-001` | 製品番号 |
| Schematic | `Main` | — |
| Company | `Fusion Elements` | FUSION ELEMENTS |
| Version | `0.5` | Rev 0.5 |

## 7. 未確定事項

- **キーごとのダイオードの有無：** 診断ファームのゴーストテスト（[§5.5](#55-診断ファームウェア)）で実機確認する。
- **CAP キーの位置：** SCAN 15／SENSE 2 は図面に基づく値で、実機では確認できていない（[キーマトリクス対応表](./fm7-keymatrix.md)参照）。
