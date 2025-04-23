#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#define OUTPUT_DIR "/tmp/"
// Function prototype for copy_main
extern int copy_main(void);
// Function prototype for initialize_curl_handle
CURL *initialize_curl_handle(void);
// Function to write data to the file
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
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
CURL *initialize_curl_handle(void) {
    CURL *curl_handle = curl_easy_init();
    if (!curl_handle) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return NULL;
    }
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    return curl_handle;
}
// Validate that the URL begins with http:// or https://
int is_valid_url(const char *url) {
    return strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0;
}
// Check whether the URL belongs to an allowed domain
int is_allowed_domain(const char *url) {
    const char *trusted_domains[] = {
        "http://list.iblocklist.com/",
        "https://example.com/"
        // Add more trusted domains here
    };
    size_t num_domains = sizeof(trusted_domains) / sizeof(trusted_domains[0]);
    for (size_t i = 0; i < num_domains; ++i) {
        if (strncmp(url, trusted_domains[i], strlen(trusted_domains[i])) == 0) {
            return 1;
        }
    }
    return 0;
}
int retrieve(const char *name, const char *url) {
    CURL *curl_handle;
    char gzip_path[1024];
    FILE *pagefile;
    snprintf(gzip_path, sizeof(gzip_path), "%s%s.gz", OUTPUT_DIR, name);
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = initialize_curl_handle();
    if (!curl_handle) {
        curl_global_cleanup();
        return 1;
    }
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    pagefile = fopen(gzip_path, "wb");
    if (!pagefile) {
        perror("Error opening output file");
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return 1;
    }
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
    }
    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return 0;
}
int retrieve_main(void) {
    FILE *config_file = fopen("/var/pfpb/config.txt", "r");
    if (!config_file) {
        perror("Error opening config file");
        return 1;
    }
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
            if (!is_valid_url(url)) {
                fprintf(stderr, "Blocked invalid URL format: %s\n", url);
                continue;
            }
            if (!is_allowed_domain(url)) {
                fprintf(stderr, "Blocked untrusted URL: %s\n", url);
                continue;
            }
            retrieve(name, url);
        } else {
            fprintf(stderr, "Malformed line in config file: %s\n", line);
        }
    }
    fclose(config_file);
    copy_main();
    return 0;
}
