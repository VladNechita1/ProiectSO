#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR 64
#define MAX_USERS 100

typedef struct {
  char id[16];
  char username[32];
  float latitude;
  float longitude;
  char clue[128];
  int value;
} Treasure;

struct UserScore {
  char username[MAX_STR];
  int score;
};

int main(int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <path_to_treasures.dat>\n", argv[0]);
      return 1;
    }

  FILE *file = fopen(argv[1], "rb");
  if (!file)
    {
      perror("Error opening file");
      return 1;
    }

  Treasure t;
  struct UserScore scores[MAX_USERS];
  int user_count = 0;

  while (fread(&t, sizeof(t), 1, file) == 1)
    {
      int found = 0;
      for (int i = 0; i < user_count; i++)
	{
	  if (strcmp(scores[i].username, t.username) == 0)
	    {
	      scores[i].score += t.value;
	      found = 1;
	      break;
	    }
	}
      if (!found && user_count < MAX_USERS)
	{
	  strncpy(scores[user_count].username, t.username, MAX_STR);
	  scores[user_count].score = t.value;
	  user_count++;
	}
    }

  fclose(file);

  for (int i = 0; i < user_count; i++)
    {
      printf("Username: %s | Score: %d\n", scores[i].username, scores[i].score);
    }

  return 0;
}
