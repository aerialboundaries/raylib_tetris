#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "colors.h"
#include "config.h"
#include "game.h"

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

int main(void)
{

  srand((unsigned int)time(NULL));

  InitWindow(500, 620, "raylib Tetris");
  SetTargetFPS(60);

  Font font = LoadFontEx("Font/monogram.ttf", 64, 0, 0);

  Game game = create_game();

  // main loop
  while (!WindowShouldClose()) {
    game_update_music(game);
    game_handle_input(game);

    // 【修正】ロック猶予中は、タイマーによる自動落下をスキップする
    if (EventTriggered(0.2) && !game_is_lock_delay_active(game)) {
      game_move_block_down(game);
    }

    // ロック猶予タイマーの監視・ロック処理を毎フレーム行う
    game_update(game);

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

  destroy_game(game);
  CloseWindow();

  return 0;
}
