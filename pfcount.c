#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Function to calculate the number of IPs in a CIDR range
unsigned long count_ips_in_cidr(const char *cidr) {
    char *slash = strchr(cidr, '/');
    if (!slash) {
        return 1; // Single IP address
    }

    int prefix_len = atoi(slash + 1);
    return (1UL << (32 - prefix_len)); // 2^(32 - prefix_len)
}

// Function to execute a shell command and return the output
char *run_command(const char *command) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    size_t size = 0;
    char *result = NULL;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        char *new_result = realloc(result, size + len + 1);
        if (!new_result) {
            perror("realloc");
            free(result);
            pclose(fp);
            exit(EXIT_FAILURE);
        }
        result = new_result;
        strcpy(result + size, buffer);
        size += len;
    }

    pclose(fp);
    return result;
}

int pfcount_main() {
    char *tables_output = run_command("pfctl -sT");
    if (!tables_output || strlen(tables_output) == 0) {
        printf("Total IPs blocked: 0\n");
        free(tables_output);
        return EXIT_SUCCESS;
    }

    unsigned long total_ips = 0;
    char *table = strtok(tables_output, "\n");
    while (table) {
        char command[256];
        snprintf(command, sizeof(command), "pfctl -t %s -T show", table);
        char *table_output = run_command(command);

        // If fetching the table fails, continue without printing errors
        if (!table_output || strlen(table_output) == 0) {
            free(table_output);
            table = strtok(NULL, "\n");
            continue;
        }

        char *line = strtok(table_output, "\n");
        while (line) {
            total_ips += count_ips_in_cidr(line);
            line = strtok(NULL, "\n");
        }

        free(table_output);
        table = strtok(NULL, "\n");
    }

    printf("Total IPs blocked: %lu\n", total_ips);

    free(tables_output);
    return EXIT_SUCCESS;
}
