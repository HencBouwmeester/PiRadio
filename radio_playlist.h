#ifndef RADIO_PLAYLIST_H
#define RADIO_PLAYLIST_H

#include <stddef.h>
#include <stdio.h>

#define RADIO_PLAYLIST_LIMIT 200
#define RADIO_PLAYLIST_FILENAME_CAPACITY 128
#define RADIO_PLAYLIST_URL_CAPACITY 512

typedef struct {
	char filename[RADIO_PLAYLIST_FILENAME_CAPACITY];
	char name[RADIO_PLAYLIST_FILENAME_CAPACITY];
	char url[RADIO_PLAYLIST_URL_CAPACITY];
} RadioPlaylistEntry;

int radio_playlist_load(FILE *source, RadioPlaylistEntry **entries, size_t *count);
void radio_playlist_free(RadioPlaylistEntry *entries);

#endif