#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define PROGRAM_NAME "git check"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

#define BUFFER_SIZE 1024
#define COMMAND_SIZE 1024
#define PATH_SIZE 1024

#define STATUS_SUCCESS 0
#define STATUS_ERROR 1
#define STATUS_WARNING 2

void* loading_animation(void* arg);
void print_help();
char* run_command(const char* command, const char* loading_message);
char* get_home_directory();
char* get_current_branch();
int check_uncommitted_changes();
int check_unpushed_commits(const char* branch);
int check_remote_status(const char* branch, int* has_changes_to_pull, int* has_remote_branch);
char* replace_home_with_tilde(const char* path, const char* home_dir);
void display_git_status(const char* path, int show_branch, int show_path);
int is_git_repository(const char *path);
void scan_directories(const char *base_path, int show_branch, int show_path);
void print_status(const char* text, int status_type);
int run_git_check(const char* command, const char* loading_message);

volatile int loading_done = 0;

int main(int argc, char *argv[])
{
    int opt;
    int show_branch = 0;
    int show_path = 0;
    char *directory = getcwd(NULL, 0);
    if (directory == NULL)
    {
        fprintf(stderr, "Failed to get current directory\n");
        return EXIT_FAILURE;
    }

    static struct option long_options[] = {
        {"branch", no_argument, 0, 'b'},
        {"path", no_argument, 0, 'p'},
        {"directory", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "bpd:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'b':
                show_branch = 1;
                break;
            case 'p':
                show_path = 1;
                break;
            case 'd':
            	{
                    char resolved_path[PATH_MAX];
                    if (realpath(optarg, resolved_path) == NULL)
                    {
                        perror("git-check");
                        free(directory);
                        return EXIT_FAILURE;
                    }
                    free(directory);
                    directory = strdup(resolved_path);
                }
                break;
            case 'h':
                print_help();
                free(directory);
                exit(EXIT_SUCCESS);
            default:
                print_help();
                free(directory);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
    {
        fprintf(stderr, "git-check: unexpected argument: '%s'\n", argv[optind]);
        print_help();
        free(directory);
        exit(EXIT_FAILURE);
    }

    if (is_git_repository(directory))
    {
        display_git_status(directory, show_branch, show_path);
    }

    scan_directories(directory, show_branch, show_path);

    free(directory);
    return 0;
}

void* loading_animation(void* arg)
{
    const char *message = (const char*)arg;
    const char *animation_chars = "|/-\\";
    int animation_length = 4;
    int i = 0;

    while (!loading_done)
    {
        printf("\r%s %c", message, animation_chars[i]);
        fflush(stdout);
        usleep(100000);

        i = (i + 1) % animation_length;
    }

    return NULL;
}

void print_help()
{
    fprintf(stderr, "usage: %s [<options>] [--]\n\n", PROGRAM_NAME);
    fprintf(stderr, "    -b, --branch           show current branch\n");
    fprintf(stderr, "    -p, --path             show full path instead of just directory name\n");
    fprintf(stderr, "    -d, --directory <dir>  specify directory to scan (default: current directory)\n");
    fprintf(stderr, "    -h, --help             show this help message\n");
}

char* run_command(const char* command, const char* loading_message)
{
    pthread_t animation_thread;
    loading_done = 0;

    if (pthread_create(&animation_thread, NULL, loading_animation, (void*)loading_message) != 0)
    {
        perror("Failed to create animation thread");
        return NULL;
    }

    char* result = malloc(BUFFER_SIZE);
    if (result == NULL)
    {
        perror("malloc");
        loading_done = 1;
        pthread_join(animation_thread, NULL);
        return NULL;
    }

    FILE* file_pointer;
    if ((file_pointer = popen(command, "r")) == NULL)
    {
        perror("popen");
        free(result);
        loading_done = 1;
        pthread_join(animation_thread, NULL);
        return NULL;
    }

    if (fgets(result, BUFFER_SIZE, file_pointer) == NULL)
    {
        result[0] = '\0';
    }

    pclose(file_pointer);

    loading_done = 1;

    if (pthread_join(animation_thread, NULL) != 0)
    {
        perror("Failed to join animation thread");
        free(result);
        return NULL;
    }

    printf("\r%s\r", loading_message);
    for (size_t i = 0; i < strlen(loading_message) + 2; i++)
    {
        printf(" ");
    }
    printf("\r");

    return result;
}

char *get_home_directory()
{
    char *home_dir = getenv("HOME");
    if (home_dir == NULL)
    {
        struct passwd *pw = getpwuid(getuid());
        home_dir = pw ? pw->pw_dir : NULL;
    }
    return home_dir;
}

void print_status(const char* text, int status_type)
{
    if (status_type == STATUS_ERROR) {
        printf("[%s%s%s] ", RED BOLD, text, RESET);
    } else if (status_type == STATUS_WARNING) {
        printf("[%s%s%s] ", YELLOW BOLD, text, RESET);
    } else {
        printf("[%s%s%s] ", GREEN BOLD, text, RESET);
    }
}

int run_git_check(const char* command, const char* loading_message)
{
    char* result = run_command(command, loading_message);
    int has_output = (result != NULL) && (strlen(result) > 0);
    free(result);
    return has_output;
}

char* get_current_branch()
{
    char* current_branch = run_command("git rev-parse --abbrev-ref HEAD 2>/dev/null", "Getting current branch...");
    if (current_branch != NULL) {
        current_branch[strcspn(current_branch, "\n")] = '\0';
    }
    return current_branch;
}

int check_uncommitted_changes()
{
    return run_git_check("git status --porcelain 2>/dev/null", "Checking for uncommitted changes...");
}

int check_unpushed_commits(const char* branch)
{
    char command[COMMAND_SIZE];
    snprintf(command, sizeof(command), "git log origin/%s..HEAD 2>/dev/null", branch);
    return run_git_check(command, "Checking for unpushed commits...");
}

int check_remote_status(const char* branch, int* has_changes_to_pull, int* has_remote_branch)
{
    *has_changes_to_pull = 0;
    *has_remote_branch = 0;

    char command[COMMAND_SIZE];
    snprintf(command, sizeof(command), "git ls-remote origin %s 2>/dev/null; echo \"EXIT_CODE:$?\"", branch);
    char* result = run_command(command, "Checking remote status...");

    if (!result) return 0;

    char* exit_code = strstr(result, "EXIT_CODE:");
    if (exit_code && atoi(exit_code + 10) != 0) {
        free(result);
        return 0;
    }
    if (exit_code) *exit_code = '\0';

    char* remote_hash = strtok(result, " \t\n");
    if (!remote_hash) {
        free(result);
        return 1;
    }

    *has_remote_branch = 1;

    char* local_hash = run_command("git rev-parse HEAD 2>/dev/null", "Checking local commits...");
    if (!local_hash) {
        free(result);
        return 1;
    }

    local_hash[strcspn(local_hash, "\n")] = '\0';

    snprintf(command, sizeof(command), "git merge-base HEAD %s 2>/dev/null", remote_hash);
    char* merge_base = run_command(command, "Checking if behind remote...");

    if (merge_base) {
        merge_base[strcspn(merge_base, "\n")] = '\0';
        *has_changes_to_pull = (strcmp(merge_base, local_hash) == 0 && strcmp(local_hash, remote_hash) != 0);
        free(merge_base);
    }

    free(result);
    free(local_hash);
    return 1;
}

char* replace_home_with_tilde(const char* path, const char* home_dir)
{
    if (home_dir != NULL && strncmp(path, home_dir, strlen(home_dir)) == 0)
    {
        size_t new_path_len = strlen(path) - strlen(home_dir) + 2;
        char* new_path = malloc(new_path_len);
        if (new_path != NULL) {
            snprintf(new_path, new_path_len, "~%s", path + strlen(home_dir));
        }
        return new_path;
    }
    else
    {
        return strdup(path);
    }
}

void display_git_status(const char* path, int show_branch, int show_path)
{
    char* cwd = strdup(path);
    char* folder_name = strrchr(cwd, '/') + 1;

    char original_cwd[PATH_SIZE];
    getcwd(original_cwd, sizeof(original_cwd));

    chdir(path);

    char* current_branch = get_current_branch();
    int has_uncommitted_changes = check_uncommitted_changes();
    int has_unpushed_commits = check_unpushed_commits(current_branch);
    char* path_with_tilde = replace_home_with_tilde(path, get_home_directory());

    int has_changes_to_pull = 0;
    int has_remote_branch = 0;
    int has_connection = check_remote_status(current_branch, &has_changes_to_pull, &has_remote_branch);

    chdir(original_cwd);

    if (show_path) {
        printf("%s%s%s: ", BOLD, path_with_tilde, RESET);
    } else {
        printf("%s%s%s: ", BOLD, folder_name, RESET);
    }

    if (show_branch) {
        printf("%s<%s>%s ", YELLOW BOLD, current_branch, RESET);
    }

    if (has_uncommitted_changes) {
        print_status("Changes", STATUS_ERROR);
    } else {
        print_status("Clean", STATUS_SUCCESS);
    }

    if (has_unpushed_commits) {
        print_status("Unpushed", STATUS_ERROR);
    } else {
        print_status("Synced", STATUS_SUCCESS);
    }

    if (has_connection && has_remote_branch) {
        if (has_changes_to_pull) {
            print_status("Behind", STATUS_ERROR);
        } else {
            print_status("Updated", STATUS_SUCCESS);
        }
    } else if (has_connection) {
        print_status("No Remote", STATUS_WARNING);
    } else {
        print_status("Offline", STATUS_WARNING);
    }
    printf("\n");

    free(cwd);
    free(current_branch);
    free(path_with_tilde);
}

int is_git_repository(const char *path)
{
    struct stat st;
    char git_path[PATH_SIZE];

    snprintf(git_path, sizeof(git_path), "%s/.git", path);
    if (stat(git_path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        return 1;
    }
    return 0;
}

void scan_directories(const char *base_path, int show_branch, int show_path)
{
    DIR *dir;
    struct dirent *entry;
    char path[PATH_SIZE];

    if ((dir = opendir(base_path)) == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

            if (is_git_repository(path))
            {
                display_git_status(path, show_branch, show_path);
            }

            scan_directories(path, show_branch, show_path);
        }
    }

    closedir(dir);
}
