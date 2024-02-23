#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <readline/readline.h>

#define OLD_STDOUT 5

static char *HOME_PATH_SYM = "~";

typedef enum bool
{
    FALSE,
    TRUE
} bool;

void clear_buffer(char *buff, size_t n)
{

    for (size_t i = 0; i < n; i++)
    {
        buff[i] = '\0';
    }
}

char *trim(char *str)
{
    int s = 0;
    while (s < strlen(str) - 1 && (str[s] == ' '))
        s++;

    int e = (int)strlen(str) - 1;
    while (e >= s && e >= 0 && (str[e] == '\n' || str[e] == ' ' || str[e] == '\r'))
        e--;

    if (e >= s)
    {
        str[e + 1] = '\0';
        return str + s;
    }
    return NULL;
}

void display_prompt()
{
    char username[256];
    cuserid(username);

    char *pwd = getcwd(NULL, 0);
    const char *home_path = getenv("HOME");
    char hostname[255];
    gethostname(hostname, 255);

    int len_home_path = strlen(home_path);
    if (strncmp(pwd, home_path, len_home_path) == 0)
    {
        size_t tmp_size = strlen(pwd) - len_home_path + 2;
        char *tmp = (char *)malloc(tmp_size);
        tmp[0] = '~';
        strncpy(tmp + 1, pwd + len_home_path, tmp_size - 1);
        free(pwd);
        pwd = tmp;
    }

    if (getuid() == 0)
    {
        printf("%s@%s:%s# ", username, hostname, pwd);
    }
    else
    {
        printf("\033[01;32m%s@%s\033[00m:%s$ ", username, hostname, pwd);
    }

    free(pwd);
}

char *find_command(char *command)
{
    char *result = NULL;
    const char *PATH = getenv("PATH");
    char *paths = malloc(strlen(PATH));
    strcpy(paths, PATH);

    if (paths != NULL)
    {
        char *path = strtok(paths, ":");
        while (path != NULL && result == NULL)
        {
            DIR *dir = opendir(path);
            if (dir != NULL)
            {
                struct dirent *entry = readdir(dir);
                while (entry != NULL)
                {
                    if (strcmp(command, entry->d_name) == 0)
                    {
                        char *result_path = (char *)malloc(strlen(entry->d_name) + strlen(command) + 2);
                        strcpy(result_path, path);
                        if (path[strlen(path) - 1] != '/')
                        {
                            result_path[strlen(path)] = '/';
                            result_path[strlen(path) + 1] = '\0';
                        }
                        strcat(result_path, command);
                        result = result_path;
                        break;
                    }
                    entry = readdir(dir);
                }
                closedir(dir);
            }
            path = strtok(NULL, ":");
        }
        free(paths);
    }
    return result;
}

char *get_next_command()
{
    return NULL;
}

void exec_command(char *command)
{
    command = trim(command);
    if (command != NULL)
    {
        if (strcmp(command, "exit") == 0)
        {
            exit(0);
        }
        else if (strcmp(command, "clear") == 0)
        {
            printf("\033[1;1H");
            printf("\033[2J");
            printf("\033[3J");
        }
        else
        {
            const size_t MAX_COUNT_ARG = 20;
            char **arguments = (char **)calloc(MAX_COUNT_ARG + 1, sizeof(char *));
            arguments[0] = strtok(command, " ");
            char *command_name = arguments[0];

            char *token = strtok(NULL, " ");
            size_t index = 1;
            while (token && index < MAX_COUNT_ARG)
            {
                arguments[index++] = token;
                token = strtok(NULL, " ");
            }
            arguments[index] = NULL;

            if (strcmp(command_name, "cd") == 0)
            {
                const char *OLDPWD = getenv("OLD_PWD");
                if (index > 2)
                {
                    printf("cd: too many arguments\n");
                }
                else if (index >= 1 && index <= 2)
                {
                    const char *path = index == 1 ? HOME_PATH_SYM : arguments[1];

                    if (path == HOME_PATH_SYM || path[0] == '~')
                    {
                        const char *home_path = getenv("HOME");
                        size_t len_path = strlen(path + 1) + strlen(home_path) + 1;
                        char *buff_path = (char *)calloc(len_path, 1);
                        strcat(buff_path, home_path);
                        strcat(buff_path, path + 1);
                        path = buff_path;
                    }
                    else if (index == 2 && strlen(arguments[1]) == 1 && arguments[1][0] == '-')
                    {
                        if (OLDPWD != NULL)
                        {
                            path = OLDPWD;
                        }
                        else
                        {
                            printf("cd: OLD_PWD not set\n");
                            path = NULL;
                        }
                    }

                    if (path != NULL)
                    {
                        char *cwd = getcwd(NULL, 0);
                        char *tmp = calloc(strlen(cwd) + strlen("OLD_PWD=") + 1, 1);
                        strcat(tmp, "OLD_PWD=");
                        strcat(tmp, cwd);
                        putenv(tmp);

                        int res = chdir(path);
                        if (res != -1)
                        {
                            switch (res)
                            {
                            case ENOTDIR:
                                printf("cd: Not a directory\n");
                                break;
                            case ENOENT:
                                printf("No such file or directory\n");
                                break;
                            default:
                                break;
                            }
                        }
                        else
                        {
                            printf("cd: Not a directory\n");
                        }
                        if (path != arguments[1] && path != HOME_PATH_SYM && path != OLDPWD)
                        {
                            free(path);
                        }
                    }

                    free(arguments);
                }
            }
            else
            {
                while (command_name != NULL)
                {
                    char *command_path = find_command(command_name);
                    if (command_path != NULL)
                    {
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            /* dup2(STDOUT_FILENO, OLD_STDOUT);
                            int f = open("res.txt", O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
                            dup2(f, STDOUT_FILENO); */
                            execv(command_path, arguments);
                        }
                        else
                        {
                            int stat_lock;
                            wait(&stat_lock);
                            free(command_path);
                            // dup2(OLD_STDOUT, STDOUT_FILENO);
                        }
                    }
                    else
                    {
                        printf("%s: command not found\n", command_name);
                    }

                    command_name = get_next_command();
                }
            }
        }
    }
}

int main(int argc, char const *argv[], char const *env[])
{
    const size_t BUFF_SIZE = 255;
    char buffer[BUFF_SIZE];

    setbuf(stdout, NULL);
    while (TRUE)
    {
        display_prompt();
        clear_buffer(buffer, BUFF_SIZE);
        // char* input = readline(NULL);
        read(STDIN_FILENO, buffer, 255);
        exec_command(buffer);
        // free(input);
    }

    return 0;
}
