#include <stdio.h>
#include <stdlib.h>

// Function to execute a shell command and return the output line count
unsigned long count_lines_from_command(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen");
        return 0;
    }

    unsigned long line_count = 0;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp)) {
        line_count++;
    }

    pclose(fp);
    return line_count;
}

int pfcount_main(void) {
    const char *command = "for i in $(pfctl -sT); do pfctl -t $i -T show; done";
    unsigned long total_ips = count_lines_from_command(command);

    printf("Total IP Ranges Blocked: %lu\n", total_ips);
    return EXIT_SUCCESS;
}
