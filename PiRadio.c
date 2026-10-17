#include "PiRadio.h"
#include "station_index.h"
#include "radio_playlist.h"
#include "radio_services.h"
#include <sys/wait.h>
#include <sys/time.h>

#define TIME_OUT 5  // One of the system's FSM transitions
#define KEYSCAN_POLL_DELAY_MS 100
#define KEYSCAN_DEBOUNCE_DELAY_MS 10
#define KEYSCAN_STABLE_READS 2

// keypad values
#define KEYPAD_NONE 			-1
#define KEYPAD_PRESET		12
#define KEYPAD_SEEK_UP		14
#define KEYPAD_3				3
#define KEYPAD_TUNE_UP		11
#define KEYPAD_1				1
#define KEYPAD_2				2
#define KEYPAD_FREQUENCY	16
#define KEYPAD_SEEK_DN		17
#define KEYPAD_6				6
#define KEYPAD_TUNE_DN		10
#define KEYPAD_4				4
#define KEYPAD_5				5
#define KEYPAD_PWR			13
#define KEYPAD_9				9
#define KEYPAD_HD_UP			15
#define KEYPAD_7				7
#define KEYPAD_8				8
#define KEYPAD_INFO			18
#define KEYPAD_HD_DN			19
#define KEYPAD_BAND			20
#define KEYPAD_0				0

#define I2C_FILE_NAME "/dev/i2c-1"
/*#define mpd_playlist_len 47*/

// timer macros
#define TIMER_CLEAR     (tv1.tv_sec = tv1.tv_usec = tv2.tv_sec = tv2.tv_usec = 0)
#define TIMER_START     gettimeofday(&tv1, (struct timezone*)0)
#define TIMER_ELAPSED   ((tv2.tv_usec-tv1.tv_usec)+((tv2.tv_sec-tv1.tv_sec)*1000000))
#define TIMER_STOP      gettimeofday(&tv2, (struct timezone*)0)
struct timeval tv1,tv2;
#define TIMEOUT_CLEAR     (tv3.tv_sec = tv3.tv_usec = tv4.tv_sec = tv4.tv_usec = 0)
#define TIMEOUT_START     gettimeofday(&tv3, (struct timezone*)0)
#define TIMEOUT_ELAPSED   ((tv4.tv_usec-tv3.tv_usec)+((tv4.tv_sec-tv3.tv_sec)*1000000))
#define TIMEOUT_STOP      gettimeofday(&tv4, (struct timezone*)0)
struct timeval tv3,tv4;
#define SHOWTIME_CLEAR     (tv5.tv_sec = tv5.tv_usec = tv6.tv_sec = tv6.tv_usec = 0)
#define SHOWTIME_START     gettimeofday(&tv5, (struct timezone*)0)
#define SHOWTIME_ELAPSED   ((tv6.tv_usec-tv5.tv_usec)+((tv6.tv_sec-tv5.tv_sec)*1000000))
#define SHOWTIME_STOP      gettimeofday(&tv6, (struct timezone*)0)
struct timeval tv5,tv6;

unsigned int idleStatus = 0;

States_t state;
States_t prevState;

Services_t runningService;

int i2c_file, addr;
uint8_t fh, fw, maxchar;

int station_digit[2];
char station_name[50];
Song current;
unsigned int updated_song = 0;
int counter = 0;
int station;
RadioMpdService *mpd_service = NULL;
int iport = 6600;
char *hostname;
char *password;

const unsigned int keys[4][6] = {
	{KEYPAD_PRESET, KEYPAD_SEEK_UP, KEYPAD_3, KEYPAD_TUNE_UP, KEYPAD_1, KEYPAD_2},
	{KEYPAD_FREQUENCY, KEYPAD_SEEK_DN, KEYPAD_6, KEYPAD_TUNE_DN, KEYPAD_4, KEYPAD_5},
	{KEYPAD_NONE, KEYPAD_PWR, KEYPAD_9, KEYPAD_HD_UP, KEYPAD_7, KEYPAD_8},
	{KEYPAD_NONE, KEYPAD_NONE, KEYPAD_INFO, KEYPAD_HD_DN, KEYPAD_BAND, KEYPAD_0}};

int oldMinutes;
int scrollCount = 0;

time_t oldTime;
int ch;
struct stat nowplayingStat;
int num_stations = 0;
FILE *fp;

int mpd_playlist_len = 1;
char MPDplaylist[RADIO_PLAYLIST_LIMIT][RADIO_PLAYLIST_FILENAME_CAPACITY];

int paused = 0;
int show_pause = 0;

static void run_command(const char *command)
{
	int status = system(command);

	if (status == -1) {
		perror("Unable to start command");
	} else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		fprintf(stderr, "Command exited with status %d: %s\n",
			WEXITSTATUS(status), command);
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr, "Command terminated by signal %d: %s\n",
			WTERMSIG(status), command);
	} else if (!WIFEXITED(status)) {
		fprintf(stderr, "Command did not exit normally: %s\n", command);
	}
}

static void stop_mpd_playback_on_startup(void)
{
	RadioMpdService *startup_mpd;

	if (!hostname)
		hostname = "localhost";

	startup_mpd = radio_mpd_service_create(hostname, iport, password);
	if (!startup_mpd) {
		fprintf(stderr, "Unable to create MPD client during startup.\n");
		return;
	}

	if (radio_mpd_service_stop(startup_mpd) != 0) {
		fprintf(stderr, "Unable to connect to MPD to stop startup playback.\n");
	}
	radio_mpd_service_free(startup_mpd);
}

