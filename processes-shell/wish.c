#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define WHITESPACE " \t\r\n"

static char **paths;
static int npaths;

static void print_error(void) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

static char **split_args(char *s, int *argc) {
    int cap = 8, n = 0;
    char **args = malloc(cap * sizeof(char *));
    char *tok;

    while ((tok = strsep(&s, WHITESPACE)) != NULL) {
        if (*tok == '\0')
            continue;
        if (n + 1 >= cap) {
            cap *= 2;
            args = realloc(args, cap * sizeof(char *));
        }
        args[n++] = tok;
    }
    args[n] = NULL;
    *argc = n;
    return args;
}

static char *find_executable(const char *name) {
    for (int i = 0; i < npaths; i++) {
        size_t len = strlen(paths[i]) + strlen(name) + 2;
        char *full = malloc(len);
        snprintf(full, len, "%s/%s", paths[i], name);
        if (access(full, X_OK) == 0)
            return full;
        free(full);
    }
    return NULL;
}

static void run_command(char **args) {
    char *prog = find_executable(args[0]);
    if (prog == NULL) {
        print_error();
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
    } else if (pid == 0) {
        execv(prog, args);
        print_error();
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }
    free(prog);
}

static void process_line(char *line) {
    int argc;
    char **args = split_args(line, &argc);

    if (argc > 0)
        run_command(args);
    free(args);
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    int interactive = 1;

    if (argc > 2) {
        print_error();
        exit(1);
    }
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();
            exit(1);
        }
        interactive = 0;
    }

    npaths = 1;
    paths = malloc(sizeof(char *));
    paths[0] = strdup("/bin");

    char *line = NULL;
    size_t cap = 0;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }
        if (getline(&line, &cap, input) == -1)
            break;
        process_line(line);
    }

    free(line);
    if (input != stdin)
        fclose(input);
    exit(0);
}
