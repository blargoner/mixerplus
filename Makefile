CC=i686-w64-mingw32-gcc
RC=i686-w64-mingw32-windres
CFLAGS=-I.
LDFLAGS=-lwinmm
DEPS = MixerPlus.h resource.h
OBJ = Error.o MixerPlusCFader.o MixerPlusCtrl.o MixerPlus.o MixerPlusOptions.o MixerPlusWnd.o
RESFILES = MixerPlus.res

%.res: %.rc $(DEPS)
	$(RC) $< -O coff $@

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<

MixerPlus: $(OBJ) $(RESFILES)
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ) $(RESFILES)