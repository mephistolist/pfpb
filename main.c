#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// Define constants for improved readability and maintainability
#define TMP_FILE "/tmp/original_entries.txt"
#define DEFAULT_TABLE_ENTRIES 1000000

// External function declarations
extern int loader_main(const char *mode);
extern int retrieve_main();
extern int pfcount_main();

// Function prototypes
void start_function();
void stop_function();
void update_function();
void print_usage(const char *program_name);
int save_original_entries(const int *value);
int load_original_entries(int *value);
int get_table_entries();
void set_table_entries(const int *value);

void print_usage(const char *program_name) {
    printf("Usage: %s <command>\n", program_name);
    printf("Commands:\n");
    printf(" pfpb start   Start loading PF tables\n");
    printf(" pfpb stop    Stop (flush) PF tables\n");
    printf(" pfpb update  Update any new blocklists\n");
}

// Function to save the original table-entries value to a file
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

// Function to load the original table-entries value from a file
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

// Function to get the current value of table-entries
int get_table_entries() {
    FILE *fp = popen("pfctl -sm | awk '/table-entries/ { print $4 }'", "r");
    if (!fp) {
        perror("popen failed");
        return -1;
    }

    char result[128] = {0};
    int table_entries = 0;

    if (fgets(result, sizeof(result), fp)) {
        table_entries = atoi(result); // Convert the output to an integer
    } else {
        fprintf(stderr, "Failed to read table-entries value.\n");
        table_entries = -1;
    }

    pclose(fp);
    return table_entries;
}

// Function to set the table-entries value
void set_table_entries(const int *value) {
    char command[256];
    snprintf(command, sizeof(command), 
             "echo \"set limit { table-entries %d }\" | pfctl -f -", *value);

    if (system(command) != 0) {
        fprintf(stderr, "Failed to set table-entries to %d\n", *value);
    }
}

void run_quiet(const char *command) {
    int ret = system(command);
    if (ret == -1) {
        perror("system");
    } /*else if (WIFEXITED(ret) && WEXITSTATUS(ret) != 0) {
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(ret), command);
    }*/
}

int main(int argc, char *argv[]) {
    // Check if the program is being run as root
    if (geteuid() != 0) {
        fprintf(stderr, "This program must be run as root.\n");
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Parse the command-line argument
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

void start_function() {
    // Get the current value of table-entries
    int original_entries = get_table_entries();
    if (original_entries == -1) {
        fprintf(stderr, "Failed to retrieve the original table-entries value.\n");
        return;
    }

    // Save the original value to the temp file
    if (save_original_entries(&original_entries) != 0) {
        fprintf(stderr, "Failed to save the original table-entries value.\n");
        return;
    }

    // Set a new value for table-entries
    set_table_entries(&(const int){DEFAULT_TABLE_ENTRIES});

    printf("Starting: Loading PF tables...\n");
    if (loader_main("start") != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Failed to load PF tables.\n");
    }
}

void stop_function() {
    // Load the original value from the temp file
    int original_entries;
    if (load_original_entries(&original_entries) != 0) {
        fprintf(stderr, "Failed to load the original table-entries value.\n");
        return;
    }

    // Restore the original table-entries value
    set_table_entries(&original_entries);

    printf("Stopping: Flushing PF tables...\n");
    if (loader_main("stop") != EXIT_SUCCESS) {
        fprintf(stderr, "Error: Failed to flush PF tables.\n");
    }
}

void update_function() {
    printf("Retrieving updates and reloading. Please wait...\n");
    retrieve_main();
    system("pfpb stop >/dev/null");
    system("pfpb start >/dev/null");
    printf("Updating is complete.\n");
    pfcount_main();
}
