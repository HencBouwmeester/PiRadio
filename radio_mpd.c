#include "radio_services.h"

#include <libmpd-1.0/libmpd/libmpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct RadioMpdService {
	MpdObj *client;
};

static void copy_text(char *destination, size_t capacity, const char *source)
{
	if (!source)
		source = "";
	snprintf(destination, capacity, "%s", source);
}

RadioMpdService *radio_mpd_service_create(const char *hostname, int port, const char *password)
{
	RadioMpdService *service;

	if (port <= 0)
		return NULL;
	if (!hostname || !hostname[0])
		hostname = "localhost";

	service = calloc(1, sizeof(*service));
	if (!service)
		return NULL;

	service->client = mpd_new((char *)hostname, port, (char *)password);
	if (!service->client) {
		free(service);
		return NULL;
	}
	return service;
}

void radio_mpd_service_free(RadioMpdService *service)
{
	if (!service)
		return;
	mpd_free(service->client);
	free(service);
}

int radio_mpd_service_connect(RadioMpdService *service)
{
	if (!service || !service->client)
		return -1;
	return mpd_connect(service->client) == 0 ? 0 : -1;
}

int radio_mpd_service_stop(RadioMpdService *service)
{
	if (radio_mpd_service_connect(service) != 0)
		return -1;
	return mpd_player_stop(service->client) == 0 ? 0 : -1;
}

int radio_mpd_service_change_station(RadioMpdService *service, const char *station)
{
	if (!station || radio_mpd_service_connect(service) != 0)
		return -1;
	if (mpd_playlist_clear(service->client) != 0 ||
	    mpd_playlist_queue_load(service->client, station) != 0 ||
	    mpd_playlist_queue_commit(service->client) != 0 ||
	    mpd_player_play(service->client) != 0)
		return -1;
	return 0;
}

int radio_mpd_service_current_song(RadioMpdService *service, RadioMpdSong *song)
{
	mpd_Song *current;

	if (!song || radio_mpd_service_connect(service) != 0)
		return -1;
	memset(song, 0, sizeof(*song));
	if (mpd_status_update(service->client) != 0)
		return -1;

	current = mpd_playlist_get_current_song(service->client);
	if (!current || !current->title)
		return 0;

	copy_text(song->name, sizeof(song->name), current->name);
	copy_text(song->artist, sizeof(song->artist), current->artist);
	copy_text(song->title, sizeof(song->title), current->title);
	return 1;
}