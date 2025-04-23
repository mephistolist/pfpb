#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Function to execute a shell command and retrieve its output as an unsigned long
unsigned long execute_command_and_get_result(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen");
        return 0;
    }
    char buffer[256];
    unsigned long result = 0;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Strip newline if present
        buffer[strcspn(buffer, "\n")] = 0;
        //printf("Command output: '%s'\n", buffer);  // Debug
        if (sscanf(buffer, "%lu", &result) != 1) {
            fprintf(stderr, "Failed to parse number from output.\n");
        }
    } else {
        fprintf(stderr, "No output from command.\n");
    }
    pclose(fp);
    return result;
}
int pfcount_main(void) {
    const char *command =
        "find /var/pfpb/tables/ -type f -print0 | xargs -0 cat 2>/dev/null | wc -l";
    unsigned long total_lines = execute_command_and_get_result(command);
    printf("Total IP Ranges Blocked: %lu\n", total_lines);
    return EXIT_SUCCESS;
}
