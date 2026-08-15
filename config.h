#pragma once

#define CELL_SIZE 30
#define GRID_OFFSET_X 11
#define GRID_OFFSET_Y 11
#define SIDEBAR_X 320

// --- ロック猶予（Lock Delay）関連 ---
// ブロックが接地してからロックされるまでの猶予時間（秒）
#define LOCK_DELAY 0.5
// 移動・回転による猶予リセットを許容する最大回数
// （標準的なガイドライン準拠テトリスでは15回。これを超えると
//   それ以上リセットされなくなり、タイマー通りにロックされる）
#define MAX_LOCK_RESETS 15
