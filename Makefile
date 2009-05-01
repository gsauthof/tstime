CFLAGS = -Wall -g -std=c99 -D_XOPEN_SOURCE

TEMP = tstime.o taskstat.o tools.o tsmon.o taskstat tsmon

tstime: tstime.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tsmon.o tsmon.c tstime.o taskstat.o tstime.c taskstat.c: taskstat.h
tsmon.o tsmon.c tstime.o tstime.c : tools.h

tsmon: tsmon.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)


clean:
	rm -f $(TEMP)
