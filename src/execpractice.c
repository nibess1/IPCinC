#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void){
    pid_t pid= fork();
    if(pid< 0){
        return EXIT_FAILURE;
    } else if (pid == 0){
        char* args[3] = {"clock", "2", NULL};
        execvp("./clock", args);
    } else {
        printf("parent");
        return 0;
    }
   
}