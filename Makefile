CFLAGS = -Wall -g -std=c99 -D_XOPEN_SOURCE

tstime: tstime.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tsmon.o tsmon.c tstime.o taskstat.o tstime.c taskstat.c: taskstat.h
tsmon.o tsmon.c tstime.o tstime.c : tools.h

tsmon: tsmon.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)
