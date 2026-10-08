#pragma once

// FM-7 キーボードマトリクス -> USB HID キーコード変換表。
//
// キー位置は docs/fm7-keymatrix.md（SCAN x SENSE の図）による。
// HID Usage ID は docs/fm7keymap.h（fm7emulator の fm7usbcode[] テーブル）に
// 合わせて選んでいるので、fm7emulator 側で追加の変換をしなくても、この
// キーボードで正しく操作できる。
//
// fm7KeyHidCode[scan][sense]: 送信する HID Usage ID。その位置にキーがない
// 場合や、純粋なモディファイアの位置（fm7KeyModifier を参照）では 0。
// fm7KeyModifier[scan][sense]: CTRL/SHIFT/GRAPH の位置での HID レポートの
// モディファイアバイトのビット（それ以外は 0）。

#include <Arduino.h>

constexpr uint8_t kNumScan = 16;
constexpr uint8_t kNumSense = 8;

// 以下で使う、見ただけでは分かりにくい HID Usage ID（Keyboard/Keypad ページ）。
constexpr uint8_t kHidPause = 0x48;  // BREAK（独立したピン、マトリクス外）
constexpr uint8_t kHidIntl2 = 0x88;  // カナ
constexpr uint8_t kHidIntl3 = 0x89;  // 円
constexpr uint8_t kHidLang1 = 0x90;  // かな（Mac JIS）
constexpr uint8_t kHidLang2 = 0x91;  // 英数（Mac JIS）

// HID レポートのモディファイアバイトのビット。
constexpr uint8_t kHidModLeftCtrl = 0x01;
constexpr uint8_t kHidModLeftShift = 0x02;
constexpr uint8_t kHidModLeftAlt = 0x04;  // GRAPH
constexpr uint8_t kHidModLeftGui = 0x08;  // Windows / Mac モードでの KANA

constexpr uint8_t kHidZenkaku = 0x35;  // JIS 配列の半角/全角

// ホストモード。実行中に CTRL+GRAPH+PF1/PF2/PF3 で切り替える。
//
//   Fm7      fm7emulator 用: KANA は International2 を送り、fm7emulator は
//            それでカナをトグルする。
//   Windows  KANA は Windows キー、GRAPH+KANA / GRAPH+SPACE は半角/全角を送る。
//   Mac      KANA は Command、GRAPH（Alt）は Option、GRAPH+KANA /
//            GRAPH+SPACE は LANG1 / LANG2 を送る。
//
// 以下のテーブルを含め、それ以外はすべてのモードで共通。USB ディスクリプタも
// 変わらないので、切り替えに再列挙は必要ない。
enum class Fm7Mode : uint8_t { Fm7 = 0, Windows = 1, Mac = 2 };
constexpr uint8_t kNumModes = 3;

constexpr uint8_t kCtrlScan = 15;
constexpr uint8_t kCtrlSense = 3;
constexpr uint8_t kGraphScan = 15;
constexpr uint8_t kGraphSense = 5;
constexpr uint8_t kKanaScan = 15;
constexpr uint8_t kKanaSense = 6;

// CTRL+GRAPH+<キー> でモードを選ぶ。そのキー自体はホストに報告しない。
struct Fm7ModeSwitch {
  uint8_t scan;
  uint8_t sense;
  Fm7Mode mode;
};

const Fm7ModeSwitch fm7ModeSwitches[] = {
    {1, 0, Fm7Mode::Fm7},      // CTRL+GRAPH+PF1
    {2, 0, Fm7Mode::Windows},  // CTRL+GRAPH+PF2
    {3, 0, Fm7Mode::Mac},      // CTRL+GRAPH+PF3
};

// GRAPH + キーの組み合わせで、モードごとに別の usage を代わりに送るもの。
// fm7emulator は F12 でメニューを開閉するが、FM-7 のキーボードには F12 が
// ないので、GRAPH+EL で F12、GRAPH+DUP で F11 を送る。GRAPH+KANA /
// GRAPH+SPACE は入力方式を切り替える: LANG1 / LANG2（Mac JIS キーボードの
// かな / 英数キー）は日本語 / 英語を直接選び、Windows では半角/全角で
// トグルする。Fm7 モードでは、fm7emulator のために KANA 単独は
// International2 のままにする。組み合わせが有効な間は GRAPH 自体（Alt）を
// レポートから外すので、ホストには Alt+F11/F12 ではなく単なる F11/F12 に見える。
struct Fm7GraphCombo {
  uint8_t scan;
  uint8_t sense;
  uint8_t usage[kNumModes];  // Fm7Mode をインデックスとする
};

