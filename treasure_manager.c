#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define ID_SIZE 16
#define USERNAME_SIZE 32
#define CLUE_SIZE 128

typedef struct {
  char id[ID_SIZE];
  char username[USERNAME_SIZE];
  float latitude;
  float longitude;
  char clue[CLUE_SIZE];
  int value;
} Treasure;

int write_treasure(int fd, Treasure *treasure)
{
  if (write(fd, treasure, sizeof(Treasure)) != sizeof(Treasure))
    {
      perror("Error writing treasure");
      return -1;
    }
  return 0;
}

int read_treasure(int fd, Treasure *treasure)
{
  int bytes_read = read(fd, treasure, sizeof(Treasure));
  if (bytes_read == 0)
    {
      return 0;
    }
  else if (bytes_read != sizeof(Treasure))
    {
      perror("Error reading treasure");
      return -1;
    }
  return 1;
}

void get_hunt_dir_path(char *hunt_id, char *path, int path_size)
{
  snprintf(path, path_size, "hunts/%s", hunt_id);
}

void get_treasure_file_path(char *hunt_id, char *path, int path_size)
{
  snprintf(path, path_size, "hunts/%s/treasures.bin", hunt_id);
}

void get_log_file_path(char *hunt_id, char *path, int path_size)
{
  snprintf(path, path_size, "hunts/%s/logged_hunt", hunt_id);
}

void get_log_symlink_path(char *hunt_id, char *path, int path_size)
{
  snprintf(path, path_size, "logged_hunt-%s", hunt_id);
}

//check if a treasure id already exists in a hunt
int treasure_id_exists(char *hunt_id, char *treasure_id)
{
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
    
  int fd = open(file_path, O_RDONLY);
  if (fd == -1)
    {
      if (errno == ENOENT)
	{
	  return 0;
	}
      perror("Error opening treasure file");
      return -1;
    }
    
  Treasure temp;
  int result;
    
  while ((result = read_treasure(fd, &temp)) > 0)
    {
      if (strcmp(temp.id, treasure_id) == 0)
	{
	  close(fd);
	  return 1;
	}
    }
    
  close(fd);
  if (result < 0)
    {
      return -1;
    }
  else
    {
      return 0;
    }
}


//making sure a hunt directory exists
int ensure_hunt_directory(char *hunt_id)
{
  struct stat st = {0};
    
  char dir_path[256];
  snprintf(dir_path, sizeof(dir_path), "hunts/%s", hunt_id);
    
  if (stat(dir_path, &st) == -1)
    {
      if (mkdir(dir_path, 0755) == -1) {
	perror("Error creating hunt directory");
	return -1;
      }
      printf("Created new hunt directory: %s\n", dir_path);
    }
    
  return 0;
}

void get_treasure_input(Treasure *treasure)
{
  printf("Enter treasure ID: ");
  scanf("%s", treasure->id);
  getchar();
    
  printf("Enter username: ");
  scanf("%s", treasure->username);
  getchar();
    
  printf("Enter latitude: ");
  scanf("%f", &treasure->latitude);
    
  printf("Enter longitude: ");
  scanf("%f", &treasure->longitude);
    
  printf("Enter clue text: ");
  scanf(" %255[^\n]", treasure->clue);
    
  printf("Enter value: ");
  scanf("%d", &treasure->value);
}

void display_treasure(Treasure *treasure)
{
  printf("ID: %s\n", treasure->id);
  printf("Username: %s\n", treasure->username);
  printf("GPS: %.6f, %.6f\n", treasure->latitude, treasure->longitude);
  printf("Clue: %s\n", treasure->clue);
  printf("Value: %d\n", treasure->value);
  printf("-----------------------\n");
}


//function that tells us a if a hunt directory exists
int hunt_directory(char *hunt_id)
{
  struct stat st = {0};
    
  char path[256];
  snprintf(path, sizeof(path), "hunts/%s", hunt_id);
    
  if (stat(path, &st) == -1)
    {
      if (mkdir(path, 0755) == -1)
	{
	  perror("Error creating hunt directory");
	  return -1;
	}
      printf("Created new hunt directory: %s\n", path);
    }
  return 0;
}


