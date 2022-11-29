
# if using PNG files:
#PNGCONF=/usr/bin/libpng-config
ifdef PNGCONF
CFLAGS=-DPNG $(shell $(PNGCONF) --cflags)
LDFLAGS=$(shell $(PNGCONF) --ldflags)
OBJ=src/pngwriter.c
else

endif

all:    main parallel

main:	src/main.c $(OBJ)
	cc -o main $(CFLAGS) $< $(OBJ) $(LDFLAGS)

parallel: src/parallel.c $(OBJ)
	mpicc -o parallel $(CFLAGS) $< $(OBJ) $(LDFLAGS)

clean:
	rm -f main parallel *.o
