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

int radio_pandora_send_action(const char *control_path, RadioPandoraAction action);
int radio_pandora_select_station(const char *control_path, int index, int station_count);
int radio_pandora_clear_nowplaying(const char *nowplaying_path);

#endif