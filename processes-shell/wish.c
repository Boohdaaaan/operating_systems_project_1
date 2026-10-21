#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_error(void) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

static void process_line(char *line) {
    (void)line;
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