static void send_pandora_action(RadioPandoraAction action)
{
	if (radio_pandora_send_action("/home/pi/.config/pianobar/ctl", action) != 0)
		perror("Unable to write Pianobar control command");
}

static int select_pandora_station(int index)
{
	if (radio_pandora_select_station("/home/pi/.config/pianobar/ctl",
					 index, num_stations) != 0) {
		perror("Unable to select Pianobar station");
		return -1;
	}
	return 0;
}

int main(){

	int key;

	hostname = getenv("MPD_HOST");
	password = getenv("MPD_PASSWORD");
	stop_mpd_playback_on_startup();
	TIMER_CLEAR;
	SHOWTIME_CLEAR;
	SHOWTIME_START;

	// Initialize SPI, GPIO, LCD, and Keypad
	if(SPI_Setup())
	{
		printf("ERROR : SPI not initialized!\n");
		return 1;
	}
	GPIO_Setup();

	nju6676Init();
	if( keyscan_init()) {
		printf("Unable to initialize keypad matrix\n");
		return 1;
	}

	// Set the default font, width, and height
	nju6676SelectFont(System5x7, nju6676ReadFontData, WHITE);
	fh = nju6676ReadFontData(System5x7+FONT_HEIGHT);
	fw = nju6676ReadFontData(System5x7+FONT_FIXED_WIDTH);
	maxchar = 128/(fw+1);

	/*int delayCount;*/
	while (1)
	{
		switch (state)
		{
			case IDLE:
				idle();
				break;
			case SHOW_TIME:
				showTime();
				break;
			case FUNCTION_SELECT:
				functionSelect();
				break;
			case AIRPLAY:
				funcAirplay();
				break;
			case PANDORA:
				funcPandora();
				break;
			case WEB_RADIO:
				funcWebRadio();
				break;
			case PRESET_SELECT:
				funcPreset();
				break;
		}

		if ((key = keyscan()) != KEYPAD_NONE)
		{
#ifdef DEBUG
			printf("Key: %d\n", key);
#endif
			transition(key);
		}
		scrollCount++;
		delay(KEYSCAN_POLL_DELAY_MS);

	}

	return 0;

}

void transition( int trigger)
{
	int w, h;

	switch (state)
	{
		case IDLE:
			if (trigger == KEYPAD_PWR ) 
			{
				state = SHOW_TIME;
				idleStatus = 0;
			}
			break;
		case SHOW_TIME:
			if ( trigger == KEYPAD_BAND )
			{
				state = FUNCTION_SELECT;
				prevState = SHOW_TIME;
			}
			else if ( trigger == TIME_OUT )
				state = IDLE;
			break;
		case FUNCTION_SELECT:
			if ( trigger == TIME_OUT )
			{
				state = prevState;
				if (state == WEB_RADIO)
					current.Title[0] = 0; // force a redraw of the screen
				else if (state == PANDORA)
					oldTime = -1;
			}
			break;
		case AIRPLAY:
			if (trigger == KEYPAD_BAND)
			{
				state = FUNCTION_SELECT;
				prevState = AIRPLAY;
			}
			else if (trigger == KEYPAD_PWR)
			{
				state = IDLE;
				prevState = AIRPLAY;
				stopServices( runningService );
			}
			break;
		case PANDORA:
			switch (trigger)
			{
				case KEYPAD_BAND:
					state = FUNCTION_SELECT;
					prevState = PANDORA;
					break;
				case KEYPAD_PWR:
					state = IDLE;
					prevState = PANDORA;
					stopServices( runningService );
					break;
				case KEYPAD_PRESET:
					state = PRESET_SELECT;
					prevState = PANDORA;
					break;
				case KEYPAD_TUNE_UP:
					// next station
					w = station_index_step(station, 1, num_stations);
					if (w >= 0 && select_pandora_station(w) == 0) {
						station = w;
						current.StationNum = station;
						retr_msg();
						updated_song = 0;
					}
					break;
				case KEYPAD_TUNE_DN:
					// previous station
					w = station_index_step(station, -1, num_stations);
					if (w >= 0 && select_pandora_station(w) == 0) {
						station = w;
						current.StationNum = station;
						retr_msg();
						updated_song = 0;
					}
					break;
				case KEYPAD_SEEK_UP:
					// love song
					send_pandora_action(RADIO_PANDORA_LOVE);

					// we are going to blink the heart so that it is obvious that it has been done
					w = pandora_heart[0]; h = pandora_heart[1];
					nju6676DrawBitmap(120,56,&pandora_heart[2],w,h,WHITE);
					nju6676Display();
					delay(300);
					nju6676FillRect(120,56,8,8,BLUE);
					nju6676Display();
					delay(300);
					w = pandora_heart[0]; h = pandora_heart[1];
					nju6676DrawBitmap(120,56,&pandora_heart[2],w,h,WHITE);
					nju6676Display();
					break;
				case KEYPAD_SEEK_DN:
					// ban song
					send_pandora_action(RADIO_PANDORA_BAN);
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_HD_UP:
					// next song
					send_pandora_action(RADIO_PANDORA_NEXT);
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_HD_DN:
					// tired song
					send_pandora_action(RADIO_PANDORA_TIRED);
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_FREQUENCY:
					// pause/un-pause the currently playing song
					if (radio_pandora_send_action("/home/pi/.config/pianobar/ctl",
								      RADIO_PANDORA_PAUSE) == 0)
						paused = !paused;
					else
						perror("Unable to pause Pianobar");
					if (!paused)
					{
						nju6676FillRect(110,56,7,8,BLUE);
						nju6676Display();
					}
					break;
			}
#ifdef DEBUG
			printf("State: PANDORA\n");
#endif
			break;
		case WEB_RADIO:
			switch (trigger)
			{
				case KEYPAD_BAND:
					state = FUNCTION_SELECT;
					prevState = WEB_RADIO;
					break;
				case KEYPAD_PWR:
					state = IDLE;
					prevState = WEB_RADIO;
					stopServices( runningService );
					break;
				case KEYPAD_TUNE_UP:
					w = station_index_step(station, 1, mpd_playlist_len);
					if (w >= 0 && radio_mpd_service_change_station(mpd_service, MPDplaylist[w]) == 0)
					{
						station = w;
						current.Title[0] = 0; // force a redraw of the screen
					}
					break;
				case KEYPAD_TUNE_DN:
					w = station_index_step(station, -1, mpd_playlist_len);
					if (w >= 0 && radio_mpd_service_change_station(mpd_service, MPDplaylist[w]) == 0)
					{
						station = w;
						current.Title[0] = 0; // force a redraw of the screen
					}
					break;
				case KEYPAD_PRESET:
					state = PRESET_SELECT;
					prevState = WEB_RADIO;
					break;
			}
#ifdef DEBUG
			printf("State: WEB_RADIO\n");
#endif
			break;
		case PRESET_SELECT:
			if ( trigger == TIME_OUT )
			{
				state = prevState;
				if (state == WEB_RADIO)
					current.Title[0] = 0; // force a redraw of the screen
				else if (state == PANDORA)
					oldTime = -1;
			}
			else
			{
				state = prevState;
				if (state == WEB_RADIO)
					transition(KEYPAD_TUNE_DN); // station is returned one too high on purpose, so this allows us to use TUNE_DN
				else if (state == PANDORA)
				{
					if (num_stations > 0) {
						w = station > 0 ? station - 1 : 0;
						if (w >= num_stations)
							w = num_stations - 1;
						if (select_pandora_station(w) == 0) {
							station = w;
							current.StationNum = station;
							retr_station_msg();
							updated_song = 0;
						}
					}
				}
			}
#ifdef DEBUG
			printf("State: PRESET_SELECT\n");
#endif
			break;
		default:
				break;
		}
	}

