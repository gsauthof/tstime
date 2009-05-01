

tstime: tstime.o taskstat.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tstime.c taskstat.c: taskstat.h
