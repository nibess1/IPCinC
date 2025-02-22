#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/******************************************************************************
 * Types
 ******************************************************************************/

typedef enum process_status {
    RUNNING = 0,
    READY = 1,
    STOPPED = 2,
    TERMINATED = 3,
    UNUSED = 4
} process_status;

typedef struct process_record {
    pid_t pid;
    int index;
    process_status status;
} process_record;

/******************************************************************************
 * Globals
 ******************************************************************************/

enum {
    MAX_PROCESSES = 99,
    MAX_RUNNING = 3,
    MAX_QUEUE = MAX_PROCESSES - MAX_RUNNING
};

process_record* process_records[MAX_PROCESSES] = { NULL };
process_record* running_processes[MAX_RUNNING] = { NULL };
process_record* process_queue[MAX_QUEUE] = { NULL };
int latest_running[MAX_RUNNING] = { -1 };

int proc_index = 0;
int add_index = 0;
int rem_index = 0;

/******************************************************************************
 * Initialising
 ******************************************************************************/

void trigger_kill(pid_t pid);
void trigger_resume(process_record* pr);

// priority manager: if any processes have terminated/ stopped, increases the priority of the remaining processes
void priority_manager(int stoppedIndex)
{
    int current_priority = latest_running[stoppedIndex];
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (latest_running[i] > current_priority) {
            latest_running[i]--;
        }
    }
}

// returns a priority to the current task about to be done
// higher priority is given to earlier tasks
int priority_allocater(int index)
{
    bool tracker[MAX_RUNNING] = { false };
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (latest_running[i] != -1) {
            tracker[latest_running[i]] = true;
        }
    }
    // tracker will have true for the taken priorities, false for the not taken ones
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (!tracker[i]) {
            return i;
        }
    }
}

int lowest_priority_index()
{
    // there will no process with a lower priority than MAX_RUNNING
    int current_running = MAX_RUNNING;
    int lowest_index = -1;
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (latest_running[i] != -1 && latest_running[i] < current_running) {
            current_running = latest_running[i];
            lowest_index = i;
        }
    }
    return lowest_index;
}

void add_to_queue(process_record* pr)
{
    if (process_queue[add_index] != NULL) {
        printf(
            "Queue is full! Please increase MAX_PROCESSES\n");
    }
    process_queue[add_index] = pr;
    add_index = (add_index + 1) % MAX_QUEUE;
}

void add_to_queue_front(process_record* pr)
{
    if (process_queue[rem_index] != NULL) {
        rem_index = (rem_index - 1 < 0) ? MAX_QUEUE : (rem_index - 1);
    }
    // if process_queue[rem_index] = NULL, it is the first item added to queue
    process_queue[rem_index] = pr;
}

process_record* remove_from_queue(void)
{
    if (process_queue[rem_index] == NULL) {
        return NULL;
    }
    process_record* pr = process_queue[rem_index];
    rem_index = (rem_index + 1) % MAX_QUEUE;
    return pr;
}

void process_tracker(void)
{
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running_processes[i] == NULL) {
            continue;
        }

        pid_t pid = running_processes[i]->pid;
        if (pid > 0) {
            int status;
            if (waitpid(pid, &status, WNOHANG) == pid) {
                printf("parent> Child %d exited with code %d.\n", pid, status);
                running_processes[i]->status = TERMINATED;
                running_processes[i] = NULL;

                // Start next process in the queue
                process_record* next = remove_from_queue();
                if (next != NULL) {
                    trigger_run(next, i);
                }
            }
        }
    }
}

