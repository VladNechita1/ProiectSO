#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>

#define MAX_STR 64

struct Treasure {
    char id[MAX_STR];
    char username[MAX_STR];
    float latitude;
  float longitude;
  char clue[MAX_STR];
  int value;
};

volatile sig_atomic_t terminate_requested = 0;

void safe_print(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vdprintf(STDOUT_FILENO, format, args);
  va_end(args);
}

void list_hunts()
{
  DIR *dir = opendir(".");
  if (!dir)
    {
      safe_print("[Monitor] Cannot open current directory\n");
      return;
    }

  struct dirent *entry;
  while ((entry = readdir(dir)))
    {
      if (entry->d_type == DT_DIR && strncmp(entry->d_name, "game", 4) == 0)
	{
	  char path[512];
	  snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
	  int count = 0;

	  int fd = open(path, O_RDONLY);
	  if (fd >= 0)
	    {
	      struct Treasure t;
	      while (read(fd, &t, sizeof(t)) == sizeof(t))
		{
		  count++;
		}
	      close(fd);
	    }
	  safe_print("[Monitor] Hunt: %s | Treasures: %d\n", entry->d_name, count);
	}
    }
  closedir(dir);
}

void list_treasures(const char *hunt_id)
{
  char path[256];
  snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

  int fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      safe_print("[Monitor] Cannot open treasures file\n");
      return;
    }

  struct Treasure t;
  while (read(fd, &t, sizeof(t)) == sizeof(t))
    {
      safe_print("ID: %s | User: %s | Lat: %.2f | Lon: %.2f | Clue: %s | Value: %d\n",
		 t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }

  close(fd);
}

void view_treasure(const char *hunt_id, const char *treasure_id)
{
  char path[256];
  snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

  int fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      safe_print("[Monitor] Cannot open treasures file\n");
      return;
    }

  struct Treasure t;
  while (read(fd, &t, sizeof(t)) == sizeof(t))
    {
      if (strcmp(t.id, treasure_id) == 0)
	{
	  safe_print("Found Treasure:\nID: %s | User: %s | Lat: %.2f | Lon: %.2f | Clue: %s | Value: %d\n",
		     t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
	  close(fd);
	  return;
        }
    }

  safe_print("[Monitor] Treasure %s not found in hunt %s.\n", treasure_id, hunt_id);
  close(fd);
}

void handle_command(const char *cmd_line)
{
  char cmd[256];
  strncpy(cmd, cmd_line, sizeof(cmd));
  cmd[sizeof(cmd) - 1] = '\0';

  char *token = strtok(cmd, " ");

  if (strcmp(token, "list_hunts") == 0)
    {
      list_hunts();
    }
  else if (strcmp(token, "list_treasures") == 0)
    {
      char *hunt_id = strtok(NULL, " ");
      if (!hunt_id)
	{
	  safe_print("[Monitor] Error: missing hunt ID.\n");
	  return;
        }
      list_treasures(hunt_id);
    }
  else if (strcmp(token, "view_treasure") == 0)
    {
      char *hunt_id = strtok(NULL, " ");
      char *treasure_id = strtok(NULL, " ");
      if (!hunt_id || !treasure_id)
	{
	  safe_print("[Monitor] Error: missing arguments.\n");
	  return;
	}
      view_treasure(hunt_id, treasure_id);
    }
  else
    {
      safe_print("[Monitor] Unknown command: %s\n", token);
    }
}

void sigusr1_handler(int sig)
{
  FILE *f = fopen("monitor_command.txt", "r");
  if (!f)
    {
      safe_print("[Monitor] Cannot read monitor_command.txt\n");
      return;
    }

  char command[256];
  if (fgets(command, sizeof(command), f))
    {
      command[strcspn(command, "\n")] = 0;
      handle_command(command);
    }

  fclose(f);
}

void sigterm_handler(int sig)
{
  safe_print("[Monitor] SIGTERM received. Exiting in 2 seconds...\n");
  usleep(2); // 2 seconds delay
  terminate_requested = 1;
}

int main(void)
{
  struct sigaction sa1, sa2;

  sa1.sa_handler = sigusr1_handler;
  sigemptyset(&sa1.sa_mask);
  sa1.sa_flags = SA_RESTART;
  sigaction(SIGUSR1, &sa1, NULL);

  sa2.sa_handler = sigterm_handler;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &sa2, NULL);

  safe_print("[Monitor] Ready. Waiting for commands...\n");

  while (!terminate_requested)
    {
      pause();
    }

  safe_print("[Monitor] Monitor exiting gracefully.\n");
  return 0;
}
