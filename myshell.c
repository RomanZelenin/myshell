#include <unistd.h>
#include <stdio.h>
#include <pty.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

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

char const *trim(char *str)
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

void exec_command(char const *command, const char **env)
{
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
            char *name = strtok(command, " ");
            const size_t MAX_COUNT_ARG = 20;
            char **arguments = (char **)calloc(MAX_COUNT_ARG + 1, sizeof(char *));
            size_t index = 0;

            char *token = strtok(NULL, " ");
            while (token && index < MAX_COUNT_ARG)
            {
                arguments[index++] = token;
                token = strtok(NULL, " ");
            }
            arguments[index] = NULL;

            if (strcmp(name, "cd") == 0)
            {
                if (index > 1)
                {
                    printf("cd: too many arguments\n");
                }
                else if (index == 1)
                {
                    char *path = arguments[0];
                    if (path[0] == '~')
                    {
                        const char *home_path = getenv("HOME");
                        size_t len_path = strlen(path + 1) + strlen(home_path) + 1;
                        char *buff_path = (char *)calloc(len_path, 1);
                        strcat(buff_path, home_path);
                        strcat(buff_path, path + 1);
                        path = buff_path;
                    }

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
                    if (path != arguments[0])
                    {
                        free(path);
                    }
                    free(arguments);
                }
            }
            else if (strcmp(name, "ls") == 0)
            {
                pid_t pid = fork();
                if (pid == 0)
                {
                    switch (index)
                    {
                    case 0:
                        execl("/bin/ls", "/bin/ls", "./", arguments[index]);
                        break;
                    case 1:
                        execl("/bin/ls", "/bin/ls", arguments[0], arguments[index]);
                        break;
                    case 2:
                        execl("/bin/ls", "/bin/ls", arguments[0], arguments[1], arguments[index]);
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    int stat_lock;
                    wait(&stat_lock);
                }
            }
            else
            {
                printf("%s: command not found\n", command);
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
        read(STDIN_FILENO, buffer, 255);
        exec_command(trim(buffer), env);
    }

    return 0;
}