//log an operation to the hunt's log file
int log_operation(char *hunt_id, char *operation)
{
  char log_path[256];
  get_log_file_path(hunt_id, log_path, sizeof(log_path));
    
  int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd == -1)
    {
      perror("Error opening log file");
      return -1;
    }
    
  char log_entry[512];
  snprintf(log_entry, sizeof(log_entry), "%s\n", operation);
    
  if (write(fd, log_entry, strlen(log_entry)) == -1)
    {
      perror("Error writing to log file");
      close(fd);
      return -1;
    }
    
  close(fd);
    
  //symbolic link to log file
  char symlink_path[256];
  get_log_symlink_path(hunt_id, symlink_path, sizeof(symlink_path));
    
  //remove existing symlink if it exists
  unlink(symlink_path);
    
  //new symlink
  if (symlink(log_path, symlink_path) == -1) {
    perror("Error creating symlink to log file");
    return -1;
  }
    
  return 0;
}


//add a new treasure to a hunt
int add_treasure(char *hunt_id)
{
  //make sure hunt directory exists
  char dir_path[256];
  get_hunt_dir_path(hunt_id, dir_path, sizeof(dir_path));
    
  if (ensure_hunt_directory(hunt_id) != 0)
    {
      return -1;
    }

  Treasure new_treasure;
  get_treasure_input(&new_treasure);
    
  if (treasure_id_exists(hunt_id, new_treasure.id) > 0)
    {
      printf("Error: Treasure ID '%s' already exists in hunt '%s'\n", new_treasure.id, hunt_id);
      return -1;
    }
    
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
    
  int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd == -1)
    {
      perror("Error opening treasure file");
      return -1;
    }
    
  if (write_treasure(fd, &new_treasure) != 0)
    {
      close(fd);
      return -1;
    }
  close(fd);
    
  char log_msg[256];
  snprintf(log_msg, sizeof(log_msg), "Added treasure: %s by user %s", new_treasure.id, new_treasure.username);
  log_operation(hunt_id, log_msg);
  printf("Treasure '%s' added successfully to hunt '%s'\n", new_treasure.id, hunt_id);
    
  return 0;
}


//list all the treasures in a hunt
int list_treasures(char *hunt_id)
{
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
    
  struct stat file_info;
  if (stat(file_path, &file_info) == -1)
    {
      if (errno == ENOENT)
	{
	  printf("Hunt '%s' has no treasures yet.\n", hunt_id);
	  return 0;
        }
      perror("Error getting treasure file information");
      return -1;
    }
  
  //hunt information
  printf("Hunt: %s\n", hunt_id);
  
  int fd = open(file_path, O_RDONLY);
  if (fd == -1)
    {
      perror("Error opening treasure file");
      return -1;
    }
    
  //read and write treasures
  Treasure treasure;
  int count = 0;
  int result;
    
  printf("Treasures:\n");
  printf("-----------------------\n");
    
  while ((result = read_treasure(fd, &treasure)) > 0)
    {
      printf("Treasure #%d:\n", ++count);
      display_treasure(&treasure);
    }
    
  close(fd);
    
  if (result < 0)
    {
      return -1;
    }
    
  if (count == 0)
    {
      printf("No treasures found in hunt '%s'.\n", hunt_id);
    }
  else
    {
      printf("Total treasures: %d\n", count);
    }
    
  char log_msg[256];
  snprintf(log_msg, sizeof(log_msg), "Listed %d treasures", count);
  log_operation(hunt_id, log_msg);
    
  return 0;
}


//view details of a specific treasure
int view_treasure(char *hunt_id, char *treasure_id)
{
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
    
  int fd = open(file_path, O_RDONLY);
  if (fd == -1)
    {
      if (errno == ENOENT)
	{
	  printf("Hunt '%s' does not exist or has no treasures.\n", hunt_id);
	  return -1;
        }
      perror("Error opening treasure file");
      return -1;
    }
    
  Treasure treasure;
  int found = 0;
  int result;
    
  while ((result = read_treasure(fd, &treasure)) > 0)
    {
      if (strcmp(treasure.id, treasure_id) == 0)
	{
	  found = 1;
	  break;
        }
    }
    
  close(fd);
    
  if (result < 0)
    {
      return -1;
    }
    
  if (!found)
    {
      printf("Treasure '%s' not found in hunt '%s'.\n", treasure_id, hunt_id);
      return -1;
    }
    
  //print the treasure
  printf("Hunt: %s\n", hunt_id);
  printf("Treasure details:\n");
  printf("-----------------------\n");
  display_treasure(&treasure);
    
  char log_msg[256];
  snprintf(log_msg, sizeof(log_msg), "Viewed treasure: %s", treasure_id);
  log_operation(hunt_id, log_msg);
    
  return 0;
}


