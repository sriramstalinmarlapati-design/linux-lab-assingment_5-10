/*
 * employee_records.c
 * Secure file-processing utility using raw Linux system calls
 * (open, read, write, lseek, close) instead of stdio's fopen/fread/fwrite.
 *
 * Records are FIXED SIZE, which is what makes direct-access update/retrieve
 * possible without rewriting the whole file: record N always lives at
 * byte offset N * sizeof(struct employee).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define FILENAME "employees.dat"
#define NAME_LEN 32

struct employee {
    int id;
    char name[NAME_LEN];
    float salary;
};

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <init|add|update|get|list> [args]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        int fd = open(FILENAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) die("open (init)");
        printf("Initialized empty employee records file: %s\n", FILENAME);
        close(fd);

    } else if (strcmp(argv[1], "add") == 0) {
        if (argc != 5) { fprintf(stderr, "Usage: add <id> <name> <salary>\n"); return 1; }

        struct employee e;
        e.id = atoi(argv[2]);
        strncpy(e.name, argv[3], NAME_LEN - 1);
        e.name[NAME_LEN - 1] = '\0';
        e.salary = atof(argv[4]);

        int fd = open(FILENAME, O_WRONLY | O_APPEND);
        if (fd < 0) die("open (add)");

        ssize_t written = write(fd, &e, sizeof(e));
        if (written != sizeof(e)) die("write (add)");

        printf("Added employee: id=%d name=%s salary=%.2f\n", e.id, e.name, e.salary);
        close(fd);

    } else if (strcmp(argv[1], "update") == 0) {
        if (argc != 6) { fprintf(stderr, "Usage: update <index> <id> <name> <salary>\n"); return 1; }

        int index = atoi(argv[2]);
        struct employee e;
        e.id = atoi(argv[3]);
        strncpy(e.name, argv[4], NAME_LEN - 1);
        e.name[NAME_LEN - 1] = '\0';
        e.salary = atof(argv[5]);

        int fd = open(FILENAME, O_WRONLY);
        if (fd < 0) die("open (update)");

        off_t offset = (off_t)index * sizeof(struct employee);
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) die("lseek (update)");

        ssize_t written = write(fd, &e, sizeof(e));
        if (written != sizeof(e)) die("write (update)");

        printf("Updated record at index %d: id=%d name=%s salary=%.2f\n",
               index, e.id, e.name, e.salary);
        close(fd);

    } else if (strcmp(argv[1], "get") == 0) {
        if (argc != 3) { fprintf(stderr, "Usage: get <index>\n"); return 1; }

        int index = atoi(argv[2]);
        struct employee e;

        int fd = open(FILENAME, O_RDONLY);
        if (fd < 0) die("open (get)");

        off_t offset = (off_t)index * sizeof(struct employee);
        if (lseek(fd, offset, SEEK_SET) == (off_t)-1) die("lseek (get)");

        ssize_t bytes = read(fd, &e, sizeof(e));
        if (bytes == 0) {
            printf("No record at index %d (end of file).\n", index);
        } else if (bytes != sizeof(e)) {
            die("read (get)");
        } else {
            printf("Record %d -> id=%d name=%s salary=%.2f\n", index, e.id, e.name, e.salary);
        }
        close(fd);

    } else if (strcmp(argv[1], "list") == 0) {
        struct employee e;
        int fd = open(FILENAME, O_RDONLY);
        if (fd < 0) die("open (list)");

        int index = 0;
        ssize_t bytes;
        while ((bytes = read(fd, &e, sizeof(e))) == sizeof(e)) {
            printf("Record %d -> id=%d name=%s salary=%.2f\n", index, e.id, e.name, e.salary);
            index++;
        }
        if (bytes < 0) die("read (list)");
        close(fd);

    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