const Fm7GraphCombo fm7GraphCombos[] = {
    // scan, sense, {Fm7, Windows, Mac}
    {13, 2, {0x45, 0x45, 0x45}},                    // GRAPH+EL    -> F12
    {14, 2, {0x44, 0x44, 0x44}},                    // GRAPH+DUP   -> F11
    {15, 6, {kHidLang1, kHidZenkaku, kHidLang1}},   // GRAPH+KANA
    {4, 5, {kHidLang2, kHidZenkaku, kHidLang2}},    // GRAPH+SPACE
};

const uint8_t fm7KeyHidCode[kNumScan][kNumSense] = {
    // sense:         0     1     2     3     4     5     6     7
    /* scan  0 */ {0x29, 0x1e, 0x14, 0x04, 0x1d, 0x2b, 0x00, 0x00},  // ESC 1 Q A Z TAB
    /* scan  1 */ {0x3a, 0x1f, 0x1a, 0x16, 0x1b, 0x00, 0x00, 0x00},  // PF1 2 W S X
    /* scan  2 */ {0x3b, 0x20, 0x08, 0x07, 0x06, 0x00, 0x00, 0x00},  // PF2 3 E D C
    /* scan  3 */ {0x3c, 0x21, 0x15, 0x09, 0x19, 0x00, 0x00, 0x00},  // PF3 4 R F V
    /* scan  4 */ {0x3d, 0x22, 0x17, 0x0a, 0x05, 0x2c, 0x00, 0x00},  // PF4 5 T G B SPACE
    /* scan  5 */ {0x3e, 0x23, 0x1c, 0x0b, 0x11, 0x53, 0x56, 0x00},  // PF5 6 Y H N = Num-
    /* scan  6 */ {0x3f, 0x24, 0x18, 0x0d, 0x10, 0x61, 0x57, 0x00},  // PF6 7 U J M Num9 Num+
    /* scan  7 */ {0x40, 0x25, 0x0c, 0x0e, 0x36, 0x60, 0x54, 0x00},  // PF7 8 I K , Num8 Num/
    /* scan  8 */ {0x41, 0x26, 0x12, 0x0f, 0x37, 0x5f, 0x55, 0x00},  // PF8 9 O L . Num7 Num*
    /* scan  9 */ {0x42, 0x27, 0x13, 0x33, 0x38, 0x5c, 0x59, 0x00},  // PF9 0 P ; / Num4 Num1
    /* scan 10 */ {0x43, 0x2d, 0x2f, 0x34, 0x2d, 0x5d, 0x5a, 0x00},  // PF10 - @ : - Num5 Num2
    /* scan 11 */ {0x00, 0x2e, 0x30, 0x31, 0x00, 0x5e, 0x5b, 0x00},  // ^ [ ] Num6 Num3
    /* scan 12 */ {0x00, kHidIntl3, 0x2a, 0x28, 0x00, 0x36, 0x58, 0x00},  // Yen BS ENTER , NumENT
    /* scan 13 */ {0x00, 0x49, 0x4b, 0x4c, 0x52, 0x50, 0x62, 0x00},  // INS EL DEL Up Left Num0
    /* scan 14 */ {0x00, 0x4e, 0x4d, 0x4a, 0x51, 0x4f, 0x63, 0x00},  // CLS DUP HOME Down Right Num.
    /* scan 15 */ {0x00, 0x00, 0x39, 0x00, 0x00, 0x00, kHidIntl2, 0x00},  // CAP [Ctrl] [Shift] [Graph] Kana
};

const uint8_t fm7KeyModifier[kNumScan][kNumSense] = {
    // sense:               0  1  2               3                4              5  6  7
    /* scan  0 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  1 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  2 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  3 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  4 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  5 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  6 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  7 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  8 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan  9 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 10 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 11 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 12 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 13 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 14 */ {0, 0, 0, 0, 0, 0, 0, 0},
    /* scan 15 */ {0, 0, 0, kHidModLeftCtrl, kHidModLeftShift, kHidModLeftAlt, 0, 0},
};
