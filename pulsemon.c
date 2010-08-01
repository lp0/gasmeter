#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pulsemon.h"
#include "pulseq.h"

char *device;
char *mqueue;
int fd;
mqd_t q;

static void setup(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <device> <mqueue>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	device = argv[1];
	mqueue = argv[2];
}

static void init(void) {
	struct mq_attr q_attr = {
		.mq_flags = 0,
		.mq_maxmsg = 4096,
		.mq_msgsize = sizeof(pulse_t)
	};
#if SERIO_OUT != 0
	int state;
#endif

	fd = open(device, O_RDONLY|O_NONBLOCK);
	cerror(device, fd < 0);

#if SERIO_OUT != 0
	cerror("Failed to get serial IO status", ioctl(fd, TIOCMGET, &state) != 0);
	state |= SERIO_OUT;
	cerror("Failed to set serial IO status", ioctl(fd, TIOCMSET, &state) != 0);
#endif

	q = mq_open(mqueue, O_WRONLY|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &q_attr);
	cerror(mqueue, q < 0);
}

static void daemon(void) {
#ifdef FORK
	pid_t pid = fork();
	cerror("Failed to become a daemon", pid < 0);
	if (pid)
		exit(EXIT_SUCCESS);
	close(0);
	close(1);
	close(2);
	setsid();
#endif
}

static void report(bool on) {
	pulse_t pulse;

	gettimeofday(&pulse.tv, NULL);
	pulse.on = on;

	_printf("%lu.%06u: %d\n", (unsigned long int)pulse.tv.tv_sec, (unsigned int)pulse.tv.tv_usec, pulse.on);
	mq_send(q, (const char *)&pulse, sizeof(pulse), 0);
}

static void check(void) {
	static int last = SERIO_IN;
	int state;

	cerror("Failed to get serial IO status", ioctl(fd, TIOCMGET, &state) != 0);
	state &= SERIO_IN;

	if (last != state)
		report(state != 0);

	last = state;
}

static bool wait(void) {
	bool ok = ioctl(fd, TIOCMIWAIT, SERIO_IN) == 0;
	if (!ok)
		perror("Failed to wait for serial IO status");
	return ok;
}

static void loop(void) {
	do {
		check();
	} while (wait());
}

static void cleanup(void) {
	cerror(device, close(fd));
	cerror(mqueue, mq_close(q));
}

int main(int argc, char *argv[]) {
	setup(argc, argv);
	init();
	daemon();
	loop();
	cleanup();
	exit(EXIT_FAILURE);
}
