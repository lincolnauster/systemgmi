CC?=gcc
CFLAGS?=-D_POSIX_C_SOURCE=200809L -std=c17 -Wall -Werror -pedantic -g

systemgmi: main.o cli.o log/log.o com/com.o com/util.o machine/machine.o \
           router/router.o systemd/systemd.o threading/threading.o \
           threading/queue.o
	$(CC) $(CFLAGS) main.o cli.o log/log.o com/com.o com/util.o \
	      machine/machine.o router/router.o systemd/systemd.o \
	      threading/threading.o threading/queue.o \
	      -o systemgmi \
	      -lsystemd -lssl -pthread \
	      `pkg-config --cflags --libs icu-uc`

main.o: main.c
cli.o: cli.h cli.c
log/log.o: log/log.c log/log.h
com/com.o: com/com.c com/com.h com/types.h
com_util/com_util.o: com/util.c com/util.h com/types.h
machine/machine.o: machine/machine.c machine/machine.h
router/router.o: router/router.c router/router.h
systemd/systemd.o: systemd/systemd.h systemd/systemd.c cli.o
threading/threading.o: threading/threading.h threading/threading.c
threading/queue.o: threading/queue.h threading/queue.c

clean:
	rm -f systemgmi
	rm -f *.o
	rm -f */*.o
