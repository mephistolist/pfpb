#include <stdio.h>
#include <stdlib.h>

// Function to execute a shell command and retrieve its output as an unsigned long
unsigned long execute_command_and_get_result(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen");
        return 0;
    }

    unsigned long result = 0;
    if (fscanf(fp, "%lu", &result) != 1) {
        fprintf(stderr, "Failed to parse the output of the command.\n");
    }

    pclose(fp);
    return result;
}

int pfcount_main(void) {
    const char *command = "find /var/pfpb/tables/ -type f -exec wc -l {} \\; | awk '{total += $1} EN;
    unsigned long total_lines = execute_command_and_get_result(command);

    printf("Total IP Ranges Blocked: %lu\n", total_lines);
    return EXIT_SUCCESS;
}
