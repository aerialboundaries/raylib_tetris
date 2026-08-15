#pragma once
#include <raylib.h>

#define NUMROWS 20
#define NUMCOLS 10

typedef struct grid_type *Grid;

Grid create_grid(void);
void destroy_grid(Grid g);
void initialize_grid(Grid g);
void printgrid(Grid g);
void grid_draw(Grid g);
void set_cell_value(Grid g, int row, int col, int value);
int get_cell_value(Grid g, int row, int col);
bool is_row_full(Grid g, int row);
void clear_row(Grid g, int row);
void move_row_down(Grid g, int row, int num_rows);
int clear_full_rows(Grid g);
