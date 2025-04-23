#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define TABLES_DIR "/var/pfpb/tables/"
#define MAX_LINE_LENGTH 1024
#define MAX_LINES 100000

// Function to compare two strings (for qsort)
int compare_lines(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Sorts a file in-place without using system()
void sort_file(const char *filename) {
    char path[512];
    snprintf(path, sizeof(path), "%s%s", TABLES_DIR, filename);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening file for reading");
        return;
    }
    char *lines[MAX_LINES];
    size_t line_count = 0;
    char buffer[MAX_LINE_LENGTH];
    // Read lines into memory
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (line_count >= MAX_LINES) {
            fprintf(stderr, "Too many lines in %s (limit: %d)\n", filename, MAX_LINES);
            fclose(fp);
            return;
        }
        lines[line_count] = strdup(buffer);
        if (!lines[line_count]) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(fp);
            return;
        }
        line_count++;
    }
    fclose(fp);
    // Sort and deduplicate
    qsort(lines, line_count, sizeof(char *), compare_lines);
    size_t unique_count = 0;
    for (size_t i = 1; i < line_count; i++) {
        if (strcmp(lines[i], lines[unique_count]) != 0) {
            unique_count++;
            lines[unique_count] = lines[i];
        } else {
            free(lines[i]); // duplicate
        }
    }
    unique_count++; // include the first one
    // Write back to file
    fp = fopen(path, "w");
    if (!fp) {
        perror("Error opening file for writing");
        return;
    }
    for (size_t i = 0; i < unique_count; i++) {
        fputs(lines[i], fp);
        free(lines[i]);
    }
    fclose(fp);
}

void process_files(void) {
    struct dirent *entry;
    DIR *dp = opendir(TABLES_DIR);
    if (dp == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.' || entry->d_type == DT_DIR) {
            continue;
        }
        sort_file(entry->d_name);
    }
    closedir(dp);
    printf("Processing Lists Complete.\n");
}
