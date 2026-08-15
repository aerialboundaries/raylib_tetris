#include <raylib.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "blocks.h"
#include "colors.h"
#include "config.h"
#include "error.h"
#include "position.h"

struct block_type {
  int id;
  int cellSize;
  int rotationState;
  Position offset;
};

Block create_block(int id)
{
  Block b = malloc(sizeof(struct block_type));
  if (b == NULL)
    terminate("Error in create: block could not be created.");

  b->id = id;
  b->cellSize = CELL_SIZE;
  b->rotationState = 0;

  // switch文の中で、すべてのケース（defaultを含む）で完全に初期化を行います
  switch (id) {
  case I_BLOCK: // Iブロック
    b->offset.row = -1;
    b->offset.column = 3;
    break;
  case O_BLOCK: // 例: Oブロック
    b->offset.row = 0;
    b->offset.column = 4;
    break;
  default: // その他のブロック（T, L, J, S, Zなど）
    b->offset.row = 0;
    b->offset.column = 3;
    break;
  }

  return b;
}

void destroy_block(Block b)
{
  free(b);
}

void draw_block(Block b, int offsetX, int offsetY)
{
  Position tiles[4];
  GetCellPositions(b, tiles);

  for (int i = 0; i < 4; i++) {
    DrawRectangle(tiles[i].column * b->cellSize + offsetX,
                  tiles[i].row * b->cellSize + offsetY, b->cellSize - 1,
                  b->cellSize - 1, colors[b->id]);
  }
}

void move_block(Block b, int rows, int columns)
{
  b->offset.row += rows;
  b->offset.column += columns;
}

void GetCellPositions(Block b, Position movedTiles[4])
{
  /* blocks.c
   * で定義されている配列から、現在の回転状態の4つのセルの座標を取得し、オフセットを足す
   */
  for (int i = 0; i < 4; i++) {
    Position tiles = block_shapes[b->id][b->rotationState][i];

    /* 基準位置（offset）にセルの相対座標を足して、実際の画面上の位置を計算 */
    movedTiles[i].row = tiles.row + b->offset.row;
    movedTiles[i].column = tiles.column + b->offset.column;
  }
}

void rotate_block_state(Block block)
{
  if (block != NULL) {
    block->rotationState = (block->rotationState + 1) % 4;
  }
}

void undo_block_rotation(Block block)
{
  if (block != NULL) {
    block->rotationState = (block->rotationState + 3) % 4; // reberse spin
  }
}

int GetBlockId(Block b)
{
  return b->id;
}
