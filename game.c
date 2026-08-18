#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "block.h"
#include "blocks.h"
#include "config.h"
#include "error.h"
#include "game.h"
#include "grid.h"

#define GAMEPAD_ID 0 // 使用するゲームパッドのID（基本は0）

struct game_type {
  Grid grid;

  int remaining_blocks[7]; // array to store remaining block id

  int block_count; // number of remaining blocks
  Block currentBlock;
  Block nextBlock;
  bool gameOver;
  int score;
  Music music;

  Sound rotateSound;

  Sound clearSound;

  // --- ロック猶予（Lock Delay）関連の状態 ---
  bool lockDelayActive; // 現在ロック猶予タイマーが作動中か
  double
      lockDelayStartTime; // タイマーが最後にリセットされた時刻（GetTime()基準）
  int lockResetCount; // 猶予タイマーをリセットした回数

  int lowestRowReached; // このブロックが今回の落下で到達した最も深い行

  /* --- DAS (Auto-Repeat) 関連の状態 --- */
  int das_direction; // 現在押されている方向(-1: 左, 1: 右, 0: なし)
  int das_timer;     // 長押し時間をカウントするタイマー
};

// internal functions (prototypes)
static void refill_blocks(Game game);
static int get_random_block_id(Game game);

static void game_move_block_left(Game game);
static void game_move_block_right(Game game);
static bool is_block_outside(Game game);
static void rotate_block(Game game);
static void lock_block(Game game);

static bool block_fits(Game game);

static void game_reset(Game game);
static void update_score(Game game, int linesCleared, int movoDownPoints);

// ロック猶予（Lock Delay / Infinity）関連の内部関数
static bool can_move_down(Game game);
static int get_block_lowest_row(Game game);
static void reset_lock_delay_if_grounded(Game game);
static void start_new_block_tracking(Game game);

/* For DAS movement */
static void handle_das_movement(Game game);

Game create_game(void)
{
  Game game = malloc(sizeof(struct game_type));
  if (game == NULL)
    terminate("Error: game could not be created.");

  game->grid = create_grid();

  initialize_grid(game->grid);

  game->block_count = 0;
  refill_blocks(game);

  game->currentBlock = create_block(get_random_block_id(game));

  game->nextBlock = create_block(get_random_block_id(game));
  start_new_block_tracking(game);

  game->gameOver = false;
  game->score = 0;
  InitAudioDevice();
  game->music = LoadMusicStream("Sounds/music.mp3");
  game->music.looping = true;
  PlayMusicStream(game->music);
  game->rotateSound = LoadSound("Sounds/rotate.mp3");

  game->clearSound = LoadSound("Sounds/clear.mp3");

  /* DAS関連メンバーのリセット */

  game->das_direction = 0;
  game->das_timer = 0;

  return game;
}

void destroy_game(Game game)
{
  if (game == NULL)
    return;

  destroy_grid(game->grid);
  if (game->currentBlock)
    destroy_block(game->currentBlock);
  if (game->nextBlock)

    destroy_block(game->nextBlock);

  UnloadSound(game->rotateSound);

  UnloadSound(game->clearSound);
  UnloadMusicStream(game->music);
  CloseAudioDevice();

  free(game);
}

void game_draw(Game game)

{
  grid_draw(game->grid);
  draw_block(game->currentBlock, GRID_OFFSET_X, GRID_OFFSET_Y);
  switch (GetBlockId(game->nextBlock)) {
  case I_BLOCK:
    draw_block(game->nextBlock, 255, 290);
    break;

  case O_BLOCK:
    draw_block(game->nextBlock, 255, 280);
    break;
  default:
    draw_block(game->nextBlock, 270, 270);
    break;
  }
}

void game_handle_input(Game game)
{
  // ゲームオーバー中の処理
  if (game->gameOver) {
    // Enter、Space、またはゲームパッドのStart/Aボタンで再開
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_MIDDLE_RIGHT) ||
        IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
      game->gameOver = false;
      game_reset(game);
    }
    return;
  }

  // 回転およびゲームオーバー解除などの単発入力処理
  int keyPressed = GetKeyPressed();
  bool rotatePressed =
      (keyPressed == KEY_UP || keyPressed == KEY_SPACE ||
       keyPressed == KEY_W) ||
      IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) ||
      IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
      IsGamepadButtonPressed(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_FACE_UP);

  if (rotatePressed) {
    rotate_block(game);
  }

  // 左右移動（DAS)の処理
  handle_das_movement(game);

  // ソフトドロップ処理
  static int soft_drop_counter = 0;

  bool softDropPressed =
      IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S) ||
      IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
      (GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_Y) > 0.5f);

  if (!game->gameOver && softDropPressed) {
    soft_drop_counter++;
    if (soft_drop_counter >= 3) { // 3フレームに一回だけ落とす
      soft_drop_counter = 0;

      if (game_move_block_down(game)) {
        update_score(game, 0, 1);
      }
    }
  } else {
    soft_drop_counter = 0; // キー/ボタンを放したらリセット
  }
}

