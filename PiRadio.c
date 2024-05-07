#include "PiRadio.h"

#define TIME_OUT 5  // One of the system's FSM transitions

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
mpd_Song *song;
Song current;
unsigned int updated_song = 0;
int counter = 0;
int station;
MpdObj *obj = NULL;
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

char system_msg[200];
time_t oldTime;
int ch;
struct stat nowplayingStat;
int num_stations = 0;
FILE *fp;

int mpd_playlist_len = 1;
char *MPDplaylist[200];

int paused = 0;
int show_pause = 0;

int main(){

	int key;

	hostname = getenv("MPD_HOST");
	password = getenv("MPD_PASSWORD");
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
		delay(100);

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
					station = station >= (num_stations-1) ? 0 : station + 1;
					sprintf( system_msg, "echo 's%d' > /home/pi/.config/pianobar/ctl", station);
					system(system_msg);
					// wait for the next refresh
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_TUNE_DN:
					// previous station
					station = station < 1 ? num_stations-1 : station - 1;
					sprintf( system_msg, "echo 's%d' > /home/pi/.config/pianobar/ctl", station);
					system(system_msg);
					// wait for the next refresh
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_SEEK_UP:
					// love song
					system("echo '+' > /home/pi/.config/pianobar/ctl");

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
					system("echo '-' > /home/pi/.config/pianobar/ctl");
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_HD_UP:
					// next song
					system("echo 'n' > /home/pi/.config/pianobar/ctl");
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_HD_DN:
					// tired song
					system("echo 't' > /home/pi/.config/pianobar/ctl");
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
					break;
				case KEYPAD_FREQUENCY:
					// pause/un-pause the currently playing song
					system("echo 'p' > /home/pi/.config/pianobar/ctl");
					paused = !paused;
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
					station++;
					station = station % mpd_playlist_len;
					change_station( obj, MPDplaylist[station]);
					current.Title[0] = 0; // force a redraw of the screen
					break;
				case KEYPAD_TUNE_DN:
					station--;
					station = station % mpd_playlist_len;
					change_station( obj, MPDplaylist[station]);
					current.Title[0] = 0; // force a redraw of the screen
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
					station--;
					station = station >= (num_stations-1) ? 0 : station;
					sprintf( system_msg, "echo 's%d' > /home/pi/.config/pianobar/ctl", station);
					system(system_msg);
					// wait for the next refresh so display a message that we understood the input
					retr_msg();
					updated_song = 0;
				}

			}
#ifdef DEBUG
			printf("State: PRESET_SELECT\n");
#endif
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
				system("sudo service shairport stop >/dev/null 2>&1");
				break;
			case pandoraService:
				system("echo 'q' > /home/pi/.config/pianobar/ctl");
				paused = 0;
				break;
			case webRadioService:
				if(!hostname)
					hostname = "localhost";
				obj = mpd_new(hostname, iport, password);
				if(!mpd_connect(obj))
					mpd_player_stop(obj);
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
		system("sudo service shairport restart >/dev/null 2>&1");
		runningService = airPlayService;
	}
}

