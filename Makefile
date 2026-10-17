prefix=/usr/local
ODIR = .
IDIR = .

CC = gcc
CCFLAGS = -Wall -O3 -D DEBUG
LIBS = -lrt -lmpd 

_DEPS = nju6676.h bcm2835.h PiRadio.h station_index.h radio_playlist.h utilities.h mpd_playlists.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = PiRadio.o bcm2835.o nju6676.o station_index.o radio_playlist.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

			 
PiRadio: $(OBJ)
	$(CC)  -o $@ $^ $(CCFLAGS) $(LIBS)

.PHONY: clean

clean:
		rm -f PiRadio $(ODIR)/*.o test_station_index test_radio_playlist *~ core $(IDIR)/*~

install: 
	install -m 0755 PiRadio $(prefix)/bin

.PHONY: install

.PHONY: test
test: test_station_index test_radio_playlist
	./test_station_index
	./test_radio_playlist

test_station_index: test_station_index.c station_index.c station_index.h
	$(CC) -o $@ test_station_index.c station_index.c -Wall -O2

test_radio_playlist: test_radio_playlist.c radio_playlist.c radio_playlist.h
	$(CC) -o $@ test_radio_playlist.c radio_playlist.c -Wall -O2
