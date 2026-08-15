#pragma once
#include "position.h"

typedef struct block_type *Block;

Block create_block(int id);
void destroy_block(Block b);
void draw_block(Block b, int offsetX, int offsetY);
void move_block(Block b, int rows, int columns);
void GetCellPositions(Block b, Position movedTiles[4]);
void rotate_block_state(Block block);
void undo_block_rotation(Block block);
int GetBlockId(Block b);
