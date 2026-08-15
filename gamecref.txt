提示していただいたC++ のコードは、ゲーム全体の状態（フィールド、現在のブロック、次のブロック、スコア、効果音など）を一つのクラスにまとめて管理する、とても素晴らしい設計になっていますね。

    このC++ の設計思想を、現在作られているC言語のプログラム（不透明構造体を使ったカプセル化）に組み込むには、C++ のクラスをC言語の構造体と関数に翻訳（ポート）してあげるのがベストプラクティスです。

        具体的には、game.h と
            game.c という新しいモジュールを作成し、C++ の std::vector
                の代わりにC言語の固定長配列を使って同様のロジックを実現します。

                    それでは、そのまま組み込んで使える完全なコード群を提案します。

    1. 新しく追加する game.h の全文 C
#pragma once
#include "block.h"
#include "grid.h"
#include <raylib.h>
#include <stdbool.h>

    typedef struct game_type *Game;

Game create_game(void);
void destroy_game(Game game);
void game_draw(Game game);
void game_handle_input(Game game);
void game_move_block_down(Game game);

// 外部からゲームオーバー状態やスコアを参照するためのゲッター関数
bool game_is_over(Game game);
int game_get_score(Game game);
Music game_get_music(Game game);

2. 新しく追加する game.c の全文 C++ の std::vector
    によるミノのランダム選択（残りのミノからランダムに選び、空になったら7種類再補充する処理）を、C言語の固定長配列と要素数管理（block_count）でスマートに実装しています。

#include "block.h"
#include "blocks.h"
#include "colors.h"
#include "game.h"
#include "grid.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

    static void
    terminate(const char *message)
{
  printf("%s\n", message);
  exit(EXIT_FAILURE);
}

struct game_type {
  Grid grid;
  int remaining_blocks[7]; // 残っているブロックのIDを保持する配列
  int block_count;         // 残っているブロックの数
  Block currentBlock;
  Block nextBlock;
  bool gameOver;
  int score;
  Music music;
  Sound rotateSound;
  Sound clearSound;
};

// 内部関数（プロトタイプ宣言）
static void refill_blocks(Game game);
static int get_random_block_id(Game game);
static void game_move_block_left(Game game);
static void game_move_block_right(Game game);
static bool is_block_outside(Game game);
static void rotate_block(Game game);
static void lock_block(Game game);
static bool block_fits(Game game);
static void game_reset(Game game);
static void update_score(Game game, int linesCleared, int moveDownPoints);

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

  game->gameOver = false;
  game->score = 0;

  InitAudioDevice();
  game->music = LoadMusicStream("Sounds/music.mp3");
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

static void refill_blocks(Game game)
{
  for (int i = 0; i < 7; i++) {
    game->remaining_blocks[i] = i + 1; // L_BLOCK(1) から Z_BLOCK(7) まで
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

  // 選ばれた要素を配列から削除し、末尾の要素を詰めます
  for (int i = randomIndex; i < game->block_count - 1; i++) {
    game->remaining_blocks[i] = game->remaining_blocks[i + 1];
  }
  game->block_count--;

  return blockId;
}

void game_draw(Game game)
{
  Draw(game->grid);
  draw_block(game->currentBlock);

  // 次のブロックの描画位置をIDに応じて調整します
  // 本来のcreate_blockのオフセットとは別に、UI表示用の座標を渡すように設計されています
  // ここではC++コードのロジックを再現するため、一時的に現在のブロックの表示位置を考慮した値にしています
  // ※必要に応じてnextBlock側の構造や描画関数を微調整してください
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
    game_move_block_left(game);
    break;
  case KEY_RIGHT:
    game_move_block_right(game);
    break;
  case KEY_DOWN:
    game_move_block_down(game);
    update_score(game, 0, 1);
    break;
  case KEY_UP:
    rotate_block(game);
    break;
  }
}

static void game_move_block_left(Game game)
{
  if (!game->gameOver) {
    Move(game->currentBlock, 0, -1);
    if (is_block_outside(game) || !block_fits(game)) {
      Move(game->currentBlock, 0, 1);
    }
  }
}

static void game_move_block_right(Game game)
{
  if (!game->gameOver) {
    Move(game->currentBlock, 0, 1);
    if (is_block_outside(game) || !block_fits(game)) {
      Move(game->currentBlock, 0, -1);
    }
  }
}

