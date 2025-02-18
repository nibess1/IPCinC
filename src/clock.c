#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void append_log(
	const pid_t pid,
	const unsigned int step_delay,
	const unsigned int i,
	const unsigned int total_count
) {
	printf("[%4d]\t%d/%d: ", pid, i, total_count);
	if (i == total_count) {
		printf("Done.\n");
	} else {
		printf("Sleep for %d seconds.\n", step_delay);
	}
}

enum {
	ARG_DELAY = 1,
	ARGS_COUNT = ARG_DELAY + 1
};

int main(int argc, char *argv[]) {
	if (argc != ARGS_COUNT) {
		fprintf(stderr, "The program must be run with one argument.\n");
		return EXIT_FAILURE;
	}
	const int step_delay_int = atoi(argv[ARG_DELAY]);
	if (step_delay_int <= 0) {
		fprintf(stderr, "The first argument must be a positive integer.\n");
		return EXIT_FAILURE;
	}
	const unsigned int step_delay = (unsigned int)step_delay_int;
	
	const pid_t pid = getpid();
	unsigned int i = 0;
	const unsigned int step_count = 5;
	while (i < step_count) {
		append_log(pid, step_delay, i++, step_count);
		sleep(step_delay /* seconds */);
	}
	append_log(pid, step_delay, i, step_count);
	return EXIT_SUCCESS;	
}
