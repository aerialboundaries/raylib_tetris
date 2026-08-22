/* TODO

 * 1. web version (DONE)
 * 2. title screen
 * 3. game level 1-15 with delta time
 * 3. Right / Left Rotation
 * 4. SRS / TSPIN
 * 5. sound change
 * 6. sprite for tetrimino
 */

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

#include "colors.h"
#include "config.h"

#include "game.h"

// --- グローバル変数 ---
// Emscriptenのコールバック関数から参照するため、主要な状態を保持します
static Font font;
static Game game;
static double lastUpdateTime = 0;

static bool EventTriggered(double interval)
{
  double currentTime = GetTime();
  if (currentTime - lastUpdateTime >= interval) {
    lastUpdateTime = currentTime;
    return true;
  }
  return false;
}

// 1フレーム分の処理を行う関数（Emscriptenから毎フレーム呼び出されます）
static void UpdateDrawFrame(void)
{
  // 1. ゲーム状態の更新
  game_update_music(game);
  game_handle_input(game);

  if (EventTriggered(0.2) && !game_is_lock_delay_active(game)) {
    game_move_block_down(game);
  }

  game_update(game);

  // 2. 描画処理
  BeginDrawing();
  ClearBackground(colors[darkBlue]);
  DrawTextEx(font, "Score", (Vector2){365, 15}, 38, 2, WHITE);
  DrawTextEx(font, "Next", (Vector2){370, 175}, 38, 2, WHITE);

  if (game_is_over(game)) {
    DrawTextEx(font, "GAME OVER", (Vector2){SIDEBAR_X, 450}, 38, 2, WHITE);
  }

  DrawRectangleRounded((Rectangle){SIDEBAR_X, 55, 170, 60}, 0.3, 6,
                       colors[lightBlue]);

  char scoreText[10];
  sprintf(scoreText, "%d", game_get_score(game));
  Vector2 textSize = MeasureTextEx(font, scoreText, 38, 2);

  DrawTextEx(font, scoreText,

             (Vector2){SIDEBAR_X + (170 - textSize.x) / 2, 65}, 38, 2, WHITE);
  DrawRectangleRounded((Rectangle){SIDEBAR_X, 215, 170, 180}, 0.3, 6,
                       colors[lightBlue]);

  game_draw(game);

  EndDrawing();
}

int main(void)
{

  srand((unsigned int)time(NULL));

  InitWindow(500, 620, "raylib Tetris");

#if !defined(__EMSCRIPTEN__)
  /* モニター設定はデスクトップ環境のみ適用します */
  if (TARGET_MONITOR < GetMonitorCount()) {

    SetWindowMonitor(TARGET_MONITOR);
  }

#endif

  SetTargetFPS(60);

  font = LoadFontEx("Font/monogram.ttf", 64, 0, 0);
  game = create_game();

#if defined(__EMSCRIPTEN__)

  // Web環境：ブラウザにメインループの管理を委ねます
  // 第2引数: FPS (0でブラウザのrequestAnimationFrameに同期)
  // 第3引数: Simulate infinite loop (1を指定して非同期実行)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);

#else
  // デスクトップ環境：従来のwhileループを実行します
  while (!WindowShouldClose()) {
    UpdateDrawFrame();
  }

  // デスクトップ版のクリーンアップ処理
  UnloadFont(font);

  destroy_game(game);
  CloseWindow();
#endif

  return 0;
}
