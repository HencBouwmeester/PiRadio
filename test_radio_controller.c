#include <assert.h>

#include "radio_controller.h"

int main(void)
{
	RadioControllerResult result;

	result = radio_controller_transition(IDLE, IDLE, RADIO_EVENT_POWER, 0, 0, 0);
	assert(result.state == SHOW_TIME);
	assert(result.action == RADIO_ACTION_WAKE);

	result = radio_controller_transition(SHOW_TIME, IDLE, RADIO_EVENT_BAND, 0, 0, 0);
	assert(result.state == FUNCTION_SELECT);
	assert(result.previous_state == SHOW_TIME);

	result = radio_controller_transition(FUNCTION_SELECT, PANDORA, RADIO_EVENT_TIMEOUT, 0, 0, 0);
	assert(result.state == PANDORA);
	assert(result.action == RADIO_ACTION_RETURN_FROM_MENU);

	result = radio_controller_transition(PANDORA, PANDORA, RADIO_EVENT_TUNE_UP, 16, 18, 0);
	assert(result.action == RADIO_ACTION_SELECT_PANDORA_STATION);
	assert(result.station_index == 17);

	result = radio_controller_transition(PANDORA, PANDORA, RADIO_EVENT_TUNE_UP, 16, 17, 0);
	assert(result.station_index == 0);

	result = radio_controller_transition(PANDORA, PANDORA, RADIO_EVENT_TUNE_UP, 0, 0, 0);
	assert(result.action == RADIO_ACTION_NONE);

	result = radio_controller_transition(PRESET_SELECT, PANDORA, RADIO_EVENT_PRESET_COMMIT, 18, 18, 0);
	assert(result.state == PANDORA);
	assert(result.action == RADIO_ACTION_SELECT_PANDORA_PRESET);
	assert(result.station_index == 17);

	result = radio_controller_transition(WEB_RADIO, WEB_RADIO, RADIO_EVENT_TUNE_DOWN, 0, 0, 18);
	assert(result.action == RADIO_ACTION_SELECT_WEB_STATION);
	assert(result.station_index == 17);

	result = radio_controller_transition(AIRPLAY, AIRPLAY, RADIO_EVENT_POWER, 0, 0, 0);
	assert(result.state == IDLE);
	assert(result.action == RADIO_ACTION_STOP_SERVICE);
	assert(radio_controller_select_service(1) == AIRPLAY);
	assert(radio_controller_select_service(2) == PANDORA);
	assert(radio_controller_select_service(3) == WEB_RADIO);
	assert(radio_controller_cancel_preset() == IDLE);
	return 0;
}