void idle()
{
	if (!idleStatus)
	{
		// clear the DDRAM and upload the digital clock, then send to display
		LED_state(ON);
		nju6676Backlight( OFF );
		nju6676ClearScreen(BLUE);
		oldMinutes = drawclock( -1 );
		stopServices( runningService );
		idleStatus = 1;
	}
	else
	{
		time_t rawtime;
		struct tm*  time_;

		time(&rawtime);
		time_ = localtime(&rawtime);
		if(time_->tm_min != oldMinutes)
			oldMinutes = drawclock( oldMinutes );
	}

}

void stopServices( Services_t currentService )
{
	if (currentService != allStop)
		switch (currentService)
		{
			case airPlayService:
				run_command("sudo service shairport stop >/dev/null 2>&1");
				break;
			case pandoraService:
				send_pandora_action(RADIO_PANDORA_QUIT);
				paused = 0;
				break;
			case webRadioService:
				if (mpd_service) {
					if (radio_mpd_service_stop(mpd_service) != 0)
						fprintf(stderr, "Unable to stop MPD playback.\n");
					radio_mpd_service_free(mpd_service);
					mpd_service = NULL;
				}
				current.Title[0] = 0; // force a redraw of the screen
				break;
			case allStop:
				paused = 0;
				break;
		}
	runningService = allStop;
}

void showTime()
{
	int key;
	int timeOut = 1;

	// clear the DDRAM and upload the digital clock, then send to display
	nju6676Backlight( ON );
	nju6676ClearScreen(BLUE);
	oldMinutes = -1; // clear out old time so that we force an update
	oldMinutes = drawclock( oldMinutes );
	nju6676Display();

	LED_state(OFF);

	TIMEOUT_CLEAR;
	TIMEOUT_START;
	TIMEOUT_STOP;

	while (TIMEOUT_ELAPSED < 5000000)
	{
		key = keyscan();
		if (key == KEYPAD_BAND)
		{
			timeOut = 0;
			break;
		}
		TIMEOUT_STOP;
	}

	if ( !timeOut ) transition(KEYPAD_BAND);
	else transition(TIME_OUT);
}

