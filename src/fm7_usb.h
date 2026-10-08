#pragma once

// FM-7 変換器の USB HID 層。
//
// 2 種類のレポート形式を持ち、コンパイル時に FM7_NKRO で選択する:
//
//   FM7_NKRO=0（デフォルト） 8 バイトのブートプロトコルレポート: モディファイア、
//                            予約、キーコード 6 個。docs/fm7emulator.c が読む形式
//                            （hid_keyboard_report_t）であり、PC の BIOS にも
//                            必要なので、安全なデフォルトとしている。
//   FM7_NKRO=1               21 バイトのレポート: モディファイア 1 バイト +
//                            usage 0x00-0x9F をカバーする 160 ビットのビットマップ
//                            （カナ 0x88 と円 0x89 を含む）。
//
// 列挙されるのは常にどちらか一方だけ。両方を同時に公開すると、通常の OS は
// キーボードが 2 台あると認識し、すべての打鍵を二重に登録してしまう。
//
// FM-7 のマトリクスにはキーごとのダイオードがないので、このハードウェアでは
// NKRO にしても真の N キーロールオーバーは実現できない -- 3 キー以上の同時押し
// では、どちらの形式でもスキャナのゴーストブロックに頼るしかない。FM7_NKRO は
// 6 キーの上限を嫌うホスト向けのもので、ロールオーバーの解決策ではない。

#include <Arduino.h>

#ifndef FM7_NKRO
#define FM7_NKRO 0
#endif

namespace fm7usb {

#if FM7_NKRO
// モディファイア 1 バイト + ビットマップ 20 バイト。
constexpr uint8_t kKeyBitmapBytes = 20;
constexpr uint8_t kMaxUsage = 0x9F;
#endif

// HID インターフェースを立ち上げる。レポートを送る前に setup() から呼ぶこと。
void begin();

// TinyUSB がバスを処理できるよう、loop() から繰り返し呼ぶ必要がある。
void task();

bool mounted();

// 作業用レポートをクリアする。
void clearKeys();

// 作業用レポートに HID usage / モディファイアビットを 1 つ追加する。
// addKey はレポートが既に満杯なら false を返す（6KRO のみ）。
bool addKey(uint8_t usage);
void addModifier(uint8_t modifierBits);

// 作業用レポートのキー部分を、モディファイアバイトはそのままに、全スロット
// ErrorRollOver（0x01）に置き換える。レポートに載せきれない数のキーが
// 押されているとき、ブートプロトコルでは切り詰めたキー一覧ではなく
// これが求められる。ビットマップがあふれない NKRO ビルドでは何もしない。
void setRollOver();

// 作業用レポートが前回送ったものと異なれば送信する。
// 実際に送信したときに true を返す。
bool sendIfChanged();

// 直近の Output Report によるホストの LED 状態。HID の LED ビットそのまま
// （bit0 NumLock、bit1 CapsLock、bit2 ScrollLock）。
uint8_t ledState();

// Output Report のコールバックで立てられ、メインループが一度だけ反応できる。
bool ledStateChanged();

}  // namespace fm7usb
