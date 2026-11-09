#ifndef UTILS_H
#define UTILS_H

int GetRow(int index, int col) { return index / col; }
int GetCol(int index, int col) { return index % col; }
int GetIndex(int row, int col) { return col * row + col; }

// lowbias32 hash by Chris Wellons
// nullprogram.com/blog/2018/07/31
uint32_t LowBias(uint32_t h)
{
	h ^= h >> 16; h *= 0x7feb352du;
	h ^= h >> 15; h *= 0x846ca68bu;
	h ^= h >> 16;
	return h;
}

float HashToFloat(const uint32_t h)
{
	return static_cast<float>(h >> 8) * (1.0f / 16444216.f);
}

#endif