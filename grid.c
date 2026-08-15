#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "config.h"
#include "error.h"
#include "grid.h"

struct grid_type {
  int grid[NUMROWS][NUMCOLS];
  int rows;
  int cols;
  int cellSize;
};

Grid create_grid(void)
{
  Grid g = malloc(sizeof(struct grid_type));
  if (g == NULL)
    terminate("Error in create: grid could not be created.");
  g->rows = NUMROWS;
  g->cols = NUMCOLS;
  g->cellSize = CELL_SIZE; // cell size for 300 x 600

  return g;
}

void destroy_grid(Grid g)
{
  free(g);
}

void initialize_grid(Grid g)
{
  for (int row = 0; row < NUMROWS; row++) {
    for (int column = 0; column < NUMCOLS; column++) {
      g->grid[row][column] = 0;
    }
  }
}

void grid_draw(Grid g)
{
  for (int row = 0; row < NUMROWS; row++) {
    for (int column = 0; column < NUMCOLS; column++) {
      int cellValue = g->grid[row][column];
      DrawRectangle(column * g->cellSize + 11, row * g->cellSize + 11,
                    g->cellSize - 1, g->cellSize - 1, colors[cellValue]);
    }
  }
}

void set_cell_value(Grid g, int row, int col, int value)
{
  g->grid[row][col] = value;
}

int get_cell_value(Grid g, int row, int col)
{
  return g->grid[row][col];
}

bool is_row_full(Grid g, int row)
{
  for (int column = 0; column < NUMCOLS; column++) {
    if (g->grid[row][column] == 0) {
      return false;
    }
  }
  return true;
}

void clear_row(Grid g, int row)
{
  for (int column = 0; column < NUMCOLS; column++) {
    g->grid[row][column] = 0;
  }
}

void move_row_down(Grid g, int row, int num_rows)
{
  for (int column = 0; column < NUMCOLS; column++) {
    g->grid[row + num_rows][column] = g->grid[row][column];
    g->grid[row][column] = 0;
  }
}

int clear_full_rows(Grid g)
{
  int completed = 0;
  for (int row = NUMROWS - 1; row >= 0; row--) {
    if (is_row_full(g, row)) {
      clear_row(g, row);
      completed++;
    } else if (completed > 0) {
      move_row_down(g, row, completed);
    }
  }
  return completed;
}
