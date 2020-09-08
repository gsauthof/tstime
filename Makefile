CFLAGS = -Wall -g -std=c99 -D_XOPEN_SOURCE

TEMP = tstime.o taskstat.o tools.o tsmon.o tstime tsmon

HG = hg

.PHONY: all
all: tstime tsmon tslog

tstime: tstime.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tsmon.o tsmon.c tslog.o tslog.c tstime.o taskstat.o tstime.c taskstat.c: taskstat.h
tsmon.o tsmon.c tslog.o tslog.c tstime.o tstime.c : tools.h

tsmon: tsmon.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tslog: tslog.o taskstat.o tools.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)


.PHONY: clean
clean:
	rm -f $(TEMP)
