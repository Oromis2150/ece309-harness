# C Command-Line Program & Bash Test Script Guide

## 1. C Program Implementation & Libraries

### User Requirements:
1. Run an infinite `while` loop that asks for user input using `fgets`.
2. If the user types `'exit'`, the loop breaks and the program ends.
3. If the user types a sentence containing the word `'hello'`, print a hardcoded greeting.
4. If the user types anything else, echo their input back to them.

---

### Required Header Files & Functions

* **`<stdio.h>` (Standard Input/Output)**
  * `fgets()`: Reads a line of text from standard input into a buffer safely, preventing buffer overflows.
  * `printf()`: Displays output (greetings, echoed input, and prompt messages) to the console.

* **`<string.h>` (String Handling)**
  * `strstr()`: Searches for the first occurrence of a substring (e.g., `"hello"`) within another string. Returns a non-`NULL` pointer if found.
  * `strncmp()`: Compares two strings up to a specified number of characters. Useful for checking if the input starts with `"exit"`.
  * `strcspn()`: Helps strip the trailing newline (`\n`) character that `fgets` automatically includes when the user presses Enter.

---

### Example C Implementation

```c
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
            printf("You typed: %s\n", input);
        }
    }

    return 0;
}
```

---

## 2. Bash Test Script for Executable `harness`

To automatically test the compiled program named `harness` by sending `'hello'` followed by `'exit'`, you can use any of the following approaches in Bash:

### Method 1: Using `printf` Pipeline

```bash
#!/bin/bash

# Send 'hello' followed by 'exit' into the compiled harness program
printf "hello\nexit\n" | ./harness
```

### Method 2: Using a Here Document

```bash
#!/bin/bash

./harness << EOF
hello
exit
EOF
```

---

### Execution Instructions

1. Save one of the scripts above as `test.sh` in the same directory as your compiled `harness` program.
2. Grant execution permissions to the script:
   ```bash
   chmod +x test.sh
   ```
3. Execute the script:
   ```bash
   ./test.sh
   ```