void functionSelect()
{
	int key;
	int timeOut = 1;

	nju6676FillRect(0,0,128,64,BLUE);
	nju6676GotoXY(34,8);
	nju6676Puts("1: AIRPLAY");
	nju6676GotoXY(34,24);
	nju6676Puts("2: PANDORA");
	nju6676GotoXY(34,40);
	nju6676Puts("3: WEBRADIO");
	nju6676Display();

	TIMEOUT_CLEAR;
	TIMEOUT_START;
	TIMEOUT_STOP;

	while (TIMEOUT_ELAPSED < 5000000)
	{
		key = keyscan();
		if (key != KEYPAD_NONE)
		{
			switch (key)
			{
				case KEYPAD_1:
					state = AIRPLAY;
					timeOut = 0;
					break;
				case KEYPAD_2:
					state = PANDORA;
					timeOut = 0;
					break;
				case KEYPAD_3:
					state = WEB_RADIO;
					timeOut = 0;
					break;
				case KEYPAD_BAND:
					timeOut = 1;
					break;
				case KEYPAD_PWR:
					state = IDLE;
					timeOut = 0;
					break;
			}
			break;
		}
		TIMEOUT_STOP;
	}

	if ( !timeOut ) transition(KEYPAD_NONE);
	else transition(TIME_OUT);
}

void funcAirplay()
{
	int w, h;

	if ( runningService != airPlayService )
	{
		stopServices( runningService );

		nju6676ClearScreen(BLUE);
		w = airplay[0]; h = airplay[1];
		nju6676DrawBitmap((SCREEN_WIDTH-w)/2,(SCREEN_HEIGHT-h)/2,&airplay[2],w,h,WHITE);
		nju6676Display();
		run_command("sudo service shairport restart >/dev/null 2>&1");
		runningService = airPlayService;
	}
}

void funcWebRadio()
{
	int w, h, k, X;
	FILE *fp, *fplist;
	char plistname[512];
	RadioPlaylistEntry *playlist_entries = NULL;
	RadioMpdSong mpd_song;
	size_t playlist_count = 0;
	size_t i;
	int path_length;
	int write_ok;


	if ( runningService != webRadioService )
	{

		stopServices( runningService );
		mpd_playlist_len = 0;

		if((fp = fopen("/home/pi/webradioplaylist.txt", "r")) == NULL) {
			return;
		}
		if (radio_playlist_load(fp, &playlist_entries, &playlist_count) != 0) {
			fclose(fp);
			fprintf(stderr, "Invalid web radio playlist configuration.\n");
			return;
		}
		fclose(fp);
		if (playlist_count == 0) {
			radio_playlist_free(playlist_entries);
			return;
		}

		if (clear_playlists() != 0) {
			radio_playlist_free(playlist_entries);
			return;
		}
		mpd_playlist_len = 0;

		for (i = 0; i < playlist_count; i++) {
			path_length = snprintf(plistname, sizeof(plistname),
					       "/var/lib/mpd/playlists/%s",
					       playlist_entries[i].filename);
			if (path_length < 0 || (size_t)path_length >= sizeof(plistname)) {
				radio_playlist_free(playlist_entries);
				return;
			}

			fplist = fopen(plistname, "w");
			if (!fplist) {
				radio_playlist_free(playlist_entries);
				return;
			}
			write_ok = fprintf(fplist, "%s\n", playlist_entries[i].url) >= 0;
			if (fclose(fplist) != 0)
				write_ok = 0;
			if (!write_ok) {
				radio_playlist_free(playlist_entries);
				return;
			}

			snprintf(MPDplaylist[i], sizeof(MPDplaylist[i]), "%s",
				 playlist_entries[i].name);
		}
		mpd_playlist_len = (int)playlist_count;
		radio_playlist_free(playlist_entries);

		if(!hostname)
			hostname = "localhost";
#ifdef DEBUG
		printf("%s\n", hostname);
#endif
		mpd_service = radio_mpd_service_create(hostname, iport, password);
		if (!mpd_service)
			return;

		// if the service has stopped or there is an error, restart it
		if (radio_mpd_service_connect(mpd_service) != 0) {
			run_command("mkdir /var/log/mpd");
			run_command("touch /var/log/mpd/mpd.log");
			run_command("sudo service mpd restart >/dev/null 2>&1");
		}

		// draw the splash screen
		nju6676ClearScreen(BLUE);
		w = radio[0]; h = radio[1];
		nju6676DrawBitmap((SCREEN_WIDTH-w)/2,(SCREEN_HEIGHT-h)/2,&radio[2],w,h,WHITE);
		nju6676Display();

		// set the station to the first one (station zero)
		station = 0; station_digit[0] = 0; station_digit[1] = 0;
#ifdef DEBUG
		printf("%s\n", MPDplaylist[station]);
#endif
		if (radio_mpd_service_change_station(mpd_service, MPDplaylist[station]) != 0)
			fprintf(stderr, "Unable to start MPD playlist: %s\n", MPDplaylist[station]);

		// now show the radio tagline
		bcm2835_delay(1000);
		nju6676ClearScreen(BLUE);
		nju6676DrawHLine(0,54,128,WHITE);
		w = radio_tag[0]; h = radio_tag[1];
		nju6676DrawBitmap((SCREEN_WIDTH-w)/2,SCREEN_HEIGHT-h,&radio_tag[2],w,h,WHITE);
		nju6676Display();

		runningService = webRadioService;
	}
	else
	{
		if (scrollCount > 4)  // needed to slow down refresh of screen
		{
			// can we create a connection?
			if (radio_mpd_service_current_song(mpd_service, &mpd_song) >= 0)
			{
				// check for valid info, otherwise we make up our own
				if (mpd_song.title[0])
				{
					if (strncmp(mpd_song.title,current.Title,21) != 0)
					{
						update_current_song(MPDplaylist[station], mpd_song.name,
								   mpd_song.artist, mpd_song.title);
						counter = 0;
						updated_song = 1;
						// static text version here...
						if (current.Station_strlen < 22)
							static_text(current.Station, current.Station_strlen, 8);
						if (current.Title_strlen < 22)
							static_text(current.Title, current.Title_strlen, 24);
						if (current.Artist_strlen < 22)
							static_text(current.Artist, current.Artist_strlen, 32);
						if (current.Album_strlen < 22)
							static_text(current.Album, current.Album_strlen, 40);

					} else {
						// scrolling text version here...
						if (updated_song)
						{
							if (current.Station_strlen > 21)
								scroll_text( current.Station, current.Station_strlen, counter, 8);
							if (current.Title_strlen > 21)
								scroll_text( current.Title, current.Title_strlen, counter, 24);
							if (current.Artist_strlen > 21)
								scroll_text( current.Artist, current.Artist_strlen, counter, 32);
							if (current.Album_strlen > 21)
								scroll_text( current.Album, current.Album_strlen, counter, 40);
							counter++;
						}
					}
				} else {
					// no valid info, so display the current station filename
					nju6676FillRect(0,0,128,53,BLUE);
					for (k=0;k<strlen(MPDplaylist[station]);k++)
						station_name[k] = MPDplaylist[station][k];
					station_name[strlen(MPDplaylist[station])] = '\0';
					if (strlen(station_name)>maxchar)
					{
						station_name[maxchar] = '\0';
						X = 1;
					} else {
						X = (SCREEN_WIDTH-(strlen(station_name)*fw+strlen(station_name)-1))/2;
					}
					nju6676GotoXY(X,8);
					nju6676Puts(station_name);
				}

				// display horizontal line, station number in lower left corner, and radio tag
				nju6676FillRect(0,56,14,8,BLUE);
				nju6676GotoXY(1,56);
				nju6676PrintNumber((char)((station+1) / 10));
				nju6676GotoXY(7,56);
				nju6676PrintNumber((char)((station+1) - (((station+1) / 10) * 10)));
				nju6676DrawHLine(0,54,128,WHITE);
				w = radio_tag[0]; h = radio_tag[1];
				nju6676DrawBitmap((SCREEN_WIDTH-w)/2,SCREEN_HEIGHT-h,&radio_tag[2],w,h,WHITE);

				nju6676Display();
			}
			scrollCount = 0;
		}
	}
}

