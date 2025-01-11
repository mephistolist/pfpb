#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define GZIP_DIR "/var/pfpb/gzips/"
#define TABLES_DIR "/var/pfpb/tables/"

extern void process_files();

// Function to calculate CIDR notation from an IP range
char *convert_to_cidr(const char *range) {
    char start[BUFFER_SIZE], end[BUFFER_SIZE];
    unsigned int start_ip[4], end_ip[4];
    unsigned int mask_bits = 32;

    if (sscanf(range, "%[^:]:%s", start, end) != 2) {
#ifdef LOG_ERRORS
        fprintf(stderr, "Invalid range format: %s\n", range);
#endif
        return NULL;
    }

    if (sscanf(start, "%u.%u.%u.%u", &start_ip[0], &start_ip[1], &start_ip[2], &start_ip[3]) != 4 ||
        sscanf(end, "%u.%u.%u.%u", &end_ip[0], &end_ip[1], &end_ip[2], &end_ip[3]) != 4) {
#ifdef LOG_ERRORS
        fprintf(stderr, "Invalid IP format in range: %s\n", range);
#endif
        return NULL;
    }

    for (int i = 0; i < 32; i++) {
        unsigned int start_bit = (start_ip[i / 8] >> (7 - (i % 8))) & 1;
        unsigned int end_bit = (end_ip[i / 8] >> (7 - (i % 8))) & 1;
        if (start_bit != end_bit) {
            mask_bits = i;
            break;
        }
    }

    char *cidr = (char *)malloc(BUFFER_SIZE);
    if (!cidr) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    snprintf(cidr, BUFFER_SIZE, "%u.%u.%u.%u/%u",
             start_ip[0], start_ip[1], start_ip[2], start_ip[3], mask_bits);

    return cidr;
}

// Function to process each line of the file
void process_line(const char *line, FILE *output_file) {
    char *tok = strtok(strdup(line), ":");
    char *range = NULL;

    while (tok != NULL) {
        range = tok;
        tok = strtok(NULL, ":");
    }

    if (range) {
        for (size_t i = 0; i < strlen(range); i++) {
            if (range[i] == '-') {
                range[i] = ':';
            }
        }

        char *cidr = convert_to_cidr(range);
        if (cidr) {
            fprintf(output_file, "%s\n", cidr);
            free(cidr);
        }
    }
}

// Function to process a single gzip file
void process_gzip_file(const char *gzip_file, const char *output_file) {
    gzFile gzfile = gzopen(gzip_file, "r");
    if (!gzfile) {
        perror("gzopen failed");
        return;
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        perror("fopen failed");
        gzclose(gzfile);
        return;
    }

    char buffer[BUFFER_SIZE];
    int line_count = 0;

    while (gzgets(gzfile, buffer, BUFFER_SIZE)) {
        if (++line_count <= 2) {
            continue;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        process_line(buffer, output);
    }

    gzclose(gzfile);
    fclose(output);
    sleep(0);
}

// Main function to process all gzip files in the directory
int parser_main() {
    DIR *dir = opendir(GZIP_DIR);
    if (!dir) {
        perror("opendir failed");
        exit(EXIT_FAILURE);
    }

    printf("Processing lists. Please wait...\n");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *filename = entry->d_name;
            const char *extension = strrchr(filename, '.');

            if (extension && strcmp(extension, ".gz") == 0) {
                char gzip_path[BUFFER_SIZE], output_path[BUFFER_SIZE];
                snprintf(gzip_path, BUFFER_SIZE, "%s%s", GZIP_DIR, filename);
                snprintf(output_path, BUFFER_SIZE, "%s%s", TABLES_DIR, filename);

                // Remove the ".gz" extension for the output file
                output_path[strlen(output_path) - 3] = '\0';

                process_gzip_file(gzip_path, output_path);
            }
        }
    }

    closedir(dir);
    process_files();
    return 0;
}