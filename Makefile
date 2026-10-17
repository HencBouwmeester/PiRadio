prefix ?= /usr/local
SYSTEMD_UNIT_DIR ?= /etc/systemd/system
DESTDIR ?=
ODIR = .
IDIR = .

CC = gcc
CCFLAGS = -Wall -O3 -D DEBUG
LIBS = -lrt -lmpd 

_DEPS = nju6676.h bcm2835.h PiRadio.h station_index.h radio_playlist.h song_metadata.h radio_services.h radio_controller.h utilities.h mpd_playlists.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = PiRadio.o bcm2835.o nju6676.o station_index.o radio_playlist.o song_metadata.o radio_services.o radio_mpd.o radio_controller.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

			 
PiRadio: $(OBJ)
	$(CC)  -o $@ $^ $(CCFLAGS) $(LIBS)

.PHONY: clean

clean:
		rm -f PiRadio $(ODIR)/*.o test_station_index test_radio_playlist test_song_metadata test_radio_services test_radio_controller *~ core $(IDIR)/*~

install: PiRadio
	install -d $(DESTDIR)$(prefix)/bin $(DESTDIR)$(SYSTEMD_UNIT_DIR)
	install -m 0755 PiRadio $(DESTDIR)$(prefix)/bin/PiRadio
	install -m 0644 PiRadio.service pianobar.service $(DESTDIR)$(SYSTEMD_UNIT_DIR)/
	@if test -z "$(DESTDIR)"; then \
		systemctl daemon-reload && \
		systemctl enable PiRadio.service && \
		systemctl start PiRadio.service; \
	fi

.PHONY: install

.PHONY: test
test: test_station_index test_radio_playlist test_song_metadata test_radio_services test_radio_controller
	./test_station_index
	./test_radio_playlist
	./test_song_metadata
	./test_radio_services
	./test_radio_controller

test_station_index: test_station_index.c station_index.c station_index.h
	$(CC) -o $@ test_station_index.c station_index.c -Wall -O2

test_radio_playlist: test_radio_playlist.c radio_playlist.c radio_playlist.h
	$(CC) -o $@ test_radio_playlist.c radio_playlist.c -Wall -O2

test_song_metadata: test_song_metadata.c song_metadata.c song_metadata.h
	$(CC) -o $@ test_song_metadata.c song_metadata.c -Wall -O2

test_radio_services: test_radio_services.c radio_services.c radio_mpd.c radio_services.h
	$(CC) -o $@ test_radio_services.c radio_services.c radio_mpd.c -Wall -O2 $(LIBS)

test_radio_controller: test_radio_controller.c radio_controller.c radio_controller.h station_index.c station_index.h
	$(CC) -o $@ test_radio_controller.c radio_controller.c station_index.c -Wall -O2
