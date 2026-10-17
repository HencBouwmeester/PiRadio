#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "radio_playlist.h"

static FILE *playlist_input(const char *contents)
{
	FILE *source = tmpfile();
	assert(source != NULL);
	assert(fputs(contents, source) >= 0);
	rewind(source);
	return source;
}

static void assert_invalid(const char *contents)
{
	RadioPlaylistEntry *entries = NULL;
	size_t count = 0;
	FILE *source = playlist_input(contents);

	assert(radio_playlist_load(source, &entries, &count) != 0);
	assert(entries == NULL);
	assert(count == 0);
	fclose(source);
}

int main(void)
{
	RadioPlaylistEntry *entries = NULL;
	size_t count = 0;
	FILE *source = playlist_input("Station One.m3u\r\nhttp://example.com/one\r\n"
				      "Station Two.m3u\nhttp://example.com/two");

	assert(radio_playlist_load(source, &entries, &count) == 0);
	assert(count == 2);
	assert(strcmp(entries[0].filename, "Station One.m3u") == 0);
	assert(strcmp(entries[0].name, "Station One") == 0);
	assert(strcmp(entries[0].url, "http://example.com/one") == 0);
	assert(strcmp(entries[1].name, "Station Two") == 0);
	radio_playlist_free(entries);
	fclose(source);

	source = playlist_input("");
	assert(radio_playlist_load(source, &entries, &count) == 0);
	assert(count == 0);
	radio_playlist_free(entries);
	fclose(source);

	assert_invalid("../outside.m3u\nhttp://example.com/\n");
	assert_invalid("incomplete.m3u\n");
	assert_invalid("duplicate.m3u\nhttp://example.com/one\n"
		       "duplicate.m3u\nhttp://example.com/two\n");
	assert_invalid("bad\tname.m3u\nhttp://example.com/\n");

	{
		char long_filename[RADIO_PLAYLIST_FILENAME_CAPACITY + 16];
		memset(long_filename, 'a', sizeof(long_filename));
		memcpy(long_filename + sizeof(long_filename) - 5, ".m3u\n", 5);
		long_filename[sizeof(long_filename) - 1] = '\0';
		assert_invalid(long_filename);
	}

	{
		FILE *many_entries = tmpfile();
		int i;
		assert(many_entries != NULL);
		for (i = 0; i <= RADIO_PLAYLIST_LIMIT; i++)
			assert(fprintf(many_entries, "station%d.m3u\nhttp://example.com/%d\n", i, i) > 0);
		rewind(many_entries);
		assert(radio_playlist_load(many_entries, &entries, &count) != 0);
		assert(entries == NULL);
		assert(count == 0);
		fclose(many_entries);
	}

	return 0;
}