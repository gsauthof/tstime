CFLAGS = -Wall -g -std=c99 -D_XOPEN_SOURCE

TEMP = tstime.o taskstat.o tools.o tsmon.o tstime tsmon

HG = hg

.PHONY: all
all: tstime tsmon

tstime: tstime.o taskstat.o tools.o version.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tsmon.o tsmon.c tstime.o taskstat.o tstime.c taskstat.c: taskstat.h
tsmon.o tsmon.c tstime.o tstime.c : tools.h

tsmon: tsmon.o taskstat.o tools.o version.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)


.PHONY: clean
clean:
	rm -f $(TEMP)


version.txt: .hg/dirstate
	$(HG) log -l1 --template '$$Id: {date|isodate} {node} $$' > $@

version.c: version.txt
	printf "#include \"version.h\"\nchar const rcsid[] = \"`cat $<`\";\n\n" > $@

