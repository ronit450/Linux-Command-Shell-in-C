#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "parse.c"
#include <dirent.h>
#include <errno.h>

int execl(const char *path, const char *arg, ...);

typedef struct
{
    /* data */
    pid_t pid;
    int Local_ID;

} background_process;

volatile int total_jobs = 0;
volatile background_process all_jobs[10];
void signal_handler(int signal)
{
    int *status;
    pid_t kidpid;

    if ((kidpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child %ld terminated\n", kidpid);
        for (int i = 0; i < total_jobs; i++)
        {
            if (all_jobs[i].pid == kidpid)
            {
                for (int j = i; j < total_jobs - 1; j++)
                {
                    all_jobs[j] = all_jobs[j + 1];
                }
            }
            total_jobs = total_jobs - 1;
        }
    }
}

int main()
{
    struct node *head = NULL;
    parseInfo *history[10];
    int history_total = 0;
    printf("Welcome to Kataria Shell Where you can find everything\n \n \n");
    while (1)
    {
        char current_working_dir[256];
        // <Display prompt>

        getcwd(current_working_dir, sizeof(current_working_dir));

        printf("%s -> ", current_working_dir);
        char command[50];
        scanf("%[^\n]%*c", command);
        parseInfo *command_details = parse(command);

        if (command_details->pipeNum > 0)
        {

            //    Making two commands, one before | and one after it

            struct commandType *command_1 = &(command_details->CommArray[0]);
            struct commandType *command_2 = &(command_details->CommArray[1]);

            char *main_parsed[command_1->VarNum + 1];
            char *Output_from_pipe[command_2->VarNum + 1];

            for (int i = 0; i < command_1->VarNum; i++)
            {
                main_parsed[i] = strdup(command_1->VarList[i]);
            }

            for (int i = 0; i < command_2->VarNum; i++)
            {
                Output_from_pipe[i] = strdup(command_2->VarList[i]);
            }

            main_parsed[command_1->VarNum] = NULL;
            Output_from_pipe[command_2->VarNum] = NULL;

            int pd[2];

            if (pipe(pd) == -1)
            {
                printf("Pipe cannot be established \n");
                exit(1);
            }

            if (fork() == 0)
            {

                close(pd[0]);
                dup2(pd[1], 1);
                close(pd[0]);
                close(pd[1]);

                if (execvp(main_parsed[0], main_parsed) < 0)
                {
                    printf("command 1 cannot be executed\n");
                    exit(0);
                }
            }

            if (fork() == 0)
            {

                close(pd[1]);
                dup2(pd[0], 0);
                close(pd[1]);
                close(pd[0]);

                if (execvp(Output_from_pipe[0], Output_from_pipe) < 0)
                {
                    printf("command 2 cannot be executed\n");
                }

                exit(0);
            }

            close(pd[0]);
            close(pd[1]);
        }
        else
        {
            if (total_jobs < 10 || command_details->boolBackground == 0)
            {
                if (history_total < 10)
                {
                    history[history_total] = command_details;
                    history_total += 1;
                }
                else
                {
                    for (int j = 0; j < history_total - 1; j++)
                    {
                        history[j] = history[j + 1];
                    }

                    history[history_total - 1] = command_details;
                }

                // Builtin Commands:

                if (strcmp(command_details->CommArray[0].command, "jobs") == 0)
                {
                    if (total_jobs == 0)
                    {
                        printf("No Backgrround Job");
                    }
                    else
                    {

                        for (int i = 0; i < total_jobs; i++)
                        {
                            printf("Local Id of Process %d \n Process Id of Process %d \n ", all_jobs[i].Local_ID, all_jobs[i].pid);
                        }
                    }
                }
                if (strcmp(command_details->CommArray[0].command, "cd") == 0)
                {
                    DIR *dir = opendir(command_details->CommArray[0].VarList[0]);
                    if (dir)
                    {
                        chdir(command_details->CommArray[0].VarList[0]);
                        closedir(dir);
                    }
                    else
                    {
                        printf("Given Path does not exisit \n");
                    }
                }
                if (strcmp(command_details->CommArray[0].command, "history") == 0)
                {
                    for (int i = 0; i < history_total; i++)
                    {
                        printf("Last command executed  %s \n ", history[i]->CommArray[0].command);
                    }
                }

                if (strcmp(command_details->CommArray[0].command, "kill") == 0)
                {
                    for (int i = 0; i < total_jobs; i++)
                    {
                        if (all_jobs[i].pid == *command_details->CommArray->VarList[0])
                        {
                            kill(all_jobs[i].pid, SIGKILL);
                            for (int j = i; j < total_jobs - 1; j++)
                            {
                                all_jobs[j] = all_jobs[j + 1];
                            }
                        }
                        total_jobs = total_jobs - 1;
                    }
                }
                if (strcmp(command_details->CommArray[0].command, "help") == 0)
                {
                    printf(
                        "\nList of Commands supported:"
                        "\n>jobs - provides a list of all background processes and their local pIDs."
                        "\n>cd PATHNAME - sets the PATHNAME as working directory."
                        "\n>history - prints a list of previously executed commands."
                        "\n>kill PID - terminates the background process identified locally with PID in the jobs list."
                        "\n>!CMD - runs the command numbered CMD in the command history."
                        "\n>exit - terminates the shell only if there are no background jobs."
                        "\n>help - prints the list of builtin commands along with their description. \n");
                }

                if (strcmp(&(command_details->CommArray[0].command[0]), "!") == 0)
                {
                    int history_number = atoi(&(command_details->CommArray[0].command[1]));
                    execvp(history[history_number - 1]->CommArray->command, history[history_number - 1]->CommArray[0].VarList);
                }
                if (strcmp((command_details->CommArray[0].command), "exit") == 0)
                {
                    if (total_jobs == 0)
                    {
                        printf("Shell will be closed now Good Bye :) \n");
                        sleep(2);
                        exit(1);
                    }
                    else
                    {
                        printf("There are some background jobs pending \n");
                    }
                }
                else
                {

                    int pid = fork();
                    /* From this point on, there are two instances of the program */
                    /* pid takes different values in the parent and child processes */

                    if (pid == 0)
                    {
                        /* Execute the command in child process */
                        if ((command_details->boolInfile == 0 && command_details->boolOutfile == 0))
                        {

                            execvp(command_details->CommArray[0].command, command_details->CommArray[0].VarList);
                        }
                        else if (command_details->boolInfile == 1 && command_details->boolOutfile == 1)
                        {
                            int in = open(command_details->inFile, O_RDONLY);
                            dup2(in, STDIN_FILENO);
                            close(in);
                            int out = open(command_details->outFile, O_WRONLY | O_CREAT, 0666); // Should also be symbolic values for access rights
                            dup2(out, STDOUT_FILENO);
                            close(out);
                            execvp(command_details->CommArray[0].command, command_details->CommArray[0].VarList);
                        }
                        else if (command_details->boolInfile == 1 && command_details->boolOutfile == 0)
                        {
                            int in = open(command_details->inFile, O_RDONLY);
                            dup2(in, STDIN_FILENO);
                            close(in);
                            execvp(command_details->CommArray[0].command, command_details->CommArray[0].VarList);
                        }
                        else if (command_details->boolInfile == 0 && command_details->boolOutfile == 1)
                        {
                            int out = open(command_details->outFile, O_WRONLY | O_CREAT, 0666); // Should also be symbolic values for access rights
                            dup2(out, STDOUT_FILENO);
                            close(out);
                            execvp(command_details->CommArray[0].command, command_details->CommArray[0].VarList);
                        }
                    }
                    else
                    {
                        if (command_details->boolBackground == 1)
                        {
                            all_jobs[total_jobs].pid = pid;
                            all_jobs[total_jobs].Local_ID = total_jobs;
                            total_jobs = total_jobs + 1;
                            signal(SIGCHLD, signal_handler);
                        }
                        /* Wait for child process to terminate */
                        if (command_details->boolBackground == 0)
                        {
                            waitpid(pid);
                        }
                    }
                }
            }
            else
            {
                printf("Backgroud Job Limit Exceeded \n");
            }
        }
    }

    return 0;
}