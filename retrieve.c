#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define OUTPUT_DIR "/tmp/"

extern int copy_main();

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

// Function to remove trailing whitespace from a string
void trim_trailing_whitespace(char *str) {
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// Function to initialize curl handle with settings
CURL *initialize_curl_handle() {
    CURL *curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return NULL;
    }
    
    // Set CURL options that don't change per request
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    return curl_handle;
}

int retrieve(const char *name, const char *url) {
    CURL *curl_handle;
    char gzip_path[1024];
    FILE *pagefile;

    // Create output file path
    snprintf(gzip_path, sizeof(gzip_path), "%s%s.gz", OUTPUT_DIR, name);

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = initialize_curl_handle();
    if (!curl_handle) {
        curl_global_cleanup();
        return 1;
    }

    // Set CURL-specific options per request
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // Open file for writing
    pagefile = fopen(gzip_path, "wb");
    if (!pagefile) {
        perror("Error opening output file");
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return 1;
    }

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

    // Perform the request
    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
    }

    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return 0;
}

int retrieve_main() {
    FILE *config_file = fopen("/var/pfpb/config.txt", "r");
    if (!config_file) {
        perror("Error opening config file");
        return 1;
    }

    // Ensure the output directory exists
    if (access(OUTPUT_DIR, F_OK) != 0 && mkdir(OUTPUT_DIR, 0755) != 0) {
        perror("Failed to create output directory");
        fclose(config_file);
        return 1;
    }

    char line[1024];
    while (fgets(line, sizeof(line), config_file)) {
        trim_trailing_whitespace(line);

        char *url = strtok(line, ";");
        char *name = strtok(NULL, ";\n");
        if (url && name) {
            retrieve(name, url);
        } else {
            fprintf(stderr, "Malformed line in config file: %s\n", line);
        }
    }

    fclose(config_file);
    copy_main();
    return 0;
}
