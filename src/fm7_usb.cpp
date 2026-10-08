#include "fm7_usb.h"

// 診断用の環境は TinyUSB なし（HID インターフェース自体もなし）でビルドするが、
// PlatformIO は src/ 内のファイルをすべてコンパイルしてしまう。
#if !defined(FM7_DIAGNOSTIC)

#include <Adafruit_TinyUSB.h>

namespace fm7usb {
namespace {

#if FM7_NKRO

// モディファイアバイト、標準の LED 出力 5 ビット、続いて usage 0x00 から
// kMaxUsage まで 1 つにつき 1 ビットの Input。レポート ID は付けない:
// 付けると、今でも一部のホストがフォールバックするブートプロトコルの
// レイアウトが崩れる。
const uint8_t kReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Constant) -- 1 バイトに揃えるパディング
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, kMaxUsage,   //   Usage Maximum (0x9F)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, kMaxUsage + 1,  // Report Count (160)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0xC0,              // End Collection
};

constexpr uint8_t kReportBytes = 1 + kKeyBitmapBytes;
uint8_t working[kReportBytes];
uint8_t lastSent[kReportBytes];

#else  // 6KRO ブートプロトコルレポート

const uint8_t kReportDescriptor[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

constexpr uint8_t kMaxKeys = 6;
constexpr uint8_t kReportBytes = 8;  // モディファイア、予約、キーコード 6 個
uint8_t working[kReportBytes];
uint8_t lastSent[kReportBytes];

#endif

// OUT エンドポイントは持たない: 標準的なブートキーボードと同様に、LED 状態は
// コントロール転送の SET_REPORT で届く。fm7emulator の tuh_hid_set_report()
// が発行するのもまさにこれである。
Adafruit_USBD_HID hid(kReportDescriptor, sizeof(kReportDescriptor),
                      HID_ITF_PROTOCOL_KEYBOARD, 2, false);

volatile uint8_t ledBits = 0;
volatile bool ledDirty = false;
bool everSent = false;

// ホストは LED 状態をレポート 0 の 1 バイトとして送ってくる。
void setReportCallback(uint8_t report_id, hid_report_type_t report_type,
                       uint8_t const* buffer, uint16_t bufsize) {
  (void)report_id;
  if (report_type != HID_REPORT_TYPE_OUTPUT || bufsize < 1) return;
  const uint8_t bits = buffer[0];
  if (bits != ledBits) {
    ledBits = bits;
    ledDirty = true;
  }
}

}  // namespace

void begin() {
  memset(working, 0, sizeof(working));
  memset(lastSent, 0, sizeof(lastSent));

  // デバイスレベルの製造元／製品名文字列は platformio.ini の USB_MANUFACTURER
  // と USB_PRODUCT から来る: setup() が走る時点で Arduino コアは既に
  // TinyUSBDevice.begin() を呼んでいるので、ここで設定しても間に合わない。
  hid.setPollInterval(1);
  hid.setReportCallback(nullptr, setReportCallback);
  hid.setStringDescriptor("FM-7 Keyboard");
  hid.begin();
}

void task() {
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif
}

bool mounted() { return TinyUSBDevice.mounted(); }

void clearKeys() { memset(working, 0, sizeof(working)); }

void addModifier(uint8_t modifierBits) { working[0] |= modifierBits; }

bool addKey(uint8_t usage) {
  if (usage == 0) return true;

#if FM7_NKRO
  if (usage > kMaxUsage) return false;
  working[1 + (usage >> 3)] |= static_cast<uint8_t>(1u << (usage & 0x07));
  return true;
#else
  for (uint8_t i = 0; i < kMaxKeys; i++) {
    if (working[2 + i] == usage) return true;  // 既に入っている
    if (working[2 + i] == 0) {
      working[2 + i] = usage;
      return true;
    }
  }
  return false;  // ロールオーバーのあふれを通知するかは呼び出し側が決める
#endif
}

void setRollOver() {
#if !FM7_NKRO
  for (uint8_t i = 0; i < kMaxKeys; i++) {
    working[2 + i] = 0x01;  // ErrorRollOver
  }
#endif
}

bool sendIfChanged() {
  if (!TinyUSBDevice.mounted()) return false;
  if (everSent && memcmp(working, lastSent, sizeof(working)) == 0) return false;
  if (!hid.ready()) return false;

  if (!hid.sendReport(0, working, sizeof(working))) return false;

  memcpy(lastSent, working, sizeof(working));
  everSent = true;
  return true;
}

uint8_t ledState() { return ledBits; }

bool ledStateChanged() {
  if (!ledDirty) return false;
  ledDirty = false;
  return true;
}

}  // namespace fm7usb

#endif  // !FM7_DIAGNOSTIC
