// FM-7 キーボード -> USB HID 変換器。
//
// このファイルには 2 つのファームウェアが入っており、PlatformIO の環境で選ぶ:
//
//   [env:pico]       変換器（デフォルト）。CTRL+GRAPH+PF1/PF2/PF3 で
//                    Fm7 / Windows / Mac モードを切り替える。fm7_hid_map.h の
//                    Fm7Mode を参照
//   [env:pico_diag]  -DFM7_DIAGNOSTIC。docs/fm7-keymatrix.md を実機で検証する
//                    ための、シリアル経由のマトリクス調査ツール
//
// docs/FM-7 Keyboard USB Conversion with Raspberry Pi Pico.md を参照。

#include <Arduino.h>

#include "fm7_matrix.h"
#include "pins.h"

namespace {
Fm7Matrix matrix;
}  // namespace

// ---------------------------------------------------------------------------
#if defined(FM7_DIAGNOSTIC)
// ---------------------------------------------------------------------------

#include "fm7_hid_map.h"
#include "fm7_key_names.h"

namespace {

uint8_t previousMask[Fm7Matrix::kScanCount] = {0};
bool previousBreak = false;
bool previousGhost = false;

void describe(uint8_t scan, uint8_t sense, const char* edge) {
  const char* name = fm7KeyName[scan][sense];
  const uint8_t usage = fm7KeyHidCode[scan][sense];
  const uint8_t modifier = fm7KeyModifier[scan][sense];

  Serial.printf("%s scan=%-2u sense=%u  ", edge, scan, sense);
  if (name[0] == '\0') {
    Serial.printf("*** UNDOCUMENTED POSITION (fm7-keymatrix.md says empty) ***\n");
    return;
  }
  Serial.printf("%-9s ", name);
  if (modifier != 0) {
    Serial.printf("modifier=0x%02X\n", modifier);
  } else if (usage != 0) {
    Serial.printf("usage=0x%02X\n", usage);
  } else {
    Serial.printf("*** NO HID CODE MAPPED ***\n");
  }
}

void dumpMatrix() {
  Serial.println();
  Serial.println("     SCAN: 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15");
  for (uint8_t s = 0; s < Fm7Matrix::kSenseCount; s++) {
    Serial.printf("  SENSE %u: ", s);
    for (uint8_t i = 0; i < Fm7Matrix::kScanCount; i++) {
      const bool down = matrix.senseMask(i) & (1u << s);
      Serial.printf(" %c ", down ? '#' : '.');
    }
    Serial.println();
  }
  Serial.printf("  keys down: %u   ghosting: %s\n\n", matrix.pressedCount(),
                matrix.ghosting() ? "YES" : "no");
}

void printHelp() {
  Serial.println();
  Serial.println("FM-7 matrix diagnostic");
  Serial.println("  press keys on the FM-7 keyboard; each edge is logged");
  Serial.println("  d  dump the current matrix as a grid");
  Serial.println("  g  ghost test: hold 3 keys forming a rectangle");
  Serial.println("  h  this help");
  Serial.println();
}

}  // namespace

void setup() {
  matrix.begin();
  pinMode(kStatusLedPin, OUTPUT);
  pinMode(kLedCapsPin, OUTPUT);
  pinMode(kLedKanaPin, OUTPUT);
  digitalWrite(kLedCapsPin, LOW);
  digitalWrite(kLedKanaPin, LOW);

  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    // USB CDC の準備を少し待つが、いつまでもブロックはしない
  }
  digitalWrite(kStatusLedPin, HIGH);
  printHelp();
}

void loop() {
  if (matrix.scan()) {
    for (uint8_t i = 0; i < Fm7Matrix::kScanCount; i++) {
      const uint8_t now = matrix.senseMask(i);
      const uint8_t was = previousMask[i];
      if (now == was) continue;
      for (uint8_t s = 0; s < Fm7Matrix::kSenseCount; s++) {
        const bool isDown = now & (1u << s);
        const bool wasDown = was & (1u << s);
        if (isDown && !wasDown) describe(i, s, "DOWN");
        if (!isDown && wasDown) describe(i, s, "UP  ");
      }
      previousMask[i] = now;
    }

    if (matrix.breakPressed() != previousBreak) {
      previousBreak = matrix.breakPressed();
      Serial.printf("%s BREAK (dedicated pin, usage=0x%02X)\n",
                    previousBreak ? "DOWN" : "UP  ", kHidPause);
    }

    if (matrix.ghosting() != previousGhost) {
      previousGhost = matrix.ghosting();
      Serial.printf(">>> GHOSTING %s -- reading is ambiguous, the converter "
                    "holds its last report here\n",
                    previousGhost ? "DETECTED" : "cleared");
    }
  }

  while (Serial.available()) {
    switch (Serial.read()) {
      case 'd': dumpMatrix(); break;
      case 'h': printHelp(); break;
      case 'g':
        Serial.println("Hold three keys that share two scan lines and two "
                       "sense lines (e.g. Q, W and A). If GHOSTING is "
                       "reported, the matrix has no per-key diodes and true "
                       "NKRO is impossible on this keyboard.");
        break;
      default: break;
    }
  }
}

// ---------------------------------------------------------------------------
#else  // 変換器ファームウェア
// ---------------------------------------------------------------------------

#include <EEPROM.h>

#include "fm7_hid_map.h"
#include "fm7_usb.h"

namespace {

// HID の LED ビット。Fm7 モードでは fm7emulator が送るとおりに届く
// （docs/fm7emulator.c の process_kbd_leds）: CAPS -> Caps Lock、KANA -> Num
// Lock、INS -> Scroll Lock。このボードではコネクタ 29 番ピン（INSERT LED）が
// 未配線なので、Scroll Lock は代わりに Pico のオンボード LED を点灯させる。
// Windows / Mac モードでは Num Lock は文字どおり Num Lock なので、KANA ランプは
// 代わりに HID の Kana ビットに従う（これらのホストは通常このビットを立てない）。
constexpr uint8_t kLedNumLock = 0x01;     // -> KANA ランプ（Fm7 モード）
constexpr uint8_t kLedCapsLock = 0x02;    // -> CAPS ランプ
constexpr uint8_t kLedScrollLock = 0x04;  // -> オンボード LED（INS）
constexpr uint8_t kLedKana = 0x10;        // -> KANA ランプ（Windows / Mac）

// モードはエミュレートされた EEPROM（フラッシュの 1 セクタ）の先頭バイトに
// 保存され、電源を切っても保持される。書き込むのはモードが変わったときだけ。
constexpr size_t kEepromSize = 256;
constexpr int kEepromMagicAddr = 0;
constexpr int kEepromModeAddr = 1;
constexpr uint8_t kEepromMagic = 0xF7;

// モードの確認表示: 両ランプが Fm7 なら 1 回、Windows なら 2 回、Mac なら
// 3 回点滅し、その後ホストの LED 状態に戻る。
constexpr uint32_t kBlinkPhaseMs = 150;

Fm7Mode mode = Fm7Mode::Fm7;

// ゴーストのなかった最後のマトリクスの読み値。現在の読み値が曖昧な間は、
// 幻のキーを送る代わりにこれを報告し続ける。そのため、まずい 3 キーの
// 組み合わせを押しても、押していないキーが入力されるのではなく、
// レポートのマトリクス部分が固まるだけで済む。
uint8_t heldMask[Fm7Matrix::kScanCount] = {0};

// モード切り替え時に押されていた CTRL/SHIFT/GRAPH 以外のキー。離されるまで
// レポートから外しておくので、モードを選んだ PF キーはホストに届かない
// （また、切り替えをまたいで押され続けたキーが新しいモードの対応で
// 解釈し直されることもない）。
uint8_t suppressMask[Fm7Matrix::kScanCount] = {0};

uint8_t blinkPhasesLeft = 0;
uint32_t blinkNextMs = 0;

bool held(uint8_t scan, uint8_t sense) {
  return heldMask[scan] & (1u << sense);
}

void applyHostLeds(uint8_t bits) {
  if (blinkPhasesLeft != 0) return;  // 点滅が終わったときに復元される
  const uint8_t kanaBit = (mode == Fm7Mode::Fm7) ? kLedNumLock : kLedKana;
  // アクティブ high: GPIO が NMOS を駆動し、それがキーボードの LED 電流を引き込む。
  digitalWrite(kLedCapsPin, (bits & kLedCapsLock) ? HIGH : LOW);
  digitalWrite(kLedKanaPin, (bits & kanaBit) ? HIGH : LOW);
  digitalWrite(kLedInsPin, (bits & kLedScrollLock) ? HIGH : LOW);
}

void startModeBlink() {
  blinkPhasesLeft = (static_cast<uint8_t>(mode) + 1) * 2;
  blinkNextMs = millis() + kBlinkPhaseMs;
  digitalWrite(kLedCapsPin, HIGH);
  digitalWrite(kLedKanaPin, HIGH);
}

void updateModeBlink() {
  if (blinkPhasesLeft == 0) return;
  if (static_cast<int32_t>(millis() - blinkNextMs) < 0) return;
  blinkPhasesLeft--;
  blinkNextMs += kBlinkPhaseMs;
  if (blinkPhasesLeft == 0) {
    applyHostLeds(fm7usb::ledState());
    return;
  }
  // 残りフェーズが奇数なら消灯、偶数なら点灯。
  const uint8_t level = (blinkPhasesLeft & 1) ? LOW : HIGH;
  digitalWrite(kLedCapsPin, level);
  digitalWrite(kLedKanaPin, level);
}

void loadMode() {
  EEPROM.begin(kEepromSize);
  const uint8_t stored = EEPROM.read(kEepromModeAddr);
  if (EEPROM.read(kEepromMagicAddr) == kEepromMagic && stored < kNumModes) {
    mode = static_cast<Fm7Mode>(stored);
  }
}

void setMode(Fm7Mode newMode) {
  if (newMode != mode) {
    mode = newMode;
    EEPROM.write(kEepromMagicAddr, kEepromMagic);
    EEPROM.write(kEepromModeAddr, static_cast<uint8_t>(mode));
    EEPROM.commit();
  }
  startModeBlink();
}

// CTRL+GRAPH+PF1/PF2/PF3。PF キーを押した瞬間だけ反応するので、
// 組み合わせを押し続けてもスキャンのたびにフラッシュを書き換えることはない。
void checkModeSwitch(const uint8_t* previous) {
  if (!held(kCtrlScan, kCtrlSense) || !held(kGraphScan, kGraphSense)) return;
  for (const Fm7ModeSwitch& sw : fm7ModeSwitches) {
    const uint8_t bit = 1u << sw.sense;
    if ((heldMask[sw.scan] & bit) == 0 || (previous[sw.scan] & bit) != 0) {
      continue;
    }
    setMode(sw.mode);
    for (uint8_t i = 0; i < Fm7Matrix::kScanCount; i++) {
      uint8_t keep = 0;
      for (uint8_t s = 0; s < Fm7Matrix::kSenseCount; s++) {
        if (fm7KeyModifier[i][s] != 0) keep |= 1u << s;
      }
      suppressMask[i] = heldMask[i] & static_cast<uint8_t>(~keep);
    }
    return;
  }
}

// 現在のモードでの (scan, sense) のモディファイアビット。モディファイアでなければ 0。
uint8_t modifierFor(uint8_t scan, uint8_t sense) {
  if (mode != Fm7Mode::Fm7 && scan == kKanaScan && sense == kKanaSense) {
    return kHidModLeftGui;
  }
  return fm7KeyModifier[scan][sense];
}

// (scan, sense) が有効な GRAPH の組み合わせの一部なら代わりの usage を、
// そうでなければ 0 を返す。
uint8_t graphComboUsage(uint8_t scan, uint8_t sense) {
  if (!held(kGraphScan, kGraphSense)) return 0;
  for (const Fm7GraphCombo& combo : fm7GraphCombos) {
    if (combo.scan == scan && combo.sense == sense) {
      return combo.usage[static_cast<uint8_t>(mode)];
    }
  }
  return 0;
}

void buildReport() {
  bool overflow = false;
  bool comboActive = false;
  uint8_t modifiers = 0;

  fm7usb::clearKeys();

  for (uint8_t i = 0; i < Fm7Matrix::kScanCount; i++) {
    const uint8_t mask = heldMask[i] & static_cast<uint8_t>(~suppressMask[i]);
    if (mask == 0) continue;
    for (uint8_t s = 0; s < Fm7Matrix::kSenseCount; s++) {
      if ((mask & (1u << s)) == 0) continue;

      // 組み合わせを先に判定する: Windows / Mac モードでは KANA も
      // モディファイアだが、GRAPH+KANA はそれより優先されなければならない。
      uint8_t usage = graphComboUsage(i, s);
      if (usage != 0) {
        comboActive = true;
      } else {
        const uint8_t modifier = modifierFor(i, s);
        if (modifier != 0) {
          modifiers |= modifier;
          continue;
        }
        usage = fm7KeyHidCode[i][s];
      }
      if (usage != 0 && !fm7usb::addKey(usage)) {
        overflow = true;
      }
    }
  }

  if (comboActive) {
    modifiers &= static_cast<uint8_t>(~kHidModLeftAlt);
  }
  fm7usb::addModifier(modifiers);

  // BREAK はマトリクス外の専用ピンなので曖昧になることはなく、常に現在の
  // 読み値から取る。
  if (matrix.breakPressed() && !fm7usb::addKey(kHidPause)) {
    overflow = true;
  }

  if (overflow) {
    fm7usb::setRollOver();
  }
}

}  // namespace

void setup() {
  matrix.begin();

  pinMode(kLedInsPin, OUTPUT);
  pinMode(kLedCapsPin, OUTPUT);
  pinMode(kLedKanaPin, OUTPUT);
  digitalWrite(kLedInsPin, LOW);

  loadMode();
  startModeBlink();  // 起動時に保存済みのモードを表示する

  fm7usb::begin();
}

void loop() {
  fm7usb::task();

  if (fm7usb::ledStateChanged()) {
    applyHostLeds(fm7usb::ledState());
  }
  updateModeBlink();

  if (matrix.scan()) {
    if (!matrix.ghosting()) {
      uint8_t previous[Fm7Matrix::kScanCount];
      for (uint8_t i = 0; i < Fm7Matrix::kScanCount; i++) {
        previous[i] = heldMask[i];
        heldMask[i] = matrix.senseMask(i);
        suppressMask[i] &= heldMask[i];  // 離されたキーは再び有効になる
      }
      checkModeSwitch(previous);
    }
    // ゴースト発生中、heldMask は最後の曖昧でないマトリクス状態を保持するが、
    // BREAK が効き続けるようにレポートは組み立て直す。
    buildReport();
  }
  fm7usb::sendIfChanged();
}

#endif  // FM7_DIAGNOSTIC
