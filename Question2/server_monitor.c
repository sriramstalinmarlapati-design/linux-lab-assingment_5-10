/*
 * server_monitor.c
 * Simulates a web server that creates child processes (e.g., to handle requests),
 * monitors them, reaps them properly to avoid zombies, and terminates
 * any child that becomes unresponsive (runs too long) using signals.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define NUM_CHILDREN 4
#define TIMEOUT_SECONDS 3

int main() {
    pid_t children[NUM_CHILDREN];

    printf("Parent PID: %d - creating %d child processes...\n", getpid(), NUM_CHILDREN);
    fflush(stdout); /* flush BEFORE fork so buffered output isn't duplicated
                        in each child's copy of the stdio buffer */

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            printf("  Child %d (PID %d) started.\n", i, getpid());
            fflush(stdout);

            if (i == 2) {
                sleep(10); /* simulate a hang */
            } else {
                sleep(1 + i); /* simulate normal quick work */
            }

            printf("  Child %d (PID %d) finished work.\n", i, getpid());
            exit(0);
        } else {
            children[i] = pid;
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        time_t start = time(NULL);
        int status;
        pid_t result;

        while (1) {
            result = waitpid(children[i], &status, WNOHANG);

            if (result == children[i]) {
                if (WIFEXITED(status)) {
                    printf("Parent: child PID %d exited normally (code %d).\n",
                           children[i], WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("Parent: child PID %d was killed by signal %d.\n",
                           children[i], WTERMSIG(status));
                }
                break;
            } else if (result == 0) {
                if (time(NULL) - start >= TIMEOUT_SECONDS) {
                    printf("Parent: child PID %d unresponsive (>%ds). Sending SIGTERM.\n",
                           children[i], TIMEOUT_SECONDS);
                    kill(children[i], SIGTERM);

                    waitpid(children[i], &status, 0);
                    printf("Parent: child PID %d reaped after SIGTERM.\n", children[i]);
                    break;
                }
                usleep(200000);
            } else {
                perror("waitpid error");
                break;
            }
        }
    }

    printf("Parent: all children handled, no zombies left.\n");
    return 0;
}