//remove a treasure from a hunt
int remove_treasure(char *hunt_id, char *treasure_id)
{
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
    
  if (access(file_path, F_OK) == -1)
    {
      printf("Hunt '%s' does not exist or has no treasures.\n", hunt_id);
      return -1;
    }
    
  char temp_path[512];
  snprintf(temp_path, sizeof(temp_path), "%s.temp", file_path);
    
  int fd_src = open(file_path, O_RDONLY);
  if (fd_src == -1)
    {
      perror("Error opening source file");
      return -1;
    }
    
  int fd_temp = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd_temp == -1)
    {
      perror("Error creating temporary file");
      close(fd_src);
      return -1;
    }
    
  Treasure treasure;
  int found = 0;
  int result;
    
  while ((result = read_treasure(fd_src, &treasure)) > 0)
    {
      if (strcmp(treasure.id, treasure_id) == 0)
	{
	  found = 1;
	  continue;
        }
        
      if (write_treasure(fd_temp, &treasure) != 0)
	{
	  close(fd_src);
	  close(fd_temp);
	  unlink(temp_path);
	  return -1;
        }
    }
    
  close(fd_src);
  close(fd_temp);
    
  if (result < 0)
    {
      unlink(temp_path);
      return -1;
    }
    
  if (!found)
    {
      printf("Treasure '%s' not found in hunt '%s'.\n", treasure_id, hunt_id);
      unlink(temp_path);
      return -1;
    }
    
  if (rename(temp_path, file_path) == -1)
    {
      perror("Error replacing treasure file");
      unlink(temp_path);
      return -1;
    }
    
  char log_msg[256];
  snprintf(log_msg, sizeof(log_msg), "Removed treasure: %s", treasure_id);
  log_operation(hunt_id, log_msg);
    
  printf("Treasure '%s' removed successfully from hunt '%s'.\n", treasure_id, hunt_id);
    
  return 0;
}


//remove a hunt
int remove_hunt(char *hunt_id)
{
  char dir_path[256];
  get_hunt_dir_path(hunt_id, dir_path, sizeof(dir_path));
    
  struct stat st;
  if (stat(dir_path, &st) == -1)
    {
      printf("Hunt '%s' does not exist.\n", hunt_id);
      return -1;
    }
    
  char symlink_path[256];
  get_log_symlink_path(hunt_id, symlink_path, sizeof(symlink_path));
    
  //remove symlink
  unlink(symlink_path);
    
  //remove log file
  char log_path[256];
  get_log_file_path(hunt_id, log_path, sizeof(log_path));
  unlink(log_path);
    
  //remove treasure file
  char file_path[256];
  get_treasure_file_path(hunt_id, file_path, sizeof(file_path));
  unlink(file_path);
    
  //remove hunt directory
  if (rmdir(dir_path) == -1)
    {
      perror("Error removing hunt directory");
      return -1;
    }
    
  printf("Hunt '%s' removed successfully.\n", hunt_id);
    
  return 0;
}


int main(int argc, char *argv[])
{
  //choosing th etreasure manager option
  if (argc < 2)
    {
      printf("Error regarding arguments number\n");
      printf("treasure_manager <option>\n");
      return 1;
    }
  
  struct stat st = {0};
  if (stat("hunts", &st) == -1)
    {
      if(mkdir("hunts", 0755) == -1)
	{
	  perror("Error creating hunts directory");
	  return 1;
	}
    }

  char *command = argv[1];

  //the option is add
  if (strcmp(command, "add") == 0)
    {
      if(argc != 3)
	{
	  printf("treasure_manager add <hunt_id>\n");
	  return 1;
	}
      return add_treasure(argv[2]);
    }
  
  //the option is list
  else if (strcmp(command, "list") == 0)
    {
      if (argc != 3)
	{
	  printf("treasure_manager list <hunt_id>\n");
	  return 1;
	}
      return list_treasures(argv[2]);
    }
  
  //the option is view
  else if (strcmp(command, "view") == 0)
    {
      if (argc != 4)
	{
	  printf("treasure_manager view <hunt_id> <treasure_id>\n");
	  return 1;
	}
      return view_treasure(argv[2], argv[3]);
    }
  
  //the option is remove treasure
  else if (strcmp(command, "remove_treasure") == 0)
    {
      if (argc != 4)
	{
	  printf("treasure_manager remove_treasure <hunt_id> <treasure_id>\n");
	  return 1;
        }
      return remove_treasure(argv[2], argv[3]);
    }
  
  //the option is remove hunt
  else if (strcmp(command, "remove_hunt") == 0)
    {
      if (argc != 3)
	{
	  printf("treasure_manager remove_hunt <hunt_id>\n");
	  return 1;
        }
      return remove_hunt(argv[2]);
    }
    else
      {
	printf("Unknown command: %s\n", command);
	return 1;
      }

return 0;
}