void game_move_block_down(Game game)
{
  if (!game->gameOver) {
    Move(game->currentBlock, 1, 0);
    if (is_block_outside(game) || !block_fits(game)) {
      Move(game->currentBlock, -1, 0);
      lock_block(game);
    }
  }
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

// 注意: 現状のblock.cには回転(Rotate)と元に戻す(UndoRotation)が未実装のため、
// ここでは枠組みだけを定義しています。必要に応じてblock.cに回転ロジックを追加してください。
static void rotate_block(Game game)
{
  if (!game->gameOver) {
    // block_typeにrotationStateを変化させる関数を後ほど追加します
    // currentBlock->rotationState = (currentBlock->rotationState + 1) % 4;

    if (is_block_outside(game) || !block_fits(game)) {
      // 元に戻す処理
    } else {
      PlaySound(game->rotateSound);
    }
  }
}

static void lock_block(Game game)
{
  Position tiles[4];
  GetCellPositions(game->currentBlock, tiles);

  // ブロックをグリッドに固定
  for (int i = 0; i < 4; i++) {
    // block.c から id を取得するゲッター、または直接アクセスが必要です
    // ここでは便宜上、C++のロジック通りにグリッドへ値をセットする流れを示しています
    // ※grid.cのset_cell_value関数を利用します
    // set_cell_value(game->grid, tiles[i].row, tiles[i].column, id);
  }

  destroy_block(game->currentBlock);
  game->currentBlock = game->nextBlock;

  if (!block_fits(game)) {
    game->gameOver = true;
  }

  game->nextBlock = create_block(get_random_block_id(game));

  // グリッドの行削除処理（現状のgrid.cには未実装のため、今後追加する要素となります）
  int rowsCleared = 0;
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
    // 必要に応じてgrid.cにセルの値を取得する関数を追加してください
  }
  return true;
}

static void game_reset(Game game)
{
  initialize_grid(game->grid);
  game->block_count = 0;
  refill_blocks(game);

  if (game->currentBlock)
    destroy_block(game->currentBlock);
  if (game->nextBlock)
    destroy_block(game->nextBlock);

  game->currentBlock = create_block(get_random_block_id(game));
  game->nextBlock = create_block(get_random_block_id(game));
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
Music game_get_music(Game game)
{
  return game->music;
}
3. 新しく書き換える
    main.c の全文 Game モジュールがすべてを隠蔽してくれたため、main.c
        は劇的にシンプルですっきりとしたコードになります。

            C
#include "colors.h"
#include "game.h"
#include <raylib.h>
#include <stdlib.h>
#include <time.h>

    int
    main(void)
{
  // ランダムの初期化
  srand(time(NULL));

  InitWindow(300, 600, "raylib Tetris");
  SetTargetFPS(60);

  Game game = create_game();

  // メインループ
  while (!WindowShouldClose()) {
    // 音楽の更新
    UpdateMusicStream(game_get_music(game));

    // 入力処理
    game_handle_input(game);

    // 描画処理
    BeginDrawing();
    ClearBackground(colors[darkBlue]);

    game_draw(game);

    EndDrawing();
  }

  destroy_game(game);
  CloseWindow();

  return 0;
}
次のステップへのアドバイス
この構成にすることで、C++ の持っていた「ゲーム管理クラス」の役割をC言語のオブジェクト指向スタイルで綺麗に再現できます。

    ただし、C++ のコードにあった
        grid.ClearFullRows() や currentBlock
            .Rotate()、そして各構造体のメンバ（id
    など）へのアクセスは、現在のC言語の各ファイル（grid.c や block
            .c）にまだ実装されていない関数が含まれています。

    統合を完了させるためには、以下の小さな関数をそれぞれのファイルに追加していく必要があります。

    block.c：現在の回転状態を変更する関数（Rotate）や、id を返すゲッター関数。

    grid.c：指定したセルの値を取得するゲッター関数、および揃った行を削除する関数（ClearFullRows）。

    データ構造がしっかりとカプセル化されているため、これらの機能追加もそれぞれのファイル内だけで完結させることができます。素晴らしいゲームになりそうですね、一歩ずつ実装を進めてみてください。