int funcPandora(void)
{
	int w, h;
	char station_line[512];

	if ( runningService != pandoraService )
	{
		stopServices( runningService );

		// Splash screen
		nju6676ClearScreen(BLUE);
		w = pandora[0]; h = pandora[1];
		nju6676DrawBitmap((SCREEN_WIDTH-w)/2,(SCREEN_HEIGHT-h)/2,&pandora[2],w,h,WHITE);
		nju6676Display();

		// Horizontal separation line
		nju6676DrawHLine(0,54,128,WHITE);

		// check to see if pianobar is already running
		pid_t pid = proc_find("pianobar");
		// it is running, so quit it
		if (pid != -1) send_pandora_action(RADIO_PANDORA_QUIT);

		// now it is not running, so clear out the current song info
		if (radio_pandora_clear_nowplaying("/home/pi/.config/pianobar/nowplaying") != 0) {
			perror("Unable to clear Pianobar now-playing file");
			return 1;
		}
		if (stat("/home/pi/.config/pianobar/nowplaying", &nowplayingStat) < 0)
			return 1;
		oldTime = nowplayingStat.st_mtime;
		// start pianobar
		run_command("su - pi -c \"nohup pianobar >/dev/null 2>&1&\"");
		bcm2835_delay(5000);
		// wait for the next refresh
		retr_msg();
		updated_song = 0;

		ch = 0; station = 0; num_stations = 0;
		fp = fopen("/home/pi/.config/pianobar/stationlist","r");
		if (!fp) {
			perror("Unable to open Pianobar station list");
		} else {
			while (fgets(station_line, sizeof(station_line), fp) != NULL)
				num_stations++;
			fclose(fp);
		}

		nju6676Display();

		runningService = pandoraService;
	}
	else
	{
		if (scrollCount > 4)  // needed to slow down refresh of screen
		{
			// get current time stamp of the file
			if (stat("/home/pi/.config/pianobar/nowplaying", &nowplayingStat) < 0)
			{
				printf("ERROR: Could not get current time stamp of file.\n");
				return 1;
			}

			// update the song information if the time stamp differs
			if (nowplayingStat.st_mtime > oldTime)
			{
				nju6676FillRect(0,0,128,53,BLUE);
				nju6676FillRect(120,56,8,8,BLUE);
				nju6676DrawHLine(0,54,128,WHITE);
				clear_current();
				parse_nowplaying("/home/pi/.config/pianobar/nowplaying");
				oldTime = nowplayingStat.st_mtime;
				counter = 0;
				updated_song = 1;

				// static text version here...
				if (current.Artist_strlen < 22)
					static_text(current.Artist, current.Artist_strlen, 8);
				if (current.Title_strlen < 22)
					static_text(current.Title, current.Title_strlen, 16);
				if (current.Album_strlen < 22)
					static_text(current.Album, current.Album_strlen, 24);
				if (current.Station_strlen < 22)
					static_text(current.Station, current.Station_strlen, 40);

				if (current.Rating)
				{
					w = pandora_heart[0]; h = pandora_heart[1];
					nju6676DrawBitmap(120,56,&pandora_heart[2],w,h,WHITE);
				}

			}
			else
			{
				// scrolling text version here...
				if (updated_song)
				{
					if (current.Artist_strlen > 21)
						scroll_text( current.Artist, current.Artist_strlen, counter, 8);
					if (current.Title_strlen > 21)
						scroll_text( current.Title, current.Title_strlen, counter, 16);
					if (current.Album_strlen > 21)
						scroll_text( current.Album, current.Album_strlen, counter, 24);
					if (current.Station_strlen > 21)
						scroll_text( current.Station, current.Station_strlen, counter, 40);
					counter++;
				}

			}

			// display station number in lower left corner
			nju6676FillRect(0,56,14,8,BLUE);
			nju6676GotoXY(1,56);
			nju6676PrintNumber((char)((station+1) / 10));
			nju6676GotoXY(7,56);
			nju6676PrintNumber((char)((station+1) - (((station+1) / 10) * 10)));
			nju6676Display();
			scrollCount = 0;
			if (paused)
			{
				if (show_pause)
				{
					show_pause = !show_pause;
					nju6676FillRect(110,56,2,7,WHITE);
					nju6676FillRect(113,56,2,7,BLUE);
					nju6676FillRect(115,56,1,7,WHITE);
					nju6676Display();
				}
				else
				{
					show_pause = !show_pause;
					nju6676FillRect(110,56,7,8,BLUE);
					nju6676Display();
				}
			}
		}
	}

	return 0;
}

