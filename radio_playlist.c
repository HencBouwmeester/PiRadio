#include "radio_playlist.h"

#include <stdlib.h>
#include <string.h>

static int read_line(FILE *source, char *line, size_t capacity)
{
	size_t length;
	char *newline;
	int character;

	if (!fgets(line, capacity, source))
		return ferror(source) ? -1 : 0;

	newline = strchr(line, '\n');
	if (newline) {
		*newline = '\0';
	} else {
		character = fgetc(source);
		if (character != EOF && character != '\n') {
			while (character != '\n' && character != EOF)
				character = fgetc(source);
			return -1;
		}
		if (ferror(source))
			return -1;
	}

	length = strlen(line);
	if (length > 0 && line[length - 1] == '\r')
		line[length - 1] = '\0';
	return 1;
}

static int contains_control_characters(const char *value)
{
	const unsigned char *character = (const unsigned char *)value;

	for (; *character; character++) {
		if (*character < 0x20 || *character == 0x7f)
			return 1;
	}
	return 0;
}

static int valid_filename(const char *filename)
{
	size_t length = strlen(filename);
	const char *character;

	if (length <= 4 || strcmp(filename + length - 4, ".m3u") != 0 ||
	    contains_control_characters(filename))
		return 0;

	for (character = filename; *character; character++) {
		if (*character == '/' || *character == '\\')
			return 0;
	}
	return 1;
}

static int valid_url(const char *url)
{
	return url[0] != '\0' && !contains_control_characters(url);
}

int radio_playlist_load(FILE *source, RadioPlaylistEntry **entries, size_t *count)
{
	RadioPlaylistEntry *loaded_entries;
	char filename[RADIO_PLAYLIST_FILENAME_CAPACITY];
	char url[RADIO_PLAYLIST_URL_CAPACITY];
	size_t loaded_count = 0;
	size_t name_length;
	int line_status;
	size_t i;

	if (!entries || !count)
		return -1;
	*entries = NULL;
	*count = 0;
	if (!source)
		return -1;

	loaded_entries = calloc(RADIO_PLAYLIST_LIMIT, sizeof(*loaded_entries));
	if (!loaded_entries)
		return -1;

	while ((line_status = read_line(source, filename, sizeof(filename))) > 0) {
		if (loaded_count == RADIO_PLAYLIST_LIMIT || !valid_filename(filename))
			goto invalid_input;

		line_status = read_line(source, url, sizeof(url));
		if (line_status != 1 || !valid_url(url))
			goto invalid_input;

		for (i = 0; i < loaded_count; i++) {
			if (strcmp(loaded_entries[i].filename, filename) == 0)
				goto invalid_input;
		}

		name_length = strlen(filename) - 4;
		memcpy(loaded_entries[loaded_count].filename, filename, strlen(filename) + 1);
		memcpy(loaded_entries[loaded_count].name, filename, name_length);
		loaded_entries[loaded_count].name[name_length] = '\0';
		memcpy(loaded_entries[loaded_count].url, url, strlen(url) + 1);
		loaded_count++;
	}

	if (line_status < 0)
		goto invalid_input;

	*entries = loaded_entries;
	*count = loaded_count;
	return 0;

invalid_input:
	free(loaded_entries);
	return -1;
}

void radio_playlist_free(RadioPlaylistEntry *entries)
{
	free(entries);
}