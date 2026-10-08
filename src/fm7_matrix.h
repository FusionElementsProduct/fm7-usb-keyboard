#pragma once

// FM-7 キーマトリクスのスキャナ: オープンドレインでのスキャン、ライン単位の
// チャタリング除去、ゴースト検出を行う。
//
// FM-7 のキーボードユニットにはキーごとの逆流防止ダイオードがない（本体側は
// オープンコレクタの 74LS145 2 個でスキャンし、曖昧さは MB88401 の
// ファームウェアで解決している）。そのため SCAN x SENSE のグリッド上で
// 長方形を成す 3 キーを押すと、4 つ目の幻のキーが現れる。スキャナは
// ゴーストフラグを報告し、それが立っている間は呼び出し側がキーイベントを
// 出してはならない。docs/fm7-keymatrix.md を参照。

#include <Arduino.h>

#include "pins.h"

class Fm7Matrix {
 public:
  static constexpr uint8_t kScanCount = sizeof(kScanPins) / sizeof(kScanPins[0]);
  static constexpr uint8_t kSenseCount = sizeof(kSensePins) / sizeof(kSensePins[0]);

  // センスラインがこの回数だけ連続して同じ値を読んだら、変化を確定する
  // （おおよそ kDebounceScans * スキャン周期）。
  static constexpr uint8_t kDebounceScans = 4;

  void begin();

  // マトリクス全体を 1 回スキャンする。チャタリング除去後の状態が変化した
  // とき、つまり呼び出し側が新しいレポートを組み立てるべきときに true を返す。
  bool scan();

  // 1 本のスキャンラインについて、チャタリング除去後のセンスビット。
  // ビット N == SENSE N が押下中。
  uint8_t senseMask(uint8_t scanIndex) const { return stable_[scanIndex]; }

  // BREAK はマトリクス外の専用ピンにあるので、ゴーストは起きない。
  bool breakPressed() const { return breakStable_; }

  // 押下中のキーが曖昧（長方形が存在する）な間は true。読み値を信用できない
  // ので、呼び出し側は最後のレポートを保持すること。
  bool ghosting() const { return ghosting_; }

  // マトリクス内で現在押されているキーの数（BREAK は除く）。
  uint8_t pressedCount() const;

 private:
  uint8_t readSenseMask(uint8_t scanIndex) const;
  void updateGhostFlag();

  uint8_t stable_[kScanCount] = {0};   // 確定済みの状態
  uint8_t candidate_[kScanCount] = {0};  // 直近の生の読み値
  uint8_t settled_[kScanCount] = {0};  // candidate_ が続いている回数
  bool breakStable_ = false;
  bool breakCandidate_ = false;
  uint8_t breakSettled_ = 0;
  bool ghosting_ = false;
};