void funcPreset(void)
{
	int key;
	int timeOut = 1;
	int entryComplete = 0;
	int digit = 0;

	nju6676FillRect(0,0,128,53,BLUE);
	nju6676GotoXY(1,0);
	nju6676Puts("*GOTO PRESET STATION*");
	nju6676Display();

	TIMEOUT_CLEAR;
	TIMEOUT_START;
	TIMEOUT_STOP;

	while (TIMEOUT_ELAPSED < 5000000)
	{
		key = keyscan();
		if (key != KEYPAD_NONE)
		{
			switch (key)
			{
				case KEYPAD_PWR:
					state = IDLE;
					entryComplete = 1;
					break;
				case KEYPAD_PRESET:
					entryComplete = 1;
					break;
				default:
					if (( key >= 0 ) && ( key < 10 ))
					{
						TIMEOUT_START;  // reset the timer
						if (!digit)
						{
							nju6676FillRect(68,20,15,24,BLUE);
							disp_preset( key, digit);
							nju6676Display();
							station = key;
							digit = 1;
						}
						else
						{
							nju6676FillRect(0,8,128,45,BLUE);
							disp_preset( key, 0);
							disp_preset( station, digit);
							nju6676Display();
							station = station*10 + key;
						}
					}
					timeOut = 0;
					break;
			}
			if (entryComplete)
				break;
		}
		delay(KEYSCAN_DEBOUNCE_DELAY_MS);  // debounce the switch
		TIMEOUT_STOP;
	}

	if ( !timeOut ) transition(KEYPAD_NONE);
	else transition(TIME_OUT);

}

void disp_preset( int digit, int position )
{
		nju6676SelectFont(Verdana24, nju6676ReadFontData, WHITE);
		nju6676GotoXY(68-20*position,20);
		nju6676PrintNumber((char)digit);
		nju6676SelectFont(System5x7, nju6676ReadFontData, WHITE);
}

int SPI_Setup(void)
{
	if (!bcm2835_init()) return 1;

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);                   
// 	bcm2835_spi_setClockDivider(2048); ///< 2048 = 8.192us = 122.0703125kHz
	bcm2835_spi_setClockDivider(8); ///< 8 = 32ns = 31.25MHz 
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      
	return 0;
}

void GPIO_Setup(void)
{
	bcm2835_gpio_fsel(LCD_RST,    BCM2835_GPIO_FSEL_OUTP); // RST
	bcm2835_gpio_fsel(LCD_A0,     BCM2835_GPIO_FSEL_OUTP); // A0
	bcm2835_gpio_fsel(POWER_EN,   BCM2835_GPIO_FSEL_OUTP); // POWER_EN
	bcm2835_gpio_fsel(LED_ON_OFF, BCM2835_GPIO_FSEL_OUTP); // LED_ON_OFF

	bcm2835_gpio_fsel(I2C_RST,    BCM2835_GPIO_FSEL_OUTP); // I2C_RST
	// toggle reset for MPC23017
	bcm2835_gpio_write(I2C_RST, ON);
	bcm2835_delay(500);
	bcm2835_gpio_write(I2C_RST, OFF);
	bcm2835_delay(500);
	bcm2835_gpio_write(I2C_RST, ON);

	// turn LED on
	LED_state(ON);
}

void LED_state( int State )
{
	bcm2835_gpio_write(LED_ON_OFF, State);
}

int set_i2c_register(int file, unsigned char addr, unsigned char reg, unsigned char value)
{

	unsigned char outbuf[2];
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];

	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = sizeof(outbuf);
	messages[0].buf   = outbuf;

	/* The first byte indicates which register we'll write */
	outbuf[0] = reg;

	/* 
	 * The second byte indicates the value to write.  Note that for many
	 * devices, we can write multiple, sequential registers at once by
	 * simply making outbuf bigger.
	 */
	outbuf[1] = value;

	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs  = messages;
	packets.nmsgs = 1;
	if(ioctl(file, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return 1;
	}

	return 0;
}


