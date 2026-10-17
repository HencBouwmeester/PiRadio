#include "station_index.h"

int station_index_step(int index, int direction, int count)
{
	if (count <= 0)
		return -1;

	index %= count;
	if (index < 0)
		index += count;

	index += direction;
	if (index < 0)
		return count - 1;
	if (index >= count)
		return 0;
	return index;
}