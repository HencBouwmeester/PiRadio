#include <assert.h>

#include "station_index.h"

int main(void)
{
	assert(station_index_step(0, 1, 3) == 1);
	assert(station_index_step(2, 1, 3) == 0);
	assert(station_index_step(0, -1, 3) == 2);
	assert(station_index_step(0, -1, 1) == 0);
	assert(station_index_step(-1, 1, 3) == 0);
	assert(station_index_step(0, -1, 0) == -1);
	return 0;
}