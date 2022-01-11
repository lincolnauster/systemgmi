CC=gcc
CFLAGS=-D_POSIX_C_SOURCE=200809L -std=c17 -Wall -Werror -pedantic -g

systemgmi: main.o log.o com.o com_util.o machine.o router.o systemd.o
	$(CC) $(CFLAGS) main.o log.o com.o com_util.o machine.o router.o systemd.o \
	      -o systemgmi \
	      -lsystemd -lssl -pthread
main.o: main.c log.o com.o machine.o router.o
	$(CC) $(CFLAGS) -c main.c -o main.o
log.o: log/log.c log/log.h
	$(CC) $(CFLAGS) -c log/log.c -o log.o
com.o: com/com.c com/com.h com/types.h
	$(CC) $(CFLAGS) -c com/com.c -o com.o
com_util.o: com/util.c com/util.h com/types.h
	$(CC) $(CFLAGS) -c com/util.c -o com_util.o
machine.o: machine/machine.c machine/machine.h
	$(CC) $(CFLAGS) -c machine/machine.c -o machine.o
router.o: router/router.c router/router.h
	$(CC) $(CFLAGS) -c router/router.c -o router.o
systemd.o: systemd/systemd.h systemd/systemd.c
	$(CC) $(CFLAGS) -c systemd/systemd.c -o systemd.o

clean:
	rm systemgmi
	rm *.o