// 左右長押し移動（DAS)を管理する内部関数
static void handle_das_movement(Game game)
{
  if (game->gameOver)
    return;

  bool left_down =
      IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A) ||
      IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
      (GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X) < -0.5f);

  bool right_down =
      IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D) ||
      IsGamepadButtonDown(GAMEPAD_ID, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
      (GetGamepadAxisMovement(GAMEPAD_ID, GAMEPAD_AXIS_LEFT_X) > 0.5f);

  /* 両方押されている、またはどちらも押されていない場合はDASをリセット */
  if (left_down == right_down) {
    game->das_direction = 0;
    game->das_timer = 0;
    return;
  }

  int current_dir = left_down ? -1 : 1;

  /* 新しくキー/ボタンが押された瞬間、または方向が切り替わった時 */

  if (game->das_direction != current_dir) {
    game->das_direction = current_dir;
    game->das_timer = 0;

    // 押された瞬間にまず1マス動かす
    if (current_dir == -1) {
      game_move_block_left(game);
    } else {
      game_move_block_right(game);
    }
    return;
  }

  // 同じ方向が押され続けている場合
  game->das_timer++;

  // 溜め時間(DAS_DELAY)を超えたら高速移動を開始する
  if (game->das_timer >= DAS_DELAY) {
    // DAS_SPEEDフレーム毎に1マス移動させる

    if ((game->das_timer - DAS_DELAY) % DAS_SPEED == 0) {
      if (current_dir == -1) {
        game_move_block_left(game);
      } else {
        game_move_block_right(game);
      }
    }
  }
}

static void refill_blocks(Game game)
{

  for (int i = 0; i < 7; i++) {

    game->remaining_blocks[i] = i + 1; // L_block(1) to Z_block(7)
  }

  game->block_count = 7;
}

static int get_random_block_id(Game game)
{
  if (game->block_count == 0) {
    refill_blocks(game);
  }

  int randomIndex = rand() % game->block_count;
  int blockId = game->remaining_blocks[randomIndex];

  for (int i = randomIndex; i < game->block_count - 1; i++) {
    game->remaining_blocks[i] = game->remaining_blocks[i + 1];
  }
  game->block_count--;

  return blockId;
}

static void game_move_block_left(Game game)

{
  if (!game->gameOver) {
    move_block(game->currentBlock, 0, -1);

    if (is_block_outside(game) || !block_fits(game)) {
      move_block(game->currentBlock, 0, 1);
    } else {
      reset_lock_delay_if_grounded(game);
    }
  }
}

static void game_move_block_right(Game game)
{
  if (!game->gameOver) {
    move_block(game->currentBlock, 0, 1);

    if (is_block_outside(game) || !block_fits(game)) {
      move_block(game->currentBlock, 0, -1);
    } else {
      reset_lock_delay_if_grounded(game);
    }
  }
}

bool game_move_block_down(Game game)
{
  if (game->gameOver)
    return false;

  move_block(game->currentBlock, 1, 0);
  if (is_block_outside(game) || !block_fits(game)) {
    move_block(game->currentBlock, -1, 0);
    return false;
  }

  game->lockDelayActive = false;

  int currentLowestRow = get_block_lowest_row(game);
  if (currentLowestRow > game->lowestRowReached) {
    game->lowestRowReached = currentLowestRow;

    game->lockResetCount = 0;
  }

  return true;
}

static bool is_block_outside(Game game)
{
  Position tiles[4];
  GetCellPositions(game->currentBlock, tiles);
  for (int i = 0; i < 4; i++) {
    if (tiles[i].row < 0 || tiles[i].row >= NUMROWS || tiles[i].column < 0 ||
        tiles[i].column >= NUMCOLS) {
      return true;
    }
  }
  return false;
}

