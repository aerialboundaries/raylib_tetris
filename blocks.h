#pragma once

#include "position.h"

/* 7種類のミノのID */

enum { L_BLOCK = 1, J_BLOCK, I_BLOCK, O_BLOCK, S_BLOCK, T_BLOCK, Z_BLOCK };

/* [4][4]の意味：4つの回転状態 × 4つのセルの座標 */
extern const Position block_shapes[8][4][4];
