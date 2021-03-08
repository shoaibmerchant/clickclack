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
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>


static int vibration = 0;
static int strength = 4000;
static int duration = 95; //in milliseconds
static int echo = 0;
static char *audiofile = NULL;
static int audioduration = 95; //in milliseconds
static int debug = 0;

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
	fprintf(stderr, " -t [int]   audio file duration in ms\n");
	fprintf(stderr, " -D         Debug mode\n");
	fprintf(stderr, " -e         echo input to output\n");
}

SDL_AudioSpec wavspec;
uint32_t wavlength;
uint8_t *wavbuffer;


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
		} else if (!strcmp(argv[i], "-t")) {
			duration = 100;
		} else if (!strcmp(argv[i], "-e")) {
			echo = 1;
		} else if (!strcmp(argv[i], "-h")) {
			usage(argv[0]);
		} else if (!strcmp(argv[i], "-D")) {
			debug = 1;
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

	char c;


	fd_set fds;
	int maxfd = STDIN_FILENO;
	struct timeval now, last;
	double interval;
	while (1) {
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		select(maxfd +1, &fds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if (feof(stdin)) break;
			c = getchar();
			if (c > 0) {
				gettimeofday(&now, NULL);
				interval = (double)(now.tv_usec - last.tv_usec) / 1000000 + (double)(now.tv_sec - last.tv_sec);
				last = now;
				if (debug) fprintf(stderr, "interval=%f, char=%d\n", interval, c);
				if (echo) putchar(c);
				if ((interval < 0) || (interval > (double) MIN_INTERVAL / 1000)) {
					if (vibration) vibrate();
					if (audiofile != NULL) playsound();
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
	fprintf(stderr, "Playing audio...\n");
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
