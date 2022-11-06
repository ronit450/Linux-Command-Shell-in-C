#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "parse.c"

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
                for (int j = 0; j < total_jobs - 1; j++)
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
    while (1)
    {
        char current_working_dir[256];
        // <Display prompt>

        getcwd(current_working_dir, sizeof(current_working_dir));
        printf("%s -> ", current_working_dir);
        // <Input command>
        // <Parse the command>
        char command[50];
        scanf("%[^\n]%*c", command);
        parseInfo *command_details = parse(command);

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
                for (int i = 0; i < total_jobs; i++)
                {
                    printf("Local Id of Process %d \n Process Id of Process %d \n ", all_jobs[i].Local_ID, all_jobs[i].pid);
                }
            }
            if (strcmp(command_details->CommArray[0].command, "cd") == 0)
            {
                chdir(command_details->CommArray[0].VarList[0]);
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
                     "\n>help - prints the list of builtin commands along with their description.\n");
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
                    exit(0);
                }
                else
                {
                    printf("There are some background jobs pending \n");
                }
            }
            else{

            int pid = fork();
            /* From this point on, there are two instances of the program */
            /* pid takes different values in the parent and child processes */

            if (pid == 0)
            {
                /* Execute the command in child process */
                int in = open(command_details->inFile, O_RDONLY);
                dup2(in, STDIN_FILENO);
                close(in);
                int out = open(command_details->outFile, O_WRONLY | O_CREAT, 0666); // Should also be symbolic values for access rights
                dup2(out, STDOUT_FILENO);
                close(out);
                execvp(command_details->CommArray[0].command, command_details->CommArray[0].VarList);
                int pid = getpid();
            }
            else
            {
                if (command_details->boolBackground == 1)
                {
                    all_jobs[total_jobs].pid = pid;
                    all_jobs[total_jobs].Local_ID = total_jobs;
                    printf("%d PID\n", all_jobs[total_jobs].pid);
                    total_jobs = total_jobs + 1;
                    printf("%d total jobs\n", total_jobs);
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
            printf("Job kam kar bhai \n");
        }
    }

    return 0;
}