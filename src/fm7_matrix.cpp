#include "fm7_matrix.h"

namespace {

// 解放したセンスラインが、次のスキャンラインを駆動する前にフラット
// ケーブル越しにプルアップで戻るまでの時間。RP2040 の内部プルアップは
// 弱い（50-80k）ので、意図的に余裕を持たせている。それでも 16 ライン
// 全体のスキャンは 1 ms を大きく下回る。
constexpr uint32_t kSettleMicros = 50;

}  // namespace

void Fm7Matrix::begin() {
  // スキャンラインはオープンドレインで駆動する: 待機時はハイインピーダンス、
  // アクティブ時は low に引き込む。待機時を high 出力にすると、2 つのキーが
  // 同じセンス行を共有したとき、駆動中の別ラインとぶつかってしまう。
  for (uint8_t pin : kScanPins) {
    pinMode(pin, INPUT);  // 解放状態。スキャン中だけ low に駆動する
  }
  for (uint8_t pin : kSensePins) {
    pinMode(pin, INPUT_PULLUP);
  }
  pinMode(kBreakPin, INPUT_PULLUP);
}

uint8_t Fm7Matrix::readSenseMask(uint8_t scanIndex) const {
  const uint8_t pin = kScanPins[scanIndex];

  // 先に方向、次にレベルを設定する: 入力のままのピンに digitalWrite() を
  // 呼ぶと、Arduino コアによっては出力ラッチではなくプルアップが切り替わる。
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(kSettleMicros);

  uint8_t mask = 0;
  for (uint8_t s = 0; s < kSenseCount; s++) {
    if (digitalRead(kSensePins[s]) == LOW) {
      mask |= static_cast<uint8_t>(1u << s);
    }
  }

  pinMode(pin, INPUT);  // 解放
  return mask;
}

void Fm7Matrix::updateGhostFlag() {
  // 2 本のスキャンラインが 2 つ以上のセンス行を共有すると長方形ができ、
  // 長方形があれば 4 隅のうち少なくとも 1 つは幻のキーである。
  ghosting_ = false;
  for (uint8_t i = 0; i < kScanCount; i++) {
    if (stable_[i] == 0) continue;
    for (uint8_t j = i + 1; j < kScanCount; j++) {
      const uint8_t shared = static_cast<uint8_t>(stable_[i] & stable_[j]);
      if (shared != 0 && (shared & (shared - 1)) != 0) {  // 2 ビット以上が立っている
        ghosting_ = true;
        return;
      }
    }
  }
}

uint8_t Fm7Matrix::pressedCount() const {
  uint8_t n = 0;
  for (uint8_t i = 0; i < kScanCount; i++) {
    for (uint8_t bit = stable_[i]; bit != 0; bit &= static_cast<uint8_t>(bit - 1)) {
      n++;
    }
  }
  return n;
}

bool Fm7Matrix::scan() {
  bool changed = false;

  for (uint8_t i = 0; i < kScanCount; i++) {
    const uint8_t raw = readSenseMask(i);

    if (raw != candidate_[i]) {
      candidate_[i] = raw;
      settled_[i] = 0;
    } else if (settled_[i] < kDebounceScans) {
      settled_[i]++;
    }

    if (settled_[i] >= kDebounceScans && stable_[i] != candidate_[i]) {
      stable_[i] = candidate_[i];
      changed = true;
    }
  }

  const bool rawBreak = (digitalRead(kBreakPin) == LOW);
  if (rawBreak != breakCandidate_) {
    breakCandidate_ = rawBreak;
    breakSettled_ = 0;
  } else if (breakSettled_ < kDebounceScans) {
    breakSettled_++;
  }
  if (breakSettled_ >= kDebounceScans && breakStable_ != breakCandidate_) {
    breakStable_ = breakCandidate_;
    changed = true;
  }

  if (changed) {
    updateGhostFlag();
  }
  return changed;
}
