
# if using PNG files:
#PNGCONF=/usr/bin/libpng-config
ifdef PNGCONF
CFLAGS=-DPNG $(shell $(PNGCONF) --cflags)
LDFLAGS=$(shell $(PNGCONF) --ldflags)
OBJ=src/pngwriter.c
else

endif

all:    main parallel master-worker master-worker-async

main:	src/main.c $(OBJ)
	cc -o main $(CFLAGS) $< $(OBJ) $(LDFLAGS)

parallel: src/parallel.c $(OBJ)
	mpicc -o parallel $(CFLAGS) $< $(OBJ) $(LDFLAGS)

master-worker: src/master-worker.c $(OBJ)
	mpicc -o master-worker $(CFLAGS) $< $(OBJ) $(LDFLAGS)

master-worker-async: src/master-worker-async.c $(OBJ)
	mpicc -o master-worker-async $(CFLAGS) $< $(OBJ) $(LDFLAGS)

clean:
	rm -f main parallel master-worker master-worker-async *.o
