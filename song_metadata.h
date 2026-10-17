#ifndef SONG_METADATA_H
#define SONG_METADATA_H

#include <stddef.h>
#include <stdio.h>

#define SONG_METADATA_TEXT_CAPACITY 200
#define SONG_METADATA_INPUT_LINE_CAPACITY 512

typedef struct {
	char Artist[SONG_METADATA_TEXT_CAPACITY];
	char Title[SONG_METADATA_TEXT_CAPACITY];
	char Album[SONG_METADATA_TEXT_CAPACITY];
	char Station[SONG_METADATA_TEXT_CAPACITY];
	char CoverArt[SONG_METADATA_TEXT_CAPACITY];
	int Rating;
	int StationNum;
	int Artist_strlen;
	int Title_strlen;
	int Album_strlen;
	int Station_strlen;
} Song;

void song_metadata_clear(Song *song);
size_t song_metadata_copy(char *destination, size_t capacity, const char *source);
void song_metadata_prepare_display(Song *song);
int song_metadata_read_pandora(FILE *source, Song *song);

#endif