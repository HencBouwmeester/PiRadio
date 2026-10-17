#include "song_metadata.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static size_t prepare_field(char *field, size_t capacity)
{
	size_t length = strlen(field);
	static const char padding[] = "    ";

	if (length > 21 && capacity - length > sizeof(padding) - 1) {
		memcpy(field + length, padding, sizeof(padding));
		length += sizeof(padding) - 1;
	}
	return length;
}

void song_metadata_clear(Song *song)
{
	if (!song)
		return;

	memset(song, 0, sizeof(*song));
	song->StationNum = 1;
}

size_t song_metadata_copy(char *destination, size_t capacity, const char *source)
{
	size_t length;

	if (!destination || capacity == 0)
		return 0;
	if (!source)
		source = "";

	length = strlen(source);
	if (length >= capacity)
		length = capacity - 1;
	memcpy(destination, source, length);
	destination[length] = '\0';
	return length;
}

void song_metadata_prepare_display(Song *song)
{
	if (!song)
		return;

	song->Artist_strlen = (int)prepare_field(song->Artist, sizeof(song->Artist));
	song->Title_strlen = (int)prepare_field(song->Title, sizeof(song->Title));
	song->Album_strlen = (int)prepare_field(song->Album, sizeof(song->Album));
	song->Station_strlen = (int)prepare_field(song->Station, sizeof(song->Station));
}

static int read_line(FILE *source, char *line, size_t capacity)
{
	char *newline;
	int character;
	size_t length;

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

static int parse_rating(const char *line, int *rating)
{
	char *end;
	long value;

	if (!line[0]) {
		*rating = 0;
		return 0;
	}

	errno = 0;
	value = strtol(line, &end, 10);
	while (isspace((unsigned char)*end))
		end++;
	if (errno == ERANGE || end == line || *end != '\0' ||
	    value < INT_MIN || value > INT_MAX)
		return -1;

	*rating = (int)value;
	return 0;
}

int song_metadata_read_pandora(FILE *source, Song *song)
{
	char line[SONG_METADATA_INPUT_LINE_CAPACITY];
	int line_number;
	int line_status;

	if (!song)
		return -1;
	song_metadata_clear(song);
	if (!source)
		return -1;

	for (line_number = 1; line_number <= 6; line_number++) {
		line_status = read_line(source, line, sizeof(line));
		if (line_status < 0)
			goto invalid_input;
		if (line_status == 0)
			break;

		switch (line_number) {
		case 1:
			song_metadata_copy(song->Artist, sizeof(song->Artist), line);
			break;
		case 2:
			song_metadata_copy(song->Title, sizeof(song->Title), line);
			break;
		case 3:
			song_metadata_copy(song->Station, sizeof(song->Station), line);
			break;
		case 4:
			if (parse_rating(line, &song->Rating) != 0)
				goto invalid_input;
			break;
		case 5:
			song_metadata_copy(song->CoverArt, sizeof(song->CoverArt), line);
			break;
		case 6:
			song_metadata_copy(song->Album, sizeof(song->Album), line);
			break;
		}
	}

	song_metadata_prepare_display(song);
	return 0;

invalid_input:
	song_metadata_clear(song);
	return -1;
}