
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define TABLES_DIR "/var/pfpb/tables/"

void sort_file(const char *filename) {
    char command[512];

    // Construct the sort command
    snprintf(command, sizeof(command), "sort -u %s%s -o %s%s", 
             TABLES_DIR, filename, TABLES_DIR, filename);

    // Execute the command
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error sorting file: %s (return code: %d)\n", filename, ret);
    }
}

void process_files() {
    struct dirent *entry;
    DIR *dp = opendir(TABLES_DIR);

    if (dp == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dp)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Skip directories
        if (entry->d_type == DT_DIR) {
            continue;
        }

        // Call sort_file for each file
        sort_file(entry->d_name);
    }

    closedir(dp);
    printf("Processing Lists Complete.\n");
}
