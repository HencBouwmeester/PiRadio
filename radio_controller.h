#ifndef RADIO_CONTROLLER_H
#define RADIO_CONTROLLER_H

typedef enum {
	IDLE,
	SHOW_TIME,
	FUNCTION_SELECT,
	AIRPLAY,
	PANDORA,
	WEB_RADIO,
	PRESET_SELECT
} States_t;

typedef enum {
	RADIO_EVENT_NONE,
	RADIO_EVENT_POWER,
	RADIO_EVENT_BAND,
	RADIO_EVENT_PRESET,
	RADIO_EVENT_TIMEOUT,
	RADIO_EVENT_PRESET_COMMIT,
	RADIO_EVENT_TUNE_UP,
	RADIO_EVENT_TUNE_DOWN,
	RADIO_EVENT_LOVE,
	RADIO_EVENT_BAN,
	RADIO_EVENT_NEXT_SONG,
	RADIO_EVENT_TIRED,
	RADIO_EVENT_PAUSE
} RadioControllerEvent;

typedef enum {
	RADIO_ACTION_NONE,
	RADIO_ACTION_WAKE,
	RADIO_ACTION_STOP_SERVICE,
	RADIO_ACTION_RETURN_FROM_MENU,
	RADIO_ACTION_SELECT_PANDORA_STATION,
	RADIO_ACTION_SELECT_PANDORA_PRESET,
	RADIO_ACTION_PANDORA_LOVE,
	RADIO_ACTION_PANDORA_BAN,
	RADIO_ACTION_PANDORA_NEXT,
	RADIO_ACTION_PANDORA_TIRED,
	RADIO_ACTION_PANDORA_PAUSE,
	RADIO_ACTION_SELECT_WEB_STATION
} RadioControllerAction;

typedef struct {
	States_t state;
	States_t previous_state;
	RadioControllerAction action;
	int station_index;
} RadioControllerResult;

RadioControllerResult radio_controller_transition(States_t state, States_t previous_state,
						   RadioControllerEvent event,
						   int station_index,
						   int pandora_station_count,
						   int web_station_count);
States_t radio_controller_select_service(int selection);
States_t radio_controller_cancel_preset(void);

#endif