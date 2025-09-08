#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

void* loading_animation(void* arg);
void print_help(const char *program_name);
char* run_command(const char* command, const char* loading_message);
char* get_home_directory();
char* get_current_branch();
int check_uncommitted_changes();
int check_unpushed_commits(const char* branch);
int check_changes_to_pull(const char* branch);
int check_remote_branch_exists(const char* branch);
char* replace_home_with_tilde(const char* path, const char* home_dir);
void display_git_status(const char* path, int show_branch, int show_path);
int is_git_repository(const char *path);
void scan_directories(const char *base_path, int show_branch, int show_path);

volatile int loading_done = 0;

int main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "help") == 0)
    {
        print_help(argv[0]);
        exit(EXIT_SUCCESS);
    }

    int opt;
    int show_branch = 0;
    int show_path = 0;
    char *directory = getcwd(NULL, 0);
    if (directory == NULL)
    {
        fprintf(stderr, "Failed to get current directory\n");
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "bpd:")) != -1)
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
                        return EXIT_FAILURE;
                    }
                    directory = strdup(resolved_path);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-b] [-p] [-d directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc)
    {
        fprintf(stderr, "git-check: unexpected argument: '%s'\n", argv[optind]);
        fprintf(stderr, "Usage: %s [-b] [-p] [-d directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (is_git_repository(directory))
    {
        display_git_status(directory, show_branch, show_path);
    }

    scan_directories(directory, show_branch, show_path);

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

void print_help(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-b] [-p] [-d directory]\n", program_name);
    fprintf(stderr, "  -b            Show branches\n");
    fprintf(stderr, "  -p            Show path instead of directory name\n");
    fprintf(stderr, "  -d directory  Specify directory to scan\n");
    fprintf(stderr, "  help          Show this help message\n");
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

    char* result = malloc(1024);
    FILE* file_pointer;
    if ((file_pointer = popen(command, "r")) == NULL)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    if (fgets(result, 1024, file_pointer) == NULL)
    {
        result[0] = '\0';
    }

    pclose(file_pointer);

    loading_done = 1;

    if (pthread_join(animation_thread, NULL) != 0)
    {
        perror("Failed to join animation thread");
        return NULL;
    }

    printf("\r%s\r", loading_message);
    for (int i = 0; i < strlen(loading_message) + 2; i++)
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

char* get_current_branch()
{
    char* current_branch = run_command("git rev-parse --abbrev-ref HEAD", "Getting current branch...");
    current_branch[strcspn(current_branch, "\n")] = '\0';
    return current_branch;
}

int check_uncommitted_changes()
{
    char* has_uncommitted_changes = run_command("git status --porcelain", "Checking for uncommitted changes...");
    int result = strlen(has_uncommitted_changes) > 0;
    free(has_uncommitted_changes);
    return result;
}

int check_unpushed_commits(const char* branch)
{
    char command[1024];
    snprintf(command, sizeof(command), "git log origin/%s..HEAD", branch);
    char* has_unpushed_commits = run_command(command, "Checking for unpushed commits...");
    int result = strlen(has_unpushed_commits) > 0;
    free(has_unpushed_commits);
    return result;
}

int check_changes_to_pull(const char* branch)
{
    run_command("git fetch > /dev/null 2>&1", "Fetching changes from remote repository...");
    char command[1024];
    snprintf(command, sizeof(command), "git log HEAD..origin/%s", branch);
    char* has_changes_to_pull = run_command(command, "Checking for changes to pull...");
    int result = strlen(has_changes_to_pull) > 0;
    free(has_changes_to_pull);
    return result;
}

int check_remote_branch_exists(const char* branch)
{
    char command[1024];
    snprintf(command, sizeof(command), "git ls-remote --heads origin %s", branch);
    char* result = run_command(command, "Checking if remote branch exists...");
    int exists = strlen(result) > 0;
    free(result);
    return exists;
}

char* replace_home_with_tilde(const char* path, const char* home_dir)
{
    if (strncmp(path, home_dir, strlen(home_dir)) == 0)
    {
        size_t new_path_len = strlen(path) - strlen(home_dir) + 2;
        char* new_path = malloc(new_path_len);
        snprintf(new_path, new_path_len, "~%s", path + strlen(home_dir));
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

    char original_cwd[1024];
    getcwd(original_cwd, sizeof(original_cwd));

    chdir(path);

    int has_uncommitted_changes = check_uncommitted_changes();
    char* current_branch = get_current_branch();
    char* path_with_tilde = replace_home_with_tilde(path, get_home_directory());

    int has_unpushed_commits = 0;
    int has_changes_to_pull = 0;

    if (check_remote_branch_exists(current_branch))
    {
        has_unpushed_commits = check_unpushed_commits(current_branch);
        has_changes_to_pull = check_changes_to_pull(current_branch);
    }

    chdir(original_cwd);

    printf("%s%s%s: ", BOLD, show_path ? path_with_tilde : folder_name, RESET);
    if (show_branch)
    {
        printf("%s<%s>%s ", YELLOW BOLD, current_branch, RESET);
    }
    printf("[%s%s%s] ", has_uncommitted_changes ? RED BOLD "Changes" : GREEN BOLD "No Changes", RESET, RESET);
    printf("[%s%s%s] ", has_unpushed_commits ? RED BOLD "Unpushed" : GREEN BOLD "Pushed", RESET, RESET);
    printf("[%s%s%s]\n", has_changes_to_pull ? RED BOLD "Pull" : GREEN BOLD "Up-to-date", RESET, RESET);

    free(cwd);
    free(current_branch);
}

int is_git_repository(const char *path)
{
    struct stat st;
    char git_path[1024];

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
    char path[1024];

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
