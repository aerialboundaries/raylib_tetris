#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "block.h"

#include "blocks.h"
#include "config.h"
#include "error.h"
#include "game.h"
#include "grid.h"

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
  game->music.looping =
      true; // ループ再生を明示的に有効化（ベストプラクティス：
            // デフォルト値に依存せず意図を明示する）
  PlayMusicStream(game->music);
  game->rotateSound = LoadSound("Sounds/rotate.mp3");
  game->clearSound = LoadSound("Sounds/clear.mp3");

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

  int keyPressed = GetKeyPressed();
  if (game->gameOver && keyPressed != 0) {
    game->gameOver = false;
    game_reset(game);
  }

  switch (keyPressed) {

  case KEY_LEFT:
  case KEY_A:
    game_move_block_left(game);
    break;
  case KEY_RIGHT:
  case KEY_D:
    game_move_block_right(game);
    break;
  case KEY_UP:
  case KEY_SPACE:
    rotate_block(game);
    break;
  }
  // 【修正】押しっぱなし（長押し）を検出する処理
  // ゲームオーバーでない、かつ下キーが押されている間は毎フレーム実行される
  static int soft_drop_counter = 0;

  if (!game->gameOver && (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))) {
    soft_drop_counter++;
    if (soft_drop_counter >= 3) { // 3フレームに一回だけ落とす
      soft_drop_counter = 0;

      // 実際に1マス下に移動できた場合のみ加点する
      // （接地していて動けなかった場合は加点しない）
      if (game_move_block_down(game)) {

        update_score(game, 0, 1);
      }
    }
  } else {
    soft_drop_counter = 0; // keyを話したらリセット
  }
}

// internal functions (implementations)
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

  // delete selected item from array and shift the last item
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
      // 移動に成功した場合、接地中であればロック猶予をリセット
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
      // 移動に成功した場合、接地中であればロック猶予をリセット
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
    // これ以上下に動けない＝接地。ここでは即ロックしない。
    // ロックするかどうかはgame_update内のロック猶予タイマーに任せる。
    move_block(game->currentBlock, -1, 0);
    return false;
  }

  // 下に移動できた＝空空中いるのでロック猶予を解除
  game->lockDelayActive = false;

  // 今回の落下で新しく到達した深さであれば、リセット回数を0に戻す。
  // これにより「実際に下へ進んでいる限りは粘れる」という
  // Infinity的な挙動になる（横移動や回転だけでは粘り続けられない）。
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
    // grid.h に定義されているNUMROWS, NUMCOLSの範囲外かどうかを判定
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

      // 元に戻す処理
      undo_block_rotation(game->currentBlock);
    } else {
      PlaySound(game->rotateSound);
      // 回転に成功した場合、接地中であればロック猶予をリセット
      reset_lock_delay_if_grounded(game);
    }
  }
}

// 【重要修正】メモリの二重解放やクラッシュ、不要な配列更新を防ぐよう順序を整理
static void lock_block(Game game)
{
  Position tiles[4];
  GetCellPositions(game->currentBlock, tiles);

  // 1. ブロックをグリッドに固定
  for (int i = 0; i < 4; i++) {
    set_cell_value(game->grid, tiles[i].row, tiles[i].column,
                   GetBlockId(game->currentBlock));
  }

  // 2. 使い終わった currentBlock のメモリを解放
  destroy_block(game->currentBlock);

  // 3. 次のブロック (nextBlock) を現在のブロックに引き継ぐ
  game->currentBlock = game->nextBlock;
  game->nextBlock = NULL; // 重複解放を防ぐために一度 NULL に退避

  // 新しいブロックの追跡（ロック猶予の状態リセット）を開始
  start_new_block_tracking(game);

  // 4. 引き継いだ currentBlock がすでに置けない状態ならゲームオーバー
  if (!block_fits(game)) {

    game->gameOver = true;
  }

  // 5. 新しい「次のブロック」を生成

  game->nextBlock = create_block(get_random_block_id(game));

  // 6. ライン消去判定とスコアの更新
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
    // グリッド上の該当セルが空(0)かどうかをチェックする処理
    if (get_cell_value(game->grid, tiles[i].row, tiles[i].column) != 0) {
      return false;
    }
  }
  return true;
}

// 現在のブロックが、盤面の状態を変えずに「あと1マス下に動けるか」を判定する。
static bool can_move_down(Game game)
{
  move_block(game->currentBlock, 1, 0);
  bool fits = !is_block_outside(game) && block_fits(game);

  move_block(game->currentBlock, -1, 0);

  return fits;
}

// 現在のブロックが占めている4マスのうち、最も下（rowの値が最大）の行を返す。
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

// 接地中（ロック猶予タイマー作動中）であれば、タイマーをリセットする。
static void reset_lock_delay_if_grounded(Game game)
{
  if (!game->lockDelayActive)
    return; // 空中にいるので、そもそもリセットする対象がない

  if (game->lockResetCount < MAX_LOCK_RESETS) {
    game->lockDelayStartTime = GetTime();
    game->lockResetCount++;
  }
}

// currentBlockが新しく出現した（スポーンした）ときに呼ぶ。
static void start_new_block_tracking(Game game)
{
  game->lockDelayActive = false;
  game->lockResetCount = 0;
  game->lowestRowReached = get_block_lowest_row(game);
}

// 毎フレーム呼び出す。ロック猶予タイマーの管理と、猶予切れになったブロックの固定を行う。

void game_update(Game game)
{
  if (game->gameOver)
    return;

  if (can_move_down(game)) {
    // まだ下に空間があるので、接地しておらずロック猶予も不要
    game->lockDelayActive = false;
    return;
  }

  // ここに来た時点で「接地している」状態
  if (!game->lockDelayActive) {
    // 接地した瞬間なので、猶予タイマーを開始する
    game->lockDelayActive = true;

    game->lockDelayStartTime = GetTime();
    return;
  }

  // 猶予タイマーが開始済みで、かつ猶予時間を過ぎていたらロックする
  if (GetTime() - game->lockDelayStartTime >= LOCK_DELAY) {
    lock_block(game);
    game->lockDelayActive = false;
  }
}

// 外部（main.c）から現在のロック猶予状態を参照するゲッター関数

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

  // raylibのMP3ストリーミングが途切れたときの安全対策
  if (!IsMusicStreamPlaying(game->music)) {
    PlayMusicStream(game->music);
  }
}

Music game_get_music(Game game)
{
  return game->music;
}
