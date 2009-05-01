

tstime: tstime.o taskstat.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

tstime.o taskstat.o tstime.c taskstat.c: taskstat.h
