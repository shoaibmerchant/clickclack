#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linux/input.h>
#include <sys/ioctl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

static void playsound();
static void usage(char *program);
static void vibrate();

static char *vibra_event_dev = "/dev/input/by-path/platform-vibrator-event";
static int vibration = 0;
static int strength = 4000;
static int duration = 95; /* in milliseconds */
static int echo = 0;
static char *audiofile = NULL;
static int audioduration = 95; /* in milliseconds */
static int debug = 0;
static int utf8 = 1;
static int firstonly = 0;

/*
 * MIN_INTERVAL
 * The minimum interval between two feedback actions in milliseconds.
 * If the input comes in faster than this it will not trigger an action.
 */
const int MIN_INTERVAL = 100;

static SDL_AudioSpec wavspec;
static uint32_t wavlength;
static uint8_t *wavbuffer;

static void
playsound()
{
	if (debug) fprintf(stderr, "Playing audio...\n");
	SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(NULL, 0, &wavspec, NULL, 0);
	if (SDL_QueueAudio(device_id, wavbuffer, wavlength) != 0) {
		fprintf(stderr, "QueueAudio failed\n");
		SDL_CloseAudioDevice(device_id);
		return;
	}
	SDL_PauseAudioDevice(device_id, 0); //unpause aka play
	SDL_Delay(audioduration);
	SDL_CloseAudioDevice(device_id);
}

static void
usage(char *program)
{
	fprintf(stderr, "Usage: %s [options]\n", program);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -V         enable vibration\n");
	fprintf(stderr, " -E [file]  path to vibrator device event file\n");
	fprintf(stderr, " -s [int]   vibration strength\n");
	fprintf(stderr, " -d [int]   vibration duration in ms\n");
	fprintf(stderr, " -f [file]  audio file to play\n");
	fprintf(stderr, " -t [int]   audio file duration in ms\n");
	fprintf(stderr, " -D         Debug mode\n");
	fprintf(stderr, " -e         echo input to output\n");
	fprintf(stderr, " -a         ASCII-mode, disables utf-8 handling\n");
	fprintf(stderr, " -o         trigger vibration only on first character of the line\n");
}

static void
vibrate()
{
	int fd, ret;
	struct pollfd pfds[1];
	int effects;

	fd = open(vibra_event_dev, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "Error reading opening event device %s\n", vibra_event_dev);
		perror("open");
		return;
	}

	if (ioctl(fd, EVIOCGEFFECTS, &effects) < 0) {
		fprintf(stderr, "EVIOCGEFFECTS failed\n");
		perror("ioctl");
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
		perror("ioctl");
		close(fd);
		return;
	}

	struct input_event play = { .type = EV_FF, .code = e.id, .value = 3 };
	if (write(fd, &play, sizeof play) < 0) {
		perror("write");
		close(fd);
		return;
	}

	usleep(duration * 1000);

	if (ioctl(fd, EVIOCRMFF, e.id) < 0) {
		fprintf(stderr, "EVIOCRMFF failed\n");
		perror("ioctl");
		close(fd);
		return;
	}

	close(fd);
}

int
main(int argc, char* argv[])
{
	/* set vibrator device event file from env */
	char *tmp;
	if ((tmp = getenv("SXMO_VIBRATE_DEV")))
		vibra_event_dev = strdup(tmp);

	/* parse command line arguments */
	int fd, i, ret;
	for (i = 1; argv[i]; i++) {
		if (!strcmp(argv[i], "-f")) {
			audiofile = strdup(argv[++i]);
		} else if (!strcmp(argv[i], "-E")) {
			vibra_event_dev = strdup(argv[++i]);
		} else if (!strcmp(argv[i], "-V")) {
			vibration = 1;
		} else if (!strcmp(argv[i], "-s")) {
			vibration = 1;
			strength = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-d")) {
			vibration = 1;
			duration = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-t")) {
			duration = 100;
		} else if (!strcmp(argv[i], "-e")) {
			echo = 1;
		} else if (!strcmp(argv[i], "-h")) {
			usage(argv[0]);
			exit(0);
		} else if (!strcmp(argv[i], "-D")) {
			debug = 1;
		} else if (!strcmp(argv[i], "-a")) {
			utf8 = 0;
		} else if (!strcmp(argv[i], "-o")) {
			firstonly = 1;
		} else {
			fprintf(stderr, "Invalid argument: %s\n", argv[i]);
			exit(2);
		}
	}

	if (audiofile != NULL) {
		SDL_Init(SDL_INIT_AUDIO);
		if (SDL_LoadWAV(audiofile, &wavspec, &wavbuffer, &wavlength) == NULL) {
			fprintf(stderr, "Error loading audio file %s", audiofile);
			exit(2);
		}
	}


	setvbuf(stdin, NULL, _IONBF, 0 ); //unbuffered input
	if (echo) setvbuf(stdout, NULL, _IONBF, 0 ); //unbuffered output

//	int flags = fcntl(fd, F_GETFL, 0);
//	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	char c = '\n';


	fd_set fds;
	int maxfd = STDIN_FILENO;
	struct timeval now, last;
	double interval;
	int skip = 0;
	while (1) {
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		select(maxfd +1, &fds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if (feof(stdin)) break;
			if (firstonly && c!='\n' && skip==0) skip = 1;
			c = getchar();
			if (c != EOF) {
				if (skip == 0) {
					//simple utf-8 parser
					if (utf8) {
						if (c >> 7 == 0) {
							skip = 0;
							if (c < 0x21 && c != 0x0a && c != 0x20) {
								//do not emit on control characters (except newline, space, tab, and backspace)
								if (debug) fprintf(stderr, "skipping control char\n");
								if (echo) putchar(c);
								continue;
							}
						} else if (c >> 5 == -0b10) {
							skip = 1;
						} else if (c >> 4 == -0b110) {
							skip = 2;
						} else if (c >> 3 == -0b1110) {
							skip = 3;
						} else {
							//invalid utf-8
							skip = 0;
							if (debug) fprintf(stderr, "invalid utf-8 char=%d (first)\n", c);
							if (echo) putchar(c);
							continue;
						}
					}

					gettimeofday(&now, NULL);
					interval = (double)(now.tv_usec - last.tv_usec) / 1000000 + (double)(now.tv_sec - last.tv_sec);
					last = now;
					if (debug) fprintf(stderr, "interval=%f, char=%d (emitted)\n", interval, c);
					if (echo) putchar(c);
					if ((interval < 0) || (interval > (double) MIN_INTERVAL / 1000)) {
						if (vibration) vibrate();
						if (audiofile != NULL) playsound();
					}
				} else {
					if (c >> 6 != -0b10 && !firstonly) {
						//invalid utf-8, reset
						skip = 0;
						if (debug) fprintf(stderr, "invalid utf-8 char=%d (cont)\n", c);
						continue;
					}
					if (debug) fprintf(stderr, "char=%d skip=%d (not emitted)\n", c, skip);
					if (echo) putchar(c);
					skip--;
				}
			}
		}
	}


	if (audiofile != NULL) {
		free(audiofile);
		SDL_FreeWAV(wavbuffer);
	}

	return 0;
}
