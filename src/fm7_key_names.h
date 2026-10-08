#pragma once

// SCAN x SENSE の各位置に対応する人間が読めるキー名。docs/fm7-keymatrix.md
// の表から書き写したもの。診断ファームウェアでのみ使い、シリアルログを
// そのドキュメントと直接見比べられるようにする。どのシリアル端末でも
// 化けないよう、名前はあえて ASCII にしている（YEN、KANA、UP/DOWN/LEFT/RIGHT）。
//
// 空文字列は、その位置にキーが配線されていないとドキュメントに書かれている
// ことを表す。診断でそこが押されたと報告されたら、ドキュメントが誤っている。

#include <Arduino.h>

// fm7KeyName[scan][sense]
const char* const fm7KeyName[16][7] = {
    /* scan  0 */ {"ESC", "1", "Q", "A", "Z", "TAB", ""},
    /* scan  1 */ {"PF1", "2", "W", "S", "X", "", ""},
    /* scan  2 */ {"PF2", "3", "E", "D", "C", "", ""},
    /* scan  3 */ {"PF3", "4", "R", "F", "V", "", ""},
    /* scan  4 */ {"PF4", "5", "T", "G", "B", "SPACE", ""},
    /* scan  5 */ {"PF5", "6", "Y", "H", "N", "=", "Num-"},
    /* scan  6 */ {"PF6", "7", "U", "J", "M", "Num9", "Num+"},
    /* scan  7 */ {"PF7", "8", "I", "K", ",", "Num8", "Num/"},
    /* scan  8 */ {"PF8", "9", "O", "L", ".", "Num7", "Num*"},
    /* scan  9 */ {"PF9", "0", "P", ";", "/", "Num4", "Num1"},
    /* scan 10 */ {"PF10", "-", "@", ":", "-(2)", "Num5", "Num2"},
    /* scan 11 */ {"", "^", "[", "]", "", "Num6", "Num3"},
    /* scan 12 */ {"", "YEN", "BS", "ENTER", "", "Num,", "NumENTER"},
    /* scan 13 */ {"", "INS", "EL", "DEL", "UP", "LEFT", "Num0"},
    /* scan 14 */ {"", "CLS", "DUP", "HOME", "DOWN", "RIGHT", "Num."},
    /* scan 15 */ {"", "", "CAP", "CTRL", "SHIFT", "GRAPH", "KANA"},
};
