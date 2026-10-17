#ifndef RADIO_SERVICES_H
#define RADIO_SERVICES_H

typedef enum {
	RADIO_PANDORA_LOVE,
	RADIO_PANDORA_BAN,
	RADIO_PANDORA_NEXT,
	RADIO_PANDORA_TIRED,
	RADIO_PANDORA_PAUSE,
	RADIO_PANDORA_QUIT
} RadioPandoraAction;

#define RADIO_MPD_TEXT_CAPACITY 512

typedef struct RadioMpdService RadioMpdService;

typedef struct {
	char name[RADIO_MPD_TEXT_CAPACITY];
	char artist[RADIO_MPD_TEXT_CAPACITY];
	char title[RADIO_MPD_TEXT_CAPACITY];
} RadioMpdSong;

typedef enum {
	RADIO_MANAGED_PANDORA,
	RADIO_MANAGED_AIRPLAY,
	RADIO_MANAGED_MPD
} RadioManagedService;

typedef enum {
	RADIO_SERVICE_START,
	RADIO_SERVICE_STOP,
	RADIO_SERVICE_RESTART
} RadioServiceAction;

int radio_pandora_send_action(const char *control_path, RadioPandoraAction action);
int radio_pandora_select_station(const char *control_path, int index, int station_count);
int radio_pandora_clear_nowplaying(const char *nowplaying_path);
int radio_pandora_stop(const char *control_path);

RadioMpdService *radio_mpd_service_create(const char *hostname, int port, const char *password);
void radio_mpd_service_free(RadioMpdService *service);
int radio_mpd_service_connect(RadioMpdService *service);
int radio_mpd_service_stop(RadioMpdService *service);
int radio_mpd_service_change_station(RadioMpdService *service, const char *station);
int radio_mpd_service_current_song(RadioMpdService *service, RadioMpdSong *song);
int radio_service_action(RadioManagedService service, RadioServiceAction action);

#endif