
# if using PNG files:
#PNGCONF=/usr/bin/libpng-config
ifdef PNGCONF
CFLAGS=-DPNG $(shell $(PNGCONF) --cflags)
LDFLAGS=$(shell $(PNGCONF) --ldflags)
OBJ=src/pngwriter.c
else

endif

all:    main

main:	src/main.c $(OBJ)
	mpicc -o main $(CFLAGS) $< $(OBJ) $(LDFLAGS)


clean:
	rm -f main *.o
