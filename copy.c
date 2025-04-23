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
    char *dir_path = strdup(path);
    if (!dir_path) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    
    char *parent_dir = dirname(dir_path);
    struct stat st;
    if (stat(parent_dir, &st) == -1) {
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
    struct stat src_stat;
    if (stat(src, &src_stat) != 0) {
        perror("Source file does not exist");
        return;
    }
    
    struct stat dest_stat;
    char final_dest[PATH_MAX];
    if (stat(dest, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        char src_copy[PATH_MAX];
        strncpy(src_copy, src, sizeof(src_copy));
        src_copy[sizeof(src_copy) - 1] = '\0';
        snprintf(final_dest, sizeof(final_dest), "%s/%s", dest, basename(src_copy));
    } else {
        snprintf(final_dest, sizeof(final_dest), "%s", dest);
    }
    
    ensure_directory_exists(final_dest);
    if (rename(src, final_dest) == 0) {
        printf("Updates found for %s\n", final_dest);
        printf("Now working to reload.\n");
        return;
    }
    
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
    
    char buffer[BUFFER_SIZE];
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
    if (unlink(src) != 0) {
        perror("unlink source file");
    }
}
int calculate_sha256(const char *filename, unsigned char *result, unsigned int *result_len) {
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
    
    const EVP_MD *md = EVP_sha256();
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
    
    if (EVP_DigestFinal_ex(mdctx, result, result_len) != 1) {
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
    int updates_found = 0;
    while ((entry = readdir(tmp_dir)) != NULL) {
        if (strstr(entry->d_name, ".gz") == NULL) {
            continue;
        }
        
        char tmp_file_path[PATH_MAX];
        char gzip_file_path[PATH_MAX];
        snprintf(tmp_file_path, sizeof(tmp_file_path), "%s/%s", TMP_DIR, entry->d_name);
        snprintf(gzip_file_path, sizeof(gzip_file_path), "%s/%s", GZIP_DIR, entry->d_name);
        unsigned char tmp_sha[EVP_MAX_MD_SIZE];
        unsigned char gzip_sha[EVP_MAX_MD_SIZE];
        unsigned int tmp_len = 0, gzip_len = 0;
        if (calculate_sha256(tmp_file_path, tmp_sha, &tmp_len) != 0) {
            fprintf(stderr, "Failed to calculate SHA-256 for %s\n", tmp_file_path);
            continue;
        }
        
        if (access(gzip_file_path, F_OK) == 0) {
            if (calculate_sha256(gzip_file_path, gzip_sha, &gzip_len) != 0) {
                fprintf(stderr, "Failed to calculate SHA-256 for %s\n", gzip_file_path);
                continue;
            }
            
            if (tmp_len == gzip_len && memcmp(tmp_sha, gzip_sha, tmp_len) == 0) {
                continue; // No changes, skip
            }
        }
        
        char *base = basename(entry->d_name);
        char file_name_without_gz[PATH_MAX];
        strncpy(file_name_without_gz, base, sizeof(file_name_without_gz));
        file_name_without_gz[sizeof(file_name_without_gz) - 1] = '\0';
        char *gz_ext = strstr(file_name_without_gz, ".gz");
        if (gz_ext) {
            *gz_ext = '\0';
        }
        
        printf("Updating %s\n", file_name_without_gz);
        move_file(tmp_file_path, gzip_file_path);
        updates_found = 1;
    }
    closedir(tmp_dir);
    if (!updates_found) {
        printf("No updates were found.\nAll files are up-to-date.\n");
        exit(EXIT_SUCCESS);
    }
    
    parser_main();
    return EXIT_SUCCESS;
}