static void rotate_block(Game game)
{
  if (!game->gameOver) {
    rotate_block_state(game->currentBlock);

    if (is_block_outside(game) || !block_fits(game)) {
      undo_block_rotation(game->currentBlock);

    } else {
      PlaySound(game->rotateSound);
      reset_lock_delay_if_grounded(game);
    }
  }
}

static void lock_block(Game game)
{
  Position tiles[4];
  GetCellPositions(game->currentBlock, tiles);

  for (int i = 0; i < 4; i++) {
    set_cell_value(game->grid, tiles[i].row, tiles[i].column,
                   GetBlockId(game->currentBlock));
  }

  destroy_block(game->currentBlock);

  game->currentBlock = game->nextBlock;

  game->nextBlock = NULL;

  start_new_block_tracking(game);

  if (!block_fits(game)) {
    game->gameOver = true;
  }

  game->nextBlock = create_block(get_random_block_id(game));

  int rowsCleared = clear_full_rows(game->grid);
  if (rowsCleared > 0) {
    PlaySound(game->clearSound);
    update_score(game, rowsCleared, 0);
  }
}

static bool block_fits(Game game)
{
  Position tiles[4];
  GetCellPositions(game->currentBlock, tiles);
  for (int i = 0; i < 4; i++) {
    if (get_cell_value(game->grid, tiles[i].row, tiles[i].column) != 0) {
      return false;
    }
  }
  return true;
}

static bool can_move_down(Game game)
{
  move_block(game->currentBlock, 1, 0);
  bool fits = !is_block_outside(game) && block_fits(game);

  move_block(game->currentBlock, -1, 0);

  return fits;
}

static int get_block_lowest_row(Game game)
{

  Position tiles[4];

  GetCellPositions(game->currentBlock, tiles);

  int lowestRow = tiles[0].row;
  for (int i = 1; i < 4; i++) {
    if (tiles[i].row > lowestRow) {
      lowestRow = tiles[i].row;
    }
  }

  return lowestRow;
}

static void reset_lock_delay_if_grounded(Game game)
{

  if (!game->lockDelayActive)
    return;

  if (game->lockResetCount < MAX_LOCK_RESETS) {
    game->lockDelayStartTime = GetTime();
    game->lockResetCount++;
  }
}

static void start_new_block_tracking(Game game)
{
  game->lockDelayActive = false;

  game->lockResetCount = 0;
  game->lowestRowReached = get_block_lowest_row(game);
}

void game_update(Game game)
{
  if (game->gameOver)
    return;

  if (can_move_down(game)) {
    game->lockDelayActive = false;
    return;
  }

  if (!game->lockDelayActive) {
    game->lockDelayActive = true;
    game->lockDelayStartTime = GetTime();

    return;
  }

  if (GetTime() - game->lockDelayStartTime >= LOCK_DELAY) {
    lock_block(game);
    game->lockDelayActive = false;
  }
}

bool game_is_lock_delay_active(Game game)
{
  return game->lockDelayActive;
}

static void game_reset(Game game)
{

  initialize_grid(game->grid);
  game->block_count = 0;
  refill_blocks(game);

  if (game->currentBlock) {
    destroy_block(game->currentBlock);
    game->currentBlock = NULL;
  }

  if (game->nextBlock) {

    destroy_block(game->nextBlock);
    game->nextBlock = NULL;
  }

  game->currentBlock = create_block(get_random_block_id(game));
  game->nextBlock = create_block(get_random_block_id(game));
  start_new_block_tracking(game);
  game->score = 0;

  game->das_direction = 0;
  game->das_timer = 0;
}

static void update_score(Game game, int linesCleared, int moveDownPoints)
{
  switch (linesCleared) {
  case 1:
    game->score += 100;
    break;

  case 2:
    game->score += 300;
    break;
  case 3:
    game->score += 500;
    break;
  case 4:
    game->score += 800;
    break;
  default:

    break;
  }
  game->score += moveDownPoints;
}

bool game_is_over(Game game)
{
  return game->gameOver;
}

int game_get_score(Game game)
{
  return game->score;
}

void game_update_music(Game game)
{
  UpdateMusicStream(game->music);

  if (!IsMusicStreamPlaying(game->music)) {

    PlayMusicStream(game->music);
  }
}

Music game_get_music(Game game)
{
  return game->music;
}