void funcWebRadio()
{
	int w, h, k, X;
	FILE *fp, *fplist;
	char temp[512];
	char plistname[512];
	char plistlink[512];


	if ( runningService != webRadioService )
	{

		stopServices( runningService );

		// clear out the old playlists 
		//system("rm /var/lib/mpd/playlists/*.m3u");
#ifdef DEBUG
	printf("Preparing to clear the directory...\n");
#endif
		clear_playlists();
#ifdef DEBUG
	printf("Now in the funcWebRadio...\n");
#endif
		mpd_playlist_len = 0;

		// create the new playlists from text file
		if((fp = fopen("/home/pi/webradioplaylist.txt", "r")) == NULL) {
			return;
		}

		size_t ln;
		while(fgets(temp, 512, fp) != NULL) {
			ln = strlen(temp) - 1;
			if (temp[ln] == '\n')
				temp[ln] = '\0';
			sprintf(plistname,"/var/lib/mpd/playlists/%s",temp);

			// copy the name of the station to an array
			temp[ln-4] = '\0';  // needed to strip file extension
			MPDplaylist[mpd_playlist_len] = malloc(ln-3); // allocate storage
			strcpy(MPDplaylist[mpd_playlist_len],temp); 

			// open new filename and store link
			fgets(plistlink, 512, fp);
			if((fplist = fopen(plistname,"w")) == NULL) {
				return;
			} 
			else {
				fputs(plistlink, fplist);
				fclose(fplist);
			}
			mpd_playlist_len++;
		}

		if(fp)
			fclose(fp);

		if(!hostname)
			hostname = "localhost";
#ifdef DEBUG
		printf("%s\n", hostname);
#endif
		obj = mpd_new(hostname, iport, password);

		// if the service has stopped or there is an error, restart it
		if(mpd_connect(obj)) {
			system("mkdir /var/log/mpd");
			system("touch /var/log/mpd/mpd.log");
			system("sudo service mpd restart >/dev/null 2>&1");
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
		change_station( obj, MPDplaylist[station]);

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
			if(!mpd_connect(obj))
			{
				// connection established, 
				mpd_status_update(obj);
				// retrieve the current information
				song = mpd_playlist_get_current_song(obj);

				// check for valid info, otherwise we make up our own
				if (song && song->title)
				{
					if (strncmp(song->title,current.Title,21) != 0)
					{
						update_current_song( (char *)MPDplaylist[station], song->name, song->artist, song->title);
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
		if (pid != -1) system("echo 'q' > /home/pi/.config/pianobar/ctl");

		// now it is not running, so clear out the current song info
		system("echo '' > /home/pi/.config/pianobar/nowplaying");
		if (stat("/home/pi/.config/pianobar/nowplaying", &nowplayingStat) < 0)
			return 1;
		oldTime = nowplayingStat.st_mtime;
		// start pianobar
		system("su - pi -c \"nohup pianobar >/dev/null 2>&1&\"");
		bcm2835_delay(5000);
		// wait for the next refresh
		retr_msg();
		updated_song = 0;

		ch = 0; station = 0; num_stations = 0;
		fp = fopen("/home/pi/.config/pianobar/stationlist","r");
		while(!feof(fp))
		{
			ch = fgetc(fp);
			if(ch == '\n')
			{
				num_stations++;
			}
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
			station = current.StationNum;
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
		delay(75);  // debounce the switch
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
	int col, row;
	unsigned char c, r;
	c = scan_cols();
	if (c != 0x3F) {
		r = scan_rows();
		while (scan_cols() == c) ;

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

		keyscan_colsetup();
		return keys[row][col];
	}

	keyscan_colsetup();
	return KEYPAD_NONE;
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

int change_station( MpdObj *obj, const char *station)
{
	if(!mpd_connect(obj))
	{
		mpd_playlist_clear(obj);
		mpd_playlist_queue_load(obj, station);
		mpd_playlist_queue_commit(obj);
		mpd_player_play(obj);
	} else {
		printf("ERROR : Cannot change station to: %s\n", station);
		return 1;
	}

	return 0;

}

void update_current_song( char *station, char *name, char *artist, char *title)
{
	int k;
	nju6676FillRect(0,0,128,53,BLUE);

	current.Artist[0] = 0;
	current.Title[0] = 0;
	current.Album[0] = 0;
	current.Station[0] = 0;
	current.Artist_strlen = 0;
	current.Title_strlen = 0;
	current.Album_strlen = 0;
	current.Station_strlen = 0;

	if(artist)
	{
		for (k = 0; k<strlen(artist); k++)
			current.Artist[k] = artist[k];
		current.Artist[strlen(artist)] = 0;
	}
	if(title)
	{
		for (k = 0; k<strlen(title); k++)
			current.Title[k] = title[k];
		current.Title[strlen(title)] = 0;
	}
	if(station)
	{
		for (k = 0; k<strlen(station); k++)
			current.Station[k] = station[k];
		current.Station[strlen(station)] = 0;
	}
	if(name)
	{
		for (k = 0; k<strlen(name); k++)
			current.Album[k] = name[k];
		current.Album[strlen(name)] = 0;
	}

	current.Artist_strlen = strlen(current.Artist);
	current.Title_strlen = strlen(current.Title);
	current.Album_strlen = strlen(current.Album);
	current.Station_strlen = strlen(current.Station);

	if (current.Artist_strlen > 21)
	{
		strcat(current.Artist, "    ");
		current.Artist_strlen+=4;
	}
	if (current.Title_strlen > 21)
	{
		strcat(current.Title, "    ");
		current.Title_strlen+=4;
	}
	if (current.Album_strlen > 21)
	{
		strcat(current.Album, "    ");
		current.Album_strlen+=4;
	}
	if (current.Station_strlen > 21)
	{
		strcat(current.Station, "    ");
		current.Station_strlen+=4;
	}
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
	char temp[512], tempStationList[512];
	int k, count = 0;
	int line_num = 0;
	int find_result = 0;

// 	FILE *fp;
// 	char temp[512];
// 	int k, count = 0;

	if((fp = fopen(fname, "r")) == NULL) {
		return -1;
	}

	while(fgets(temp, 512, fp) != NULL) {
		count++;
		switch (count)
		{
			case 1 :
				for (k = 0; k<strlen(temp)-2; k++)
					current.Artist[k] = temp[k];
				current.Artist[strlen(temp)] = 0;
				break;
			case 2 :
				for (k = 0; k<strlen(temp)-2; k++)
					current.Title[k] = temp[k];
				current.Title[strlen(temp)] = 0;
				break;
			case 3 :
				for (k = 0; k<strlen(temp)-2; k++)
					current.Station[k] = temp[k];
				current.Station[strlen(temp)] = 0;
				if((fpStationList = fopen("/home/pi/.config/pianobar/stationlist", "r")) == NULL) {
					return(-1);
				}
				while(fgets(tempStationList, 512, fpStationList) != NULL) {
					if((strstr(tempStationList, current.Station)) != NULL) {
						/*printf("A match found on line: %d\n", line_num);*/
						/*printf("\n%s\n", temp);*/
						current.StationNum = line_num;
						find_result++;
						break;
					}
					line_num++;
				}

				if(find_result == 0) {
					/*printf("\nSorry, couldn't find a match.\n");*/
					current.StationNum = 1;
				}

				//Close the file if still open.
				if(fpStationList) {
					fclose(fpStationList);
				}
				break;
// 			case 3 :
// 				for (k = 0; k<strlen(temp)-2; k++)
// 					current.Station[k] = temp[k];
// 				current.Station[strlen(temp)] = 0;
// 				break;
			case 4 :
				current.Rating = atoi(temp);
				break;
			case 5 :
				for (k = 0; k<strlen(temp)-2; k++)
					current.CoverArt[k] = temp[k];
				current.CoverArt[strlen(temp)] = 0;
				break;
			case 6 :
				for (k = 0; k<strlen(temp)-1; k++)
					current.Album[k] = temp[k];
				current.Album[strlen(temp)] = 0;
				break;
		}
	}

	//Close the file if still open.
	if(fp) fclose(fp);

	current.Artist_strlen = strlen(current.Artist);
	current.Title_strlen = strlen(current.Title);
	current.Album_strlen = strlen(current.Album);
	current.Station_strlen = strlen(current.Station);

	if (current.Artist_strlen > 21)
	{
		strcat(current.Artist, "    ");
		current.Artist_strlen+=4;
	}
	if (current.Title_strlen > 21)
	{
		strcat(current.Title, "    ");
		current.Title_strlen+=4;
	}
	if (current.Album_strlen > 21)
	{
		strcat(current.Album, "    ");
		current.Album_strlen+=4;
	}
	if (current.Station_strlen > 21)
	{
		strcat(current.Station, "    ");
		current.Station_strlen+=4;
	}

	return 0;
};

void clear_current(void)
{
	char *msg;
	msg = current.Artist;
	memset(msg, 0, sizeof(char)*200);
	msg = current.Title;
	memset(msg, 0, sizeof(char)*200);
	msg = current.Album;
	memset(msg, 0, sizeof(char)*200);
	msg = current.Station;
	memset(msg, 0, sizeof(char)*200);
} 

void clear_playlists()
{
    // These are data types defined in the "dirent" header
    DIR *theFolder = opendir("/var/lib/mpd/playlists");
    struct dirent *next_file;
    char filepath[256];

    while ( (next_file = readdir(theFolder)) != NULL )
    {
	if (0==strcmp(next_file->d_name, ".") || 0==strcmp(next_file->d_name, "..")) { continue; }
        // build the path for each file in the folder
        sprintf(filepath, "%s/%s", "/var/lib/mpd/playlists", next_file->d_name);
        remove(filepath);
    }
    closedir(theFolder);
}
