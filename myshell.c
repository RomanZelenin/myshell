#include <unistd.h>
#include <stdio.h>
#include <pty.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

typedef enum bool
{
    FALSE,
    TRUE
} bool;

char const *get_env_value(char const *env[], char const *name)
{
    int numEnvItems = 0;
    char const *value = NULL;
    int nameLength = strlen(name);

    while (env[numEnvItems] != NULL)
    {
        int len = (int)(strchr(env[numEnvItems], '=') - env[numEnvItems]);
        if (nameLength == len && strncmp(name, env[numEnvItems], len) == 0)
        {
            value = env[numEnvItems] + len + 1;
            break;
        }
        numEnvItems++;
    }
    return value;
}

void clear_buffer(char *buff, size_t n)
{

    for (size_t i = 0; i < n; i++)
    {
        buff[i] = '\0';
    }
}

void display_prompt(const char **env)
{
    const char *username = get_env_value(env, "LOGNAME");
    const char *pwd = get_env_value(env, "PWD");
    const char *home_path = get_env_value(env, "HOME");
    const char *hostname = get_env_value(env, "NAME");

    int len_pwd = strlen(pwd);
    int len_home_path = strlen(home_path);

    if (len_pwd >= len_home_path && strncmp(pwd, home_path, MIN(len_pwd, len_home_path)) == 0)
    {
        size_t tmp_size = len_pwd - len_home_path + 2;
        char *tmp = (char *)malloc(tmp_size);
        tmp[0] = '~';
        strncpy(tmp + 1, pwd + len_home_path, tmp_size - 1);
        pwd = tmp;
    }

    if (strlen(username) == strlen("root"))
    {
        printf("%s@%s:%s# ", username, hostname, pwd);
    }
    else
    {
        printf("\033[01;32m%s@%s\033[00m:%s$ ", username, hostname, pwd);
    }
}

void exec_command(char const *command)
{
    if (command != NULL)
    {
        size_t len_command = strlen(command);
        if (len_command == strlen("exit") && strcmp(command, "exit") == 0)
        {
            exit(0);
        }
        else if (len_command == strlen("clear") && strcmp(command, "clear") == 0)
        {
            printf("\033[1;1H");
            printf("\033[2J");
            printf("\033[3J");
        }
        else
        {
            printf("%s: command not found\n", command);
        }
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

int main(int argc, char const *argv[], char const *env[])
{
    const size_t BUFF_SIZE = 255;
    char buffer[BUFF_SIZE];

    setbuf(stdout, NULL);
    while (TRUE)
    {
        display_prompt(env);
        clear_buffer(buffer, BUFF_SIZE);
        read(STDIN_FILENO, buffer, 255);
        exec_command(trim(buffer));
    }

    return 0;
}
