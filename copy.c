#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#define TMP_DIR "/tmp"
#define GZIP_DIR "/var/pfpb/gzips"
#define BUFFER_SIZE 8192

extern int parser_main(void);

void ensure_directory_exists(const char *path) {
    char *dir_path = strdup(path); // Create a modifiable copy of the path
    if (!dir_path) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    char *parent_dir = dirname(dir_path); // Get the parent directory
    struct stat st;

    // Check if the directory exists
    if (stat(parent_dir, &st) == -1) {
        // Directory doesn't exist, attempt to create it
        if (mkdir(parent_dir, 0755) != 0) {
            perror("mkdir failed");
            free(dir_path);
            exit(EXIT_FAILURE);
        }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory\n", parent_dir);
        free(dir_path);
        exit(EXIT_FAILURE);
    }

    free(dir_path);
}

void move_file(const char *src, const char *dest) {
    // Check if the source file exists
    struct stat src_stat;
    if (stat(src, &src_stat) != 0) {
        perror("Source file does not exist");
        return;
    }

    // Handle the case where dest is a directory
    struct stat dest_stat;
    char final_dest[PATH_MAX];
    if (stat(dest, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        snprintf(final_dest, sizeof(final_dest), "%s/%s", dest, basename((char *)src;
    } else {
        snprintf(final_dest, sizeof(final_dest), "%s", dest);
    }

    // Ensure destination directory exists
    ensure_directory_exists(final_dest);

    // Attempt to rename first
    if (rename(src, final_dest) == 0) {
        printf("Updates found for %s\n", final_dest);
        printf("Now working to reload.\n");
        return;
    }

    // Fallback to copy if rename fails
    FILE *src_file = fopen(src, "rb");
    if (!src_file) {
        perror("fopen source file");
        return;
    }

    FILE *dest_file = fopen(final_dest, "wb");
    if (!dest_file) {
        perror("fopen destination file");
        fclose(src_file);
        return;
    }

    char buffer[8192];
    size_t bytes_read, bytes_written;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_read, dest_file);
        if (bytes_written < bytes_read) {
            perror("fwrite");
            break;
        }
    }

    if (ferror(src_file)) {
        perror("fread");
    }

    fclose(src_file);
    fclose(dest_file);

    // Remove the source file after successful copy
    if (unlink(src) != 0) {
        perror("unlink source file");
    }
}

// Function to calculate MD5 hash
int calculate_md5(const char *filename, unsigned char *result) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("EVP_MD_CTX_new");
        return -1;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    const EVP_MD *md = EVP_md5();
    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        perror("EVP_DigestInit_ex");
        fclose(file);
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
            perror("EVP_DigestUpdate");
            fclose(file);
            EVP_MD_CTX_free(mdctx);
            return -1;
        }
    }

    if (ferror(file)) {
        perror("fread");
        fclose(file);
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    if (EVP_DigestFinal_ex(mdctx, result, NULL) != 1) {
        perror("EVP_DigestFinal_ex");
        fclose(file);
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    fclose(file);
    EVP_MD_CTX_free(mdctx);
    return 0;
}

int copy_main(void) {
    DIR *tmp_dir = opendir(TMP_DIR);
    if (!tmp_dir) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    int updates_found = 0; // Flag to track updates

    while ((entry = readdir(tmp_dir)) != NULL) {
        if (strstr(entry->d_name, ".gz") == NULL) {
            continue;
        }

        char tmp_file_path[PATH_MAX];
        char gzip_file_path[PATH_MAX];
        snprintf(tmp_file_path, sizeof(tmp_file_path), "%s/%s", TMP_DIR, entry->d_na;
        snprintf(gzip_file_path, sizeof(gzip_file_path), "%s/%s", GZIP_DIR, entry->d;

        unsigned char tmp_md5[EVP_MAX_MD_SIZE];
        unsigned char gzip_md5[EVP_MAX_MD_SIZE];

        if (calculate_md5(tmp_file_path, tmp_md5) != 0) {
            fprintf(stderr, "Failed to calculate MD5 for %s\n", tmp_file_path);
            continue;
        }

        if (access(gzip_file_path, F_OK) == 0) { // Check if the file exists
            if (calculate_md5(gzip_file_path, gzip_md5) != 0) {
                fprintf(stderr, "Failed to calculate MD5 for %s\n", gzip_file_path);
                continue;
            }

            if (memcmp(tmp_md5, gzip_md5, EVP_MD_size(EVP_md5())) == 0) {
                // Skip identical files
                continue;
            }
        }

        // Get the basename and remove the ".gz" extension
        char *basename_file = basename(entry->d_name);
        char file_name_without_gz[PATH_MAX];
        snprintf(file_name_without_gz, sizeof(file_name_without_gz), "%s", basename_;

        // Remove the .gz suffix
        char *gz_ext = strstr(file_name_without_gz, ".gz");
        if (gz_ext) {
            *gz_ext = '\0';  // Null-terminate the string at ".gz"
        }

        printf("Updating %s\n", file_name_without_gz);
        move_file(tmp_file_path, gzip_file_path);
        updates_found = 1; // Mark that an update was found
    }

    closedir(tmp_dir);

    if (!updates_found) {
        printf("No updates were found.\nAll files are up-to-date.\n");
        exit(EXIT_SUCCESS);
    }

    parser_main();
    return EXIT_SUCCESS;
}
