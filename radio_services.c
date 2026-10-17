#include "radio_services.h"

#include <stdio.h>

static int write_text_file(const char *path, const char *text)
{
	FILE *file;
	int result;

	if (!path || !text)
		return -1;

	file = fopen(path, "w");
	if (!file)
		return -1;

	result = fputs(text, file) == EOF ? -1 : 0;
	if (fclose(file) != 0)
		result = -1;
	return result;
}

int radio_pandora_send_action(const char *control_path, RadioPandoraAction action)
{
	const char *command;

	switch (action) {
	case RADIO_PANDORA_LOVE:
		command = "+\n";
		break;
	case RADIO_PANDORA_BAN:
		command = "-\n";
		break;
	case RADIO_PANDORA_NEXT:
		command = "n\n";
		break;
	case RADIO_PANDORA_TIRED:
		command = "t\n";
		break;
	case RADIO_PANDORA_PAUSE:
		command = "p\n";
		break;
	case RADIO_PANDORA_QUIT:
		command = "q\n";
		break;
	default:
		return -1;
	}

	return write_text_file(control_path, command);
}

int radio_pandora_select_station(const char *control_path, int index, int station_count)
{
	char command[32];
	int length;

	if (index < 0 || station_count <= 0 || index >= station_count)
		return -1;

	length = snprintf(command, sizeof(command), "s%d\n", index);
	if (length < 0 || (size_t)length >= sizeof(command))
		return -1;

	return write_text_file(control_path, command);
}

int radio_pandora_clear_nowplaying(const char *nowplaying_path)
{
	return write_text_file(nowplaying_path, "\n");
}