CC?=gcc
CFLAGS?=-D_POSIX_C_SOURCE=200809L -std=c17 -Wall -Werror -pedantic -g

systemgmi: main.o log.o com.o com_util.o machine.o router.o systemd.o \
           threading.o threading_queue.o cli.o
	$(CC) $(CFLAGS) main.o cli.o log.o com.o com_util.o machine.o router.o \
	      systemd.o threading.o threading_queue.o \
	      -o systemgmi \
	      -lsystemd -lssl -pthread \
	      `pkg-config --cflags --libs icu-uc`
main.o: main.c log.o com.o machine.o router.o
	$(CC) $(CFLAGS) -c main.c -o main.o
log.o: log/log.c log/log.h
	$(CC) $(CFLAGS) -c log/log.c -o log.o
cli.o: cli.h cli.c
	$(CC) $(CFLAGS) -c cli.c -o cli.o
com.o: com/com.c com/com.h com/types.h
	$(CC) $(CFLAGS) -c com/com.c -o com.o
com_util.o: com/util.c com/util.h com/types.h
	$(CC) $(CFLAGS) -c com/util.c -o com_util.o
machine.o: machine/machine.c machine/machine.h
	$(CC) $(CFLAGS) -c machine/machine.c -o machine.o
router.o: router/router.c router/router.h
	$(CC) $(CFLAGS) -c router/router.c -o router.o
systemd.o: systemd/systemd.h systemd/systemd.c cli.o
	$(CC) $(CFLAGS) -c systemd/systemd.c -o systemd.o
threading.o: threading/threading.h threading/threading.c
	$(CC) $(CFLAGS) -c threading/threading.c -o threading.o
threading_queue.o: threading/queue.h threading/queue.c
	$(CC) $(CFLAGS) -c threading/queue.c -o threading_queue.o

clean:
	rm systemgmi
	rm *.o
