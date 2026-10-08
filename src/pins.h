#pragma once

// FM-7 キーボード 34 ピンコネクタ <-> Raspberry Pi Pico GPIO の対応表（仕様 v0.4: CAPS/KANA LED は NMOS 経由、SENSE7 なし）。
// コネクタ各ピンの役割は docs/FM-7 Keyboard USB Conversion with Raspberry Pi Pico.md による。
// コネクタ 24 番ピン（SENSE7）は未配線 -- fm7-keymatrix.md でもキーが接続されていないことを確認済み。
// コネクタ 29 番ピン（INSERT LED）は未配線 -- この設計では CAPS/KANA のみを駆動する。

#include <Arduino.h>

// SCAN0..15 はコネクタ 1-16 番ピンを駆動する（一度に 1 本ずつ、残りは high。アクティブ low）。
constexpr uint8_t kScanPins[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

// SENSE0..6 はコネクタ 17-23 番ピンを読む（内部プルアップ、アクティブ low）。24 番ピン（SENSE7）は未配線。
constexpr uint8_t kSensePins[7] = {
    16, 17, 18, 19, 20, 21, 22,
};

// BREAK キー、コネクタ 25 番ピン -- スキャンマトリクスとは独立（内部プルアップ、アクティブ low）。
constexpr uint8_t kBreakPin = 27;

// オンボード LED。診断ファームウェアではステータス表示に使う。変換器
// ファームウェアでは代わりに INS ランプをここに表示する。コネクタ 29 番ピン
// （INSERT LED）が未配線で、それを駆動できるヘッダの GPIO も残っていないため。
constexpr uint8_t kStatusLedPin = 25;
constexpr uint8_t kLedInsPin = kStatusLedPin;  // アクティブ high（GPIO high = LED 点灯）

// CAPS LOCK LED。NMOS シンク（Q1）+ 470Ω 制限抵抗を介してコネクタ 31 番ピンを駆動 -- アクティブ high（GPIO high = LED 点灯）。
constexpr uint8_t kLedCapsPin = 26;

// カナ LED。NMOS シンク（Q2）+ 470Ω 制限抵抗を介してコネクタ 30 番ピンを駆動 -- アクティブ high（GPIO high = LED 点灯）。
constexpr uint8_t kLedKanaPin = 28;