int get_i2c_register(int file, unsigned char addr, unsigned char reg, unsigned char *val) {
	unsigned char inbuf, outbuf;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];

	/*
	 * In order to read a register, we first do a "dummy write" by writing
	 * 0 bytes to the register we want to read from.  This is similar to
	 * the packet in set_i2c_register, except it's 1 byte rather than 2.
	 */
	outbuf = reg;
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = sizeof(outbuf);
	messages[0].buf   = &outbuf;

	/* The data will get returned in this structure */
	messages[1].addr  = addr;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len   = sizeof(inbuf);
	messages[1].buf   = &inbuf;

	/* Send the request to the kernel and get the result back */
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if(ioctl(file, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return 1;
	}
	*val = inbuf;

	return 0;
}

int keyscan_colsetup(void)
{
	// switch back to original setup

	// write data value 0b11110000 into register 00 (IODIRA) 
	// makes GPA0-3 outputs, rest of the pins  (GPA4-7) inputs
	// since these are the rows
	if(set_i2c_register(i2c_file, addr, 0x00, 0b11110000)) {
		printf("Unable to set register!\n");
		return 1;
	}
	// write data value 0b00001111 into register 12 (GPIOA) 
	// sets them all to LOW
	if(set_i2c_register(i2c_file, addr, 0x12, 0b00000000)) {
		printf("Unable to set register!\n");
		return 1;
	}
	// write data value 0b11111111 into register 01 (IODIRB) 
	// makes GPB0-7 inputs since these are the columns
	if(set_i2c_register(i2c_file, addr, 0x01, 0b11111111)) {
		printf("Unable to set register!\n");
		return 1;
	}

	return 0;
}

int keyscan_rowsetup(void)
{
	// a key has been pressed so flip and check the rows
	// now the rows become inputs, so GPA0-3 are inputs
	if(set_i2c_register(i2c_file, addr, 0x00, 0b11111111)) {
		printf("Unable to set register!\n");
		exit(1);
	}
	// and the columns become outputs
	if(set_i2c_register(i2c_file, addr, 0x12, 0b11000000)) {
		printf("Unable to set register!\n");
		exit(1);
	}
	// with value set to HIGH
	if(set_i2c_register(i2c_file, addr, 0x01, 0b00111111)) {
		printf("Unable to set register!\n");
		exit(1);
	}

	return 0;
}

int keyscan_init(void)
{

	//specify a device address 0x24 and i2c device "/dev/i2c-1" 
	addr = 0x24;
	// Open a connection to the I2C userspace control file.
	if ((i2c_file = open(I2C_FILE_NAME, O_RDWR)) < 0) {
		perror("Unable to open i2c control file");
		return 1;
	}

	return keyscan_colsetup();
}

unsigned char scan_cols(void)
{
	unsigned char c;
	if(keyscan_colsetup()) {
		printf("Unable to setup colscan!\n");
		exit(1);
	}

	if(get_i2c_register(i2c_file, addr, 0x13, &c)) {
		printf("Unable to get register!\n");
		exit(1);
	}

	return c;
}

unsigned char scan_rows(void)
{
	unsigned char r;
	if(keyscan_rowsetup()) {
		printf("Unable to setup rowscan!\n");
		exit(1);
	}
	if(get_i2c_register(i2c_file, addr, 0x12, &r)) {
		printf("Unable to get register!\n");
		exit(1);
	}
	return r;
}

int keyscan(void)
{
	static int last_key = KEYPAD_NONE;
	static int key_held = 0;
	int col, row;
	unsigned char c, r;
	int stable_reads = 0;
	int pressed_key;

	c = scan_cols();
	if (c == 0x3F) {
		keyscan_colsetup();
		key_held = 0;
		last_key = KEYPAD_NONE;
		return KEYPAD_NONE;
	}

	if (key_held) {
		keyscan_colsetup();
		return KEYPAD_NONE;
	}

	while (stable_reads < KEYSCAN_STABLE_READS) {
		if (scan_cols() != c) {
			keyscan_colsetup();
			key_held = 0;
			last_key = KEYPAD_NONE;
			return KEYPAD_NONE;
		}
		stable_reads++;
		delay(KEYSCAN_DEBOUNCE_DELAY_MS);
	}

	r = scan_rows();
	col = 0; row = 0;
	c += 0b11000000; c = ~c;
	if(c != 0) {
		while (c != 1){
			c = c >> 1;
			col++;
		}
	}
	if(r != 0) {
		while (r != 1){
			r = r >> 1;
			row++;
		}
	}

	pressed_key = keys[row][col];
	keyscan_colsetup();

	if (pressed_key == KEYPAD_NONE)
		return KEYPAD_NONE;
	if (pressed_key == last_key)
		return KEYPAD_NONE;

	key_held = 1;
	last_key = pressed_key;
	return pressed_key;
}

