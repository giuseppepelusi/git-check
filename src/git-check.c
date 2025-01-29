#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

char* execute_command(const char* command)
{
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
    return result;
}

char *get_current_directory()
{
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL)
    {
        perror("getcwd");
    }
    return cwd;
}

int main() {
    char* cwd = get_current_directory();
    char* folder_name = strrchr(cwd, '/') + 1;

    char* uncommitted_changes = execute_command("git status --porcelain");
    int has_uncommitted_changes = strlen(uncommitted_changes) > 0;

    char* unpushed_commits = execute_command("git log origin/main..HEAD");
    int has_unpushed_commits = strlen(unpushed_commits) > 0;

    execute_command("git fetch");
    char* changes_to_pull = execute_command("git log HEAD..origin/main");
    int has_changes_to_pull = strlen(changes_to_pull) > 0;

    printf("%s%s%s: ", BOLD, folder_name, RESET);

    printf("[%s%s%s] ", has_uncommitted_changes ? RED BOLD "Changes" : GREEN BOLD "No Changes", RESET, RESET);
    printf("[%s%s%s] ", has_unpushed_commits ? RED BOLD "Unpushed" : GREEN BOLD "Pushed", RESET, RESET);
    printf("[%s%s%s]\n", has_changes_to_pull ? RED BOLD "Pull" : GREEN BOLD "Up-to-date", RESET, RESET);

    free(cwd);
    free(uncommitted_changes);
    free(unpushed_commits);
    free(changes_to_pull);

    return 0;
}
