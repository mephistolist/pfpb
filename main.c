#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define TMP_FILE "/tmp/original_entries.txt"
#define DEFAULT_TABLE_ENTRIES 1000000

extern int loader_main(const char *mode);
extern int retrieve_main(void);
extern int pfcount_main(void);

// Function prototypes
void start_function(void);
void stop_function(void);
void update_function(void);
void print_usage(const char *program_name);
int save_original_entries(const int *value);
int load_original_entries(int *value);
int get_table_entries(void);
void set_table_entries(const int *value);
void run_quiet(const char *cmd_path, char *const argv[]);

void print_usage(const char *program_name) {
    printf("Usage: %s <command>\n", program_name);
    printf("Commands:\n");
    printf("  pfpb start  - Start and load the PF tables containing IP ranges from iblocklist to block.\n");
    printf("  pfpb stop   - Stop all PF tables and unblock IP ranges.\n");
    printf("  pfpb update - Update any new blocklists so if any new IP ranges exist, they can be applied.\n");
}

int save_original_entries(const int *value) {
    FILE *fp = fopen(TMP_FILE, "w");
    if (!fp) {
        perror("Failed to open temporary file for writing");
        return -1;
    }
    fprintf(fp, "%d\n", *value);
    fclose(fp);
    return 0;
}

int load_original_entries(int *value) {
    FILE *fp = fopen(TMP_FILE, "r");
    if (!fp) {
        perror("Failed to open temporary file for reading");
        return -1;
    }
    if (fscanf(fp, "%d", value) != 1) {
        fprintf(stderr, "Failed to read value from temporary file.\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int get_table_entries(void) {
    FILE *fp = popen("pfctl -sm | awk '/table-entries/ { print $4 }'", "r");
    if (!fp) {
        perror("popen failed");
        return -1;
    }
    char result[128] = {0};
    int table_entries = 0;
    if (fgets(result, sizeof(result), fp)) {
        table_entries = atoi(result);
    } else {
        fprintf(stderr, "Failed to read table-entries value.\n");
        table_entries = -1;
    }
    pclose(fp);
    return table_entries;
}

void set_table_entries(const int *value) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid == 0) {
        // Child process
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execl("/sbin/pfctl", "pfctl", "-f", "-", (char *)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(pipefd[0]);
        dprintf(pipefd[1], "set limit { table-entries %d }\n", *value);
        close(pipefd[1]);
        waitpid(pid, NULL, 0);
    }
}

void run_quiet(const char *cmd_path, char *const argv[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Redirect output to /dev/null
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        execvp(cmd_path, argv);
        perror("execvp");
        _exit(EXIT_FAILURE);
    } else {
        waitpid(pid, NULL, 0);
    }
}

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        fprintf(stderr, "This program must be run as root.\n");
        return EXIT_FAILURE;
    }
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    const char *command = argv[1];
    if (strcmp(command, "start") == 0) {
        start_function();
    } else if (strcmp(command, "stop") == 0) {
        stop_function();
    } else if (strcmp(command, "update") == 0) {
        update_function();
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void start_function(void) {
    int original_entries = get_table_entries();
    if (original_entries == -1) {
        fprintf(stderr, "Failed to retrieve the original table-entries value.\n");
        return;
    }
    if (save_original_entries(&original_entries) != 0) {
        fprintf(stderr, "Failed to save the original table-entries value.\n");
        return;
    }
    set_table_entries(&(const int){DEFAULT_TABLE_ENTRIES});
    printf("Starting: Loading PF tables...\n");
    if (loader_main("start") != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Failed to load PF tables.\n");
    }
}

void stop_function(void) {
    int original_entries;
    if (load_original_entries(&original_entries) != 0) {
        fprintf(stderr, "Failed to load the original table-entries value.\n");
        return;
    }
    set_table_entries(&original_entries);
    printf("Stopping: Flushing PF tables...\n");
    if (loader_main("stop") != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Failed to flush PF tables.\n");
    }
}

void update_function(void) {
    printf("Retrieving updates.\nPlease wait...\n");
    retrieve_main();
    // Run `pfpb stop` and `pfpb start` quietly
    char *stop_argv[] = { "pfpb", "stop", NULL };
    char *start_argv[] = { "pfpb", "start", NULL };
    run_quiet("pfpb", stop_argv);
    run_quiet("pfpb", start_argv);
    printf("Updating is complete.\n");
    pfcount_main();
}
