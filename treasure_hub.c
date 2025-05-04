#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

pid_t monitor_pid = -1;
int monitor_shutting_down = 0;

void sigchld_handler(int sig)
{
  int status;
  pid_t pid = waitpid(-1, &status, WNOHANG);
  if (pid == monitor_pid)
    {
      printf("[Hub] Monitor process has exited.\n");
      monitor_pid = -1;
      monitor_shutting_down = 0;
    }
}

int main(void)
{
  char input[256];

  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  printf("Welcome to the Treasure Hub!\n");

  while (1)
    {
      printf(">> ");

      if (!fgets(input, sizeof(input), stdin))
	{
	  break;
	}
      input[strcspn(input, "\n")] = '\0';

      if (monitor_shutting_down)
	{
	  printf("[Hub] Monitor is shutting down.\n");
	  continue;
	}

      if (strcmp(input, "start_monitor") == 0)
	{
	  if (monitor_pid > 0)
	    {
	      printf("[Hub] Monitor already running.\n");
	      continue;
	    }

	  pid_t pid = fork();
	  if (pid == 0)
	    {
	      // Child process becomes monitor
	      char *args[] = { "./treasure_monitor", NULL };
	      execv(args[0], args);
	      perror("[Hub] Failed to start monitor");
	      exit(1);
	    }
	  else if (pid > 0)
	    {
	      monitor_pid = pid;
	      printf("[Hub] Monitor started (PID %d).\n", monitor_pid);
	    }
	  else
	    {
	      perror("[Hub] fork failed");
	    }
	}

      else if (strcmp(input, "stop_monitor") == 0)
	{
	  if (monitor_pid <= 0)
	    {
	      printf("[Hub] No monitor is currently running.\n");
	      continue;
            }

	  kill(monitor_pid, SIGTERM);
	  monitor_shutting_down = 1;
	  printf("[Hub] Stop signal sent. Waiting for monitor to exit...\n");
        }

      else if (strcmp(input, "list_hunts") == 0 || strncmp(input, "list_treasures ", 15) == 0 || strncmp(input, "view_treasure ", 14) == 0)
	{
	  if (monitor_pid <= 0)
	    {
	      printf("[Hub] Error: Monitor is not running.\n");
	      continue;
	    }

	  FILE *f = fopen("monitor_command.txt", "w");
	  if (!f)
	    {
	      perror("[Hub] Failed to write to command file");
	      continue;
	    }

	  fprintf(f, "%s\n", input);
	  fclose(f);

	  kill(monitor_pid, SIGUSR1);
	  printf("[Hub] Sent command to monitor: %s\n", input);
        }

      else if (strcmp(input, "exit") == 0)
	{
	  if (monitor_pid > 0)
	    {
	      printf("[Hub] Cannot exit: Monitor still running. Use stop_monitor first.\n");
            }
	  else
	    {
	      printf("[Hub] Exiting hub.\n");
	      break;
            }
        }

      else
	{
	  printf("[Hub] Unknown command.\n");
        }
    }

  return 0;
}
