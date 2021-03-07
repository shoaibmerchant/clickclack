#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/input.h>


static int vibration = 0;
static int strength = 4000;
static int duration = 95; //in milliseconds
static int echo = 0;
static char *audiofile = NULL;

const int MIN_INTERVAL = 100; //minimal interval between two feedback actions in milliseconds
                              //if input comes in faster than this it will not trigger
							  //an action


extern int errno;

void playsound();
void vibrate();

void usage() {
	fprintf(stderr, "Usage: clickclack [options]\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -v         enable vibration\n");
	fprintf(stderr, " -s [int]   vibration strength\n");
	fprintf(stderr, " -d [int]   vibration duration in ms\n");
	fprintf(stderr, " -f [file]  audio file to play\n");
	fprintf(stderr, " -e         echo input to output\n");
}



int main(int argc, char* argv[])
{
	/* parse command line arguments */
	int fd, i, ret;
	for (i = 1; argv[i]; i++) {
		if (!strcmp(argv[i], "-f")) {
			audiofile = strdup(argv[++i]);
		} else if (!strcmp(argv[i], "-v")) {
			vibration = 1;
		} else if (!strcmp(argv[i], "-s")) {
			vibration = 1;
			strength = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-d")) {
			vibration = 1;
			duration = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-e")) {
			echo = 1;
		} else if (!strcmp(argv[i], "-h")) {
			usage(argv[0]);
		} else {
			fprintf(stderr, "Invalid argument: %s\n", argv[i]);
			exit(2);
		}
	}



	setvbuf(stdin, NULL, _IONBF, 0 ); //unbuffered input
	if (echo) setvbuf(stdout, NULL, _IONBF, 0 ); //unbuffered output

//	int flags = fcntl(fd, F_GETFL, 0);
//	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	char c;


	fd_set fds;
	int maxfd = STDIN_FILENO;
	clock_t now, last = 0;
	double interval;
	while (1) {
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		select(maxfd +1, &fds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if (feof(stdin)) break;
			c = getchar();
			if (c) {
				now = clock();
				interval = (double) (now - last) / CLOCKS_PER_SEC;
				last = now;
				if (interval > (double) MIN_INTERVAL * 1000) {
					if (vibration) vibrate();
					if (audiofile) playsound();
				}
				if (echo) putchar(c);
			}
		}
	}


	if (audiofile != NULL)
		free(audiofile);

	return 0;
}

void vibrate() {
	int fd, ret;
	struct pollfd pfds[1];
	int effects;

	fd = open("/dev/input/by-path/platform-vibrator-event", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Error reading opening event device /dev/input/by-path/platform-vibrator-event\n");
		return;
	}

	if (ioctl(fd, EVIOCGEFFECTS, &effects) < 0) {
		fprintf(stderr, "EVIOCGEFFECTS failed\n");
		close(fd);
		return;
	}

	struct ff_effect e = {
					.type = FF_RUMBLE,
					.id = -1,
					.u.rumble = { .strong_magnitude = strength },
	};

	if (ioctl(fd, EVIOCSFF, &e) < 0) {
		fprintf(stderr, "EVIOCSFF failed\n");
		close(fd);
		return;
	}

	struct input_event play = { .type = EV_FF, .code = e.id, .value = 3 };
	if (write(fd, &play, sizeof play) < 0) {
		fprintf(stderr, "write failed\n");
		close(fd);
		return;
	}

	usleep(duration * 1000);

	if (ioctl(fd, EVIOCRMFF, e.id) < 0) {
		fprintf(stderr, "EVIOCRMFF failed\n");
		close(fd);
		return;
	}

	close(fd);
}

void playsound() {
}
