#include "radio_controller.h"

#include "station_index.h"

static RadioControllerResult result_for(States_t state, States_t previous_state)
{
	RadioControllerResult result;
	result.state = state;
	result.previous_state = previous_state;
	result.action = RADIO_ACTION_NONE;
	result.station_index = -1;
	return result;
}

RadioControllerResult radio_controller_transition(States_t state, States_t previous_state,
						   RadioControllerEvent event,
						   int station_index,
						   int pandora_station_count,
						   int web_station_count)
{
	RadioControllerResult result = result_for(state, previous_state);
	int selected_station;

	switch (state) {
	case IDLE:
		if (event == RADIO_EVENT_POWER) {
			result.state = SHOW_TIME;
			result.action = RADIO_ACTION_WAKE;
		}
		break;
	case SHOW_TIME:
		if (event == RADIO_EVENT_BAND) {
			result.state = FUNCTION_SELECT;
			result.previous_state = SHOW_TIME;
		} else if (event == RADIO_EVENT_TIMEOUT) {
			result.state = IDLE;
		}
		break;
	case FUNCTION_SELECT:
		if (event == RADIO_EVENT_TIMEOUT) {
			result.state = previous_state;
			result.action = RADIO_ACTION_RETURN_FROM_MENU;
		}
		break;
	case AIRPLAY:
		if (event == RADIO_EVENT_BAND) {
			result.state = FUNCTION_SELECT;
			result.previous_state = AIRPLAY;
		} else if (event == RADIO_EVENT_POWER) {
			result.state = IDLE;
			result.previous_state = AIRPLAY;
			result.action = RADIO_ACTION_STOP_SERVICE;
		}
		break;
	case PANDORA:
		if (event == RADIO_EVENT_BAND) {
			result.state = FUNCTION_SELECT;
			result.previous_state = PANDORA;
		} else if (event == RADIO_EVENT_POWER) {
			result.state = IDLE;
			result.previous_state = PANDORA;
			result.action = RADIO_ACTION_STOP_SERVICE;
		} else if (event == RADIO_EVENT_PRESET) {
			result.state = PRESET_SELECT;
			result.previous_state = PANDORA;
		} else if (event == RADIO_EVENT_TUNE_UP || event == RADIO_EVENT_TUNE_DOWN) {
			selected_station = station_index_step(station_index,
					event == RADIO_EVENT_TUNE_UP ? 1 : -1,
					pandora_station_count);
			if (selected_station >= 0) {
				result.action = RADIO_ACTION_SELECT_PANDORA_STATION;
				result.station_index = selected_station;
			}
		} else if (event == RADIO_EVENT_LOVE) {
			result.action = RADIO_ACTION_PANDORA_LOVE;
		} else if (event == RADIO_EVENT_BAN) {
			result.action = RADIO_ACTION_PANDORA_BAN;
		} else if (event == RADIO_EVENT_NEXT_SONG) {
			result.action = RADIO_ACTION_PANDORA_NEXT;
		} else if (event == RADIO_EVENT_TIRED) {
			result.action = RADIO_ACTION_PANDORA_TIRED;
		} else if (event == RADIO_EVENT_PAUSE) {
			result.action = RADIO_ACTION_PANDORA_PAUSE;
		}
		break;
	case WEB_RADIO:
		if (event == RADIO_EVENT_BAND) {
			result.state = FUNCTION_SELECT;
			result.previous_state = WEB_RADIO;
		} else if (event == RADIO_EVENT_POWER) {
			result.state = IDLE;
			result.previous_state = WEB_RADIO;
			result.action = RADIO_ACTION_STOP_SERVICE;
		} else if (event == RADIO_EVENT_PRESET) {
			result.state = PRESET_SELECT;
			result.previous_state = WEB_RADIO;
		} else if (event == RADIO_EVENT_TUNE_UP || event == RADIO_EVENT_TUNE_DOWN) {
			selected_station = station_index_step(station_index,
					event == RADIO_EVENT_TUNE_UP ? 1 : -1,
					web_station_count);
			if (selected_station >= 0) {
				result.action = RADIO_ACTION_SELECT_WEB_STATION;
				result.station_index = selected_station;
			}
		}
		break;
	case PRESET_SELECT:
		if (event == RADIO_EVENT_TIMEOUT) {
			result.state = previous_state;
			result.action = RADIO_ACTION_RETURN_FROM_MENU;
		} else if (event == RADIO_EVENT_PRESET_COMMIT) {
			result.state = previous_state;
			if (previous_state == PANDORA && pandora_station_count > 0) {
				selected_station = station_index > 0 ? station_index - 1 : 0;
				if (selected_station >= pandora_station_count)
					selected_station = pandora_station_count - 1;
				result.action = RADIO_ACTION_SELECT_PANDORA_PRESET;
				result.station_index = selected_station;
			} else if (previous_state == WEB_RADIO) {
				selected_station = station_index_step(station_index, -1, web_station_count);
				if (selected_station >= 0) {
					result.action = RADIO_ACTION_SELECT_WEB_STATION;
					result.station_index = selected_station;
				}
			}
		} else {
			result.state = previous_state;
		}
		break;
	}

	return result;
}

States_t radio_controller_select_service(int selection)
{
	switch (selection) {
	case 1:
		return AIRPLAY;
	case 2:
		return PANDORA;
	case 3:
		return WEB_RADIO;
	default:
		return IDLE;
	}
}

States_t radio_controller_cancel_preset(void)
{
	return IDLE;
}