void perform_run(char* args[])
{
    // find a slot to run in
    process_record* p = (process_record*)malloc(sizeof(process_record));
    int running_index = -1;
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running_processes[i] == NULL) {
            running_index = i;
            break;
        }
    }

    int p_idx = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_records[i] == NULL) {
            p_idx = i;
            break;
        }
    }
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed\n");
        return;
    }
    if (pid == 0) {
        const size_t len = strlen(p->args[0]);
        char exec[len + 3];
        strcpy(exec, "./");
        strcat(exec, p->args[0]);
        execvp(exec, p->args);
        // Unreachable code unless execution failed.
        exit(EXIT_FAILURE);
    }

    if (kill(pid, SIGSTOP) == -1) {
        fprintf(stderr, "Could not create child process\n", pid);
        perform_exit();
        exit(EXIT_FAILURE);
    }

    if(running_index != -1){
        p->pid = pid;
        p->index = p_idx;
        trigger_resume(p);
        running_processes[running_index] = p;
        process_records[p_idx] = p;
//        printf("[%d] %d currently running\n", p->index, p->pid);
    } else {
        add_to_queue(p);
//      printf("added to queue");
    }
    
}

void perform_kill(char* args[])
{
    const pid_t pid = atoi(args[0]);
    if (pid <= 0) {
        printf("The process ID must be a positive integer.\n");
        return;
    }
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        process_record* const p = process_records[i];
        if(p != NULL && p->pid == pid && p->status != TERMINATED){
            
        }
    }
    trigger_kill(pid);
}

void trigger_kill(pid_t pid)
{
    
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        process_record* const p = process_records[i];
        if ((p->pid == pid) && (p->status != TERMINATED)) {
            kill(p->pid, SIGTERM);
            printf("[%d] %d killed\n", i, p->pid);
            p->status = TERMINATED;
            return;
        }
    }
    printf("Process %d not found.\n", pid);
}

void perform_stop(char* args[])
{
    const pid_t pid = atoi(args[0]);
    if (pid <= 0) {
        printf("The process ID must be a positive integer.\n");
        return;
    }
    // find the running process
    process_record* pr = NULL;
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running_processes[i] == NULL) {
            continue;
        }
        if (pid == running_processes[i]->pid) {
            pr = running_processes[i];
        }
    }

    if (pr == NULL) {
        printf("Unable to locate process with pid %d", pid);
        return;
    }
    kill(pr->pid, SIGSTOP);
    pr->status = STOPPED;

    // start next process automatically
    process_record* pr = remove_from_queue();
    if (pr != NULL) {
        trigger_run(pr, i);
    }
}

void perform_resume(char* args[])
{
    const pid_t pid = atoi(args[0]);
    if (pid <= 0) {
        printf("The process ID must be a positive integer.\n");
        return;
    }

    process_record* pr = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_records[i] != NULL) {
            if (process_records[i]->pid == pid) {
                pr = process_records[i];
            }
        }
    }

    if (pr == NULL) {
        printf("Unable to locate process with pid %d", pid);
        return;
    }
    if (pr->status == RUNNING) {
        printf("Process, %d is already running\n", pid);
        return;
    }
    // find available running slot
    int running_index = -1;
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running_processes[i] == NULL) {
            running_index = i;
            break;
        }
    }

    // if unable to find slot, free a slot by removing the lastest running process
    if (running_index = -1) {
        int to_stop_idx = lowest_priority_index();
        process_record* to_stop = running_processes[to_stop_idx];
        kill(to_stop->pid, SIGSTOP);
        to_stop->status = READY;
        add_to_queue_front(to_stop);
        running_index = to_stop_idx;
    }



    trigger_resume(pr);
    printf("resuming %d", pr->pid);
    running_processes[running_index] = pr;
}

void trigger_resume(process_record* p)
{
    kill(p->pid, SIGCONT);
    p->status = RUNNING;
}
void perform_list(void)
{
    bool anything = false;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        process_record* const p = process_records[i];
        if (p != NULL) {
            printf("%d, %d\n", p->pid, p->status);
            anything = true;
        }
    }
    if (!anything) {
        printf("No processes to list.\n");
    }
}

void free_process(process_record* pr)
{
    for (int i = 0; i < 10 && pr->args[i] != NULL; i++) {
        free(pr->args[i]);
    }
    free(pr);
}

void perform_exit(void)
{
    // terminate running processes
    for (int i = 0; i < MAX_RUNNING; i++) {
        if (running_processes[i] != NULL) {
            trigger_kill(running_processes[i]->pid);
        }
    }
    // free running processes
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_records[i] != NULL) {
            free_process(process_records[i]);
        }
    }

    printf("bye!\n");
}

/******************************************************************************
 * Input helpers
 ******************************************************************************/

void process_input(char* buffer, char* args[], int args_count_max)
{
    char* p = strtok(buffer, " ");
    int arg_cnt = 0;
    while (p != NULL) {
        args[arg_cnt++] = p;
        if (arg_cnt == args_count_max - 1) {
            break;
        }
        p = strtok(NULL, " ");
    }
    args[arg_cnt] = NULL;
}

char* get_input(char* buffer)
{
    // capture a command
    printf(
        "\x1B[34m"
        "cs205"
        "\x1B[0m"
        "$ ");
    fgets(buffer, 79, stdin);
    for (char* c = buffer; *c != '\0'; ++c) {
        if ((*c == '\r') || (*c == '\n')) {
            *c = '\0';
            break;
        }
    }
    strcat(buffer, " ");
    char buffer2[80];
    strcpy(buffer2, buffer);
    // tokenize command's arguments and retrieve just the command
    char* p = strtok(buffer2, " ");
    char* result = (char*)malloc(21 * sizeof(char));
    strcpy(result, p);
    return result;
}

bool valid_command(char cmd[])
{
    return strcmp(cmd, "kill") == 0 || strcmp(cmd, "run") == 0 || strcmp(cmd, "list") == 0 || strcmp(cmd, "resume") == 0 || strcmp(cmd, "stop") == 0 || strcmp(cmd, "exit") == 0;
}
/******************************************************************************
 * Entry point
 ******************************************************************************/
void run_terminal(int writing_pipe)
{
    char buffer[80];
    // NULL-terminated array
    while (true) {
        char* cmd = get_input(buffer);
        if (valid_command(cmd)) {
            if (write(writing_pipe, buffer, 80) <= 0) {
                printf("unable to write");
                break;
            }
            if (strcmp(cmd, "exit") == 0) {
                break;
            }
        } else {
            printf(
                "invalid command. Valid commands are run, stop, resume, "
                "kill, list "
                "and exit\n");
        }
        free(cmd);
    }
    return;
}

void run_process_manager(int reading_pipe)
{

    while (true) {
        char buffer[100];
        if (read(reading_pipe, buffer, 100) > 0) {
            buffer[99] = '\0';
            char* args[10];
            const int args_count = sizeof(args) / sizeof(*args);
            process_input(buffer, args, args_count);
            char* cmd = args[0];
            if (strcmp(cmd, "kill") == 0) {
                perform_kill(&args[1]);
            } else if (strcmp(cmd, "run") == 0) {
                perform_run(&args[1]);
            } else if (strcmp(cmd, "list") == 0) {
                perform_list();
                // } else if (strcmp(cmd, "resume") == 0) {
                //     perform_resume(&args[1]);
            } else if (strcmp(cmd, "stop") == 0) {
                perform_stop(&args[1]);
            } else if (strcmp(cmd, "exit") == 0) {
                perform_exit();
                break;
            }
        }
        // manage processes
        process_tracker();
    }
    fprintf(stderr, "child > unable to read\n");
}

int main(void)
{
    int p[2];
    if (pipe(p)) {
        return EXIT_FAILURE;
    }
    int reading_pipe = p[0];
    int writing_pipe = p[1];
    fcntl(p[0], F_SETFL, O_NONBLOCK);
    fcntl(p[1], F_SETFL, O_NONBLOCK);

    // create a pipe for the parent process (terminal) to send information
    // (buffer data) to child process (process manager)
    const pid_t child_pid = fork();
    if (child_pid < 0) {
        // Error
        fprintf(stderr, "fork failed\n");
        return EXIT_FAILURE;
    } else if (child_pid != 0) {
        // Parent
        close(reading_pipe);
        run_terminal(writing_pipe);
        close(writing_pipe);
        return EXIT_SUCCESS;
    } else {
        // Child
        close(writing_pipe);
        run_process_manager(reading_pipe);
        close(reading_pipe);
        return EXIT_SUCCESS;
    }
}