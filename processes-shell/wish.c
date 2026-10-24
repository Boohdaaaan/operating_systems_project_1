#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

static pid_t run_command(char **args, const char *outfile) {
    char *prog = find_executable(args[0]);
    if (prog == NULL) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
    } else if (pid == 0) {
        if (outfile != NULL) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(prog, args);
        print_error();
        exit(1);
    }
    free(prog);
    return pid;
}

/* splits "cmd args > file"; returns -1 on syntax error */
static int parse_redirect(char *cmd, char **outfile) {
    *outfile = NULL;
    char *op = strchr(cmd, '>');
    if (op == NULL)
        return 0;
    *op = '\0';

    char *rest = op + 1;
    if (strchr(rest, '>') != NULL)
        return -1;

    int n;
    char **files = split_args(rest, &n);
    if (n == 1)
        *outfile = files[0];
    free(files);
    return n == 1 ? 0 : -1;
}

static void set_path(char **args, int argc) {
    for (int i = 0; i < npaths; i++)
        free(paths[i]);
    free(paths);

    npaths = argc;
    paths = malloc((argc > 0 ? argc : 1) * sizeof(char *));
    for (int i = 0; i < argc; i++)
        paths[i] = strdup(args[i]);
}

static int run_builtin(char **args, int argc) {
    if (strcmp(args[0], "exit") == 0) {
        if (argc != 1)
            print_error();
        else
            exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (argc != 2 || chdir(args[1]) != 0)
            print_error();
    } else if (strcmp(args[0], "path") == 0) {
        set_path(args + 1, argc - 1);
    } else {
        return 0;
    }
    return 1;
}

static void process_line(char *line) {
    char *outfile;
    if (parse_redirect(line, &outfile) < 0) {
        print_error();
        return;
    }

    int argc;
    char **args = split_args(line, &argc);

    if (argc == 0) {
        if (outfile != NULL)
            print_error();
    } else if (!run_builtin(args, argc)) {
        pid_t pid = run_command(args, outfile);
        if (pid > 0)
            waitpid(pid, NULL, 0);
    }
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
