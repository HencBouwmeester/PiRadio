#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "radio_services.h"

static void read_control(const char *path, char *contents, size_t capacity)
{
	FILE *file = fopen(path, "r");
	assert(file != NULL);
	assert(fgets(contents, capacity, file) != NULL);
	assert(fclose(file) == 0);
}

int main(void)
{
	char path[] = "/tmp/piradio-control-XXXXXX";
	char contents[32];
	int descriptor = mkstemp(path);
	assert(descriptor >= 0);
	assert(close(descriptor) == 0);

	assert(radio_pandora_select_station(path, 15, 16) == 0);
	read_control(path, contents, sizeof(contents));
	assert(strcmp(contents, "s15\n") == 0);
	assert(radio_pandora_select_station(path, -1, 16) != 0);
	assert(radio_pandora_select_station(path, 16, 16) != 0);
	assert(radio_pandora_select_station(path, 0, 0) != 0);
	read_control(path, contents, sizeof(contents));
	assert(strcmp(contents, "s15\n") == 0);

	assert(radio_pandora_send_action(path, RADIO_PANDORA_LOVE) == 0);
	read_control(path, contents, sizeof(contents));
	assert(strcmp(contents, "+\n") == 0);
	assert(radio_pandora_send_action(path, RADIO_PANDORA_QUIT) == 0);
	read_control(path, contents, sizeof(contents));
	assert(strcmp(contents, "q\n") == 0);
	assert(radio_pandora_send_action(path, (RadioPandoraAction)99) != 0);

	assert(radio_pandora_clear_nowplaying(path) == 0);
	read_control(path, contents, sizeof(contents));
	assert(strcmp(contents, "\n") == 0);
	assert(radio_pandora_clear_nowplaying("/no/such/directory/nowplaying") != 0);
	assert(unlink(path) == 0);
	return 0;
}