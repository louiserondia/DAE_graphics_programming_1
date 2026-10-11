#ifndef UTILS_H
#define UTILS_H

int GetRow(int index, int col) { return index / col; }
int GetCol(int index, int col) { return index % col; }
int GetIndex(int row, int col) { return col * row + col; }

#endif