prefix=/usr/local
ODIR = .
IDIR = .

CC = gcc
CCFLAGS = -Wall -O3 -D DEBUG
LIBS = -lrt -lmpd 

_DEPS = nju6676.h bcm2835.h PiRadio.h utilities.h mpd_playlists.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = PiRadio.o bcm2835.o nju6676.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

			 
PiRadio: $(OBJ)
	$(CC)  -o $@ $^ $(CCFLAGS) $(LIBS)

.PHONY: clean

clean:
		rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ 

install: 
	install -m 0755 PiRadio $(prefix)/bin

.PHONY: install