int drawclock( int oldMinutes ) {
	time_t temps = time(0);
	struct tm *now = localtime(&temps);
	int hours, 
		 minutes, 
		 clock[4]; 
	unsigned int PM;
	char text[100],
	     bufferH[10],
	     bufferM[10];

	strftime(bufferH, 10, "%H", now);
	hours = strtoumax(bufferH,NULL,10);
	PM = hours > 11 ? 1 : 0;
	hours = hours > 12 ? hours - 12 : hours;
	strftime(bufferM, 10, "%M", now);
	minutes = strtoumax(bufferM,NULL,10);

	if (minutes != oldMinutes)
	{
		clock[0] = hours/10;
		clock[1] = hours - 10*clock[0];
		clock[2] = minutes/10;
		clock[3] = minutes - 10*clock[2];

		nju6676ClearScreen(BLUE);
		nju6676SelectFont(Verdana24, nju6676ReadFontData, WHITE);
		nju6676GotoXY(7,20);
		if (clock[0])
			nju6676PrintNumber(clock[0]);
		nju6676GotoXY(23,20);
		nju6676PrintNumber(clock[1]);
		nju6676GotoXY(46,20);
		nju6676Puts(":");
		nju6676GotoXY(58,20);
		nju6676PrintNumber(clock[2]);
		nju6676GotoXY(78,20);
		nju6676PrintNumber(clock[3]);
		nju6676SelectFont(System5x7, nju6676ReadFontData, WHITE);
		if (PM)
		{
			nju6676GotoXY(107,33);
			nju6676Puts("PM");
		} else {
			nju6676GotoXY(107,23);
			nju6676Puts("AM");
		}

		strftime(text, sizeof(text)-1, "%m/%d/%Y", now);
		nju6676GotoXY((SCREEN_WIDTH-(strlen(text)*5+strlen(text)-1))/2,56);
		nju6676Puts(text);

		nju6676Display();
	}
	return minutes;
}

void update_current_song( char *station, char *name, char *artist, char *title)
{
	nju6676FillRect(0,0,128,53,BLUE);

	song_metadata_clear(&current);
	current.Artist_strlen = song_metadata_copy(current.Artist, sizeof(current.Artist), artist);
	current.Title_strlen = song_metadata_copy(current.Title, sizeof(current.Title), title);
	current.Station_strlen = song_metadata_copy(current.Station, sizeof(current.Station), station);
	current.Album_strlen = song_metadata_copy(current.Album, sizeof(current.Album), name);
	song_metadata_prepare_display(&current);
}

void static_text( char *msg, int len, int Y)
{
	int X = (SCREEN_WIDTH-(len*5+len-1))/2;
	nju6676GotoXY(X,Y);
	nju6676Puts(msg);
}

void scroll_text( char *msg, int len, int counter, int Y)
{
	unsigned int k;
	char msgtext[22];
	for (k=0;k<21;k++)
		msgtext[k] = msg[(k+counter) % len];
	msgtext[21] = '\0';
	nju6676GotoXY(1,Y);
	nju6676Puts(msgtext);
}

void retr_msg(void)
{
	char *msg;
	// wait for the next refresh so display a message that we understood the input
	nju6676FillRect(0,0,128,53,BLUE);
	nju6676FillRect(120,56,8,8,BLUE);
	msg = "Retrieving info...\n";
	nju6676GotoXY((SCREEN_WIDTH-(strlen(msg)-1)*5-strlen(msg)-2)/2,8);
	nju6676Puts(msg);
	nju6676Display();
}

void retr_station_msg(void)
{
	char *msg;
	nju6676FillRect(0,0,128,53,BLUE);
	nju6676FillRect(120,56,8,8,BLUE);
	msg = "Retrieving station...\n";
	nju6676GotoXY((SCREEN_WIDTH-(strlen(msg)-1)*5-strlen(msg)-2)/2,8);
	nju6676Puts(msg);
	nju6676Display();
}

pid_t proc_find(const char* name) 
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                if (!strcmp(first, name)) {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }

    }

    closedir(dir);
    return -1;
}

int parse_nowplaying(char *fname) 
{
	FILE *fp, *fpStationList;
	char tempStationList[512];
	int line_num = 0;
	int station_found = 0;

	if((fp = fopen(fname, "r")) == NULL) {
		return -1;
	}
	if (song_metadata_read_pandora(fp, &current) != 0) {
		fclose(fp);
		return -1;
	}
	if (fclose(fp) != 0)
		return -1;

	if (!current.Station[0])
		return 0;

	fpStationList = fopen("/home/pi/.config/pianobar/stationlist", "r");
	if (!fpStationList)
		return -1;

	while (fgets(tempStationList, sizeof(tempStationList), fpStationList) != NULL) {
		if (strstr(tempStationList, current.Station) != NULL) {
			current.StationNum = line_num;
			station_found = 1;
			break;
		}
		line_num++;
	}

	fclose(fpStationList);
	if (!station_found)
		current.StationNum = 1;

	return 0;
}

void clear_current(void)
{
	song_metadata_clear(&current);
} 

int clear_playlists(void)
{
	DIR *theFolder = opendir("/var/lib/mpd/playlists");
	struct dirent *next_file;
	int result = 0;

	if (!theFolder) {
		perror("Unable to open MPD playlist directory");
		return -1;
	}

	while ((next_file = readdir(theFolder)) != NULL) {
	if (strcmp(next_file->d_name, ".") == 0 ||
	    strcmp(next_file->d_name, "..") == 0)
		continue;
	if (unlinkat(dirfd(theFolder), next_file->d_name, 0) != 0)
		result = -1;
	}

	if (closedir(theFolder) != 0)
	result = -1;
	return result;
}
