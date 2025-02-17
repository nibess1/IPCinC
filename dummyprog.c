#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum {
	ARG_DELAY = 1,
	ARGS_COUNT = ARG_DELAY + 1
};

int main(int argc, char * argv[]){
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
    sleep(step_delay);
    printf("[%4d] Completed nothing in %d seconds\n", pid, step_delay_int);
}