#include "radio_services.h"

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

int radio_pandora_count_stations(FILE *station_list)
{
	char line[512];
	int count = 0;

	if (!station_list) {
		errno = EINVAL;
		return -1;
	}

	while (fgets(line, sizeof(line), station_list) != NULL) {
		if (count == INT_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		count++;
	}

	return ferror(station_list) ? -1 : count;
}

int radio_service_action(RadioManagedService service, RadioServiceAction action)
{
	const char *service_name;
	const char *action_name;
	pid_t child;
	int status;

	switch (service) {
	case RADIO_MANAGED_PANDORA:
		service_name = "pianobar.service";
		break;
	case RADIO_MANAGED_AIRPLAY:
		service_name = "shairport.service";
		break;
	case RADIO_MANAGED_MPD:
		service_name = "mpd.service";
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	switch (action) {
	case RADIO_SERVICE_START:
		action_name = "start";
		break;
	case RADIO_SERVICE_STOP:
		action_name = "stop";
		break;
	case RADIO_SERVICE_RESTART:
		action_name = "restart";
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	child = fork();
	if (child < 0)
		return -1;
	if (child == 0) {
		execlp("systemctl", "systemctl", action_name, service_name, (char *)NULL);
		_exit(127);
	}

	while (waitpid(child, &status, 0) < 0) {
		if (errno != EINTR)
			return -1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		errno = EIO;
		return -1;
	}
	return 0;
}

static int pianobar_process_running(void)
{
	DIR *proc = opendir("/proc");
	struct dirent *entry;
	char path[64];
	char command[512];
	char *end;
	char *name;
	FILE *file;
	size_t length;
	long pid;
	int found = 0;

	if (!proc)
		return -1;

	while ((entry = readdir(proc)) != NULL) {
		pid = strtol(entry->d_name, &end, 10);
		if (end == entry->d_name || *end != '\0' || pid <= 0)
			continue;

		if (snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid) >= (int)sizeof(path))
			continue;
		file = fopen(path, "r");
		if (!file)
			continue;
		length = fread(command, 1, sizeof(command) - 1, file);
		fclose(file);
		if (length == 0)
			continue;
		command[length] = '\0';
		name = strrchr(command, '/');
		name = name ? name + 1 : command;
		if (strcmp(name, "pianobar") == 0) {
			found = 1;
			break;
		}
	}

	closedir(proc);
	return found;
}

int radio_pandora_stop(const char *control_path)
{
	int running = pianobar_process_running();
	int result = 0;

	if (running < 0)
		return -1;
	if (running && radio_pandora_send_action(control_path, RADIO_PANDORA_QUIT) != 0)
		result = -1;
	if (radio_service_action(RADIO_MANAGED_PANDORA, RADIO_SERVICE_STOP) != 0)
		result = -1;
	return result;
}