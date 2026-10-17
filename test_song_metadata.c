#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "song_metadata.h"

static FILE *metadata_input(const char *contents)
{
	FILE *source = tmpfile();
	assert(source != NULL);
	assert(fputs(contents, source) >= 0);
	rewind(source);
	return source;
}

int main(void)
{
	Song song;
	char copied[5];
	FILE *source = metadata_input("Artist Name\r\nSong Title\nStation Name\n1\n"
				      "https://example.com/cover\nAlbum Name");

	assert(song_metadata_read_pandora(source, &song) == 0);
	assert(strcmp(song.Artist, "Artist Name") == 0);
	assert(strcmp(song.Title, "Song Title") == 0);
	assert(strcmp(song.Station, "Station Name") == 0);
	assert(song.Rating == 1);
	assert(strcmp(song.CoverArt, "https://example.com/cover") == 0);
	assert(strcmp(song.Album, "Album Name") == 0);
	assert(song.Artist_strlen == (int)strlen(song.Artist));
	fclose(source);

	source = metadata_input("A very long artist name that will scroll\n");
	assert(song_metadata_read_pandora(source, &song) == 0);
	assert(song.Artist_strlen == (int)strlen("A very long artist name that will scroll    "));
	assert(song.Artist[song.Artist_strlen] == '\0');
	assert(song.Title[0] == '\0');
	fclose(source);

	{
		char long_artist[SONG_METADATA_TEXT_CAPACITY + 80];
		memset(long_artist, 'x', sizeof(long_artist) - 1);
		long_artist[sizeof(long_artist) - 1] = '\0';
		source = tmpfile();
		assert(source != NULL);
		assert(fprintf(source, "%s\n", long_artist) > 0);
		rewind(source);
		assert(song_metadata_read_pandora(source, &song) == 0);
		assert(strlen(song.Artist) == sizeof(song.Artist) - 1);
		assert(song.Artist[sizeof(song.Artist) - 1] == '\0');
		assert(song.Artist_strlen == (int)sizeof(song.Artist) - 1);
		fclose(source);
	}

	source = metadata_input("Partial record");
	assert(song_metadata_read_pandora(source, &song) == 0);
	assert(strcmp(song.Artist, "Partial record") == 0);
	assert(song.StationNum == 1);
	fclose(source);

	source = metadata_input("Artist\nTitle\nStation\nnot-a-rating\n");
	assert(song_metadata_read_pandora(source, &song) != 0);
	assert(song.Artist[0] == '\0');
	fclose(source);

	memset(copied, 'z', sizeof(copied));
	assert(song_metadata_copy(copied, sizeof(copied), "too long") == 4);
	assert(strcmp(copied, "too ") == 0);
	assert(song_metadata_copy(copied, sizeof(copied), NULL) == 0);
	assert(copied[0] == '\0');
	return 0;
}