#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 256

int main(void) {
    char input[BUFFER_SIZE];

    while (1) {
        printf("> ");
        
        // Read user input safely
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove trailing newline character added by fgets
        input[strcspn(input, "\n")] = '\0';

        // Check for 'exit' condition
        if (strncmp(input, "exit", 4) == 0 && (input[4] == '\0' || input[4] == '\r')) {
            printf("Exiting program.\n");
            break;
        }

        // Check if the input contains 'hello'
        if (strstr(input, "hello") != NULL) {
            printf("Hello there! Welcome!\n");
        } else {
            // Echo back anything else
            printf("%s\n", input);
        }
    }

    return 0;
}