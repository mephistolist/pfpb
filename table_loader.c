#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TABLES_DIR "/var/pfpb/tables/"
#define PFCTL_CMD "pfctl"

extern int pfcount_main();

// Function prototypes
void load_pf_tables(DIR *dp);
void flush_pf_tables(DIR *dp);
int loader_main(const char *mode);

// Executes pfctl to load a table
void execute_pfctl_load(const char *filename) {
    char command[512];
    snprintf(command, sizeof(command), "%s -t %s -T add -f %s%s 2>/dev/null", 
             PFCTL_CMD, filename, TABLES_DIR, filename);

    printf("Loading %s...\n", filename);

    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error loading table: %s\n", filename);
    }
}

// Executes pfctl to flush a table
void execute_pfctl_flush(const char *filename) {
    char command[512];
    snprintf(command, sizeof(command), "%s -t %s -T kill 2>/dev/null", PFCTL_CMD, filename);

    printf("Stopping %s...\n", filename);

    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error flushing table: %s\n", filename);
    }
}

// Loads all tables from the directory
void load_pf_tables(DIR *dp) {
    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Execute pfctl for each file
        execute_pfctl_load(entry->d_name);
    }
}

// Flushes all tables from the directory
void flush_pf_tables(DIR *dp) {
    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.') {
            continue;
        }

        // Execute pfctl for each file
        execute_pfctl_flush(entry->d_name);
    }
}

// Main loader function to be called
int loader_main(const char *mode) {
    DIR *dp = opendir(TABLES_DIR);
    if (dp == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    if (strcmp(mode, "start") == 0) {
        load_pf_tables(dp);
        pfcount_main();
        printf("Done.\n");
    } else if (strcmp(mode, "stop") == 0) {
        flush_pf_tables(dp);
        pfcount_main();
        printf("Done.\n");
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        closedir(dp);
        return EXIT_FAILURE;
    }

    closedir(dp);
    return EXIT_SUCCESS;
}
