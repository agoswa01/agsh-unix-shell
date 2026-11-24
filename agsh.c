/*
 * agsh - a tiny Unix shell in C
 * Based on Stephen Brennan's "Write a Shell in C" tutorial.
 * Named it anuranan goswami shell or agshell
 * Compile:  gcc -Wall -o agsh agsh.c
 * Run:      ./agsh
 */

#include <sys/wait.h>   // waitpid() and WIFEXITED / WIFSIGNALED macros
#include <unistd.h>     // chdir(), fork(), execvp(), pid_t
#include <stdlib.h>     // malloc(), realloc(), free(), exit(), EXIT_*
#include <stdio.h>      // printf(), fprintf(), getchar(), perror(), stderr
#include <string.h>     // strcmp(), strtok()

/* ------------------------------------------------------------------ */
/* Forward declarations of builtins (needed because agsh_help uses the  */
/* builtin arrays, and the arrays contain agsh_help).                   */
/* ------------------------------------------------------------------ */
int agsh_cd(char **args);
int agsh_help(char **args);
int agsh_exit(char **args);

/* Names of builtin commands ... */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

/* ... and the matching functions (array of function pointers). */
int (*builtin_func[]) (char **) = {
  &agsh_cd,
  &agsh_help,
  &agsh_exit
};

int agsh_num_builtins(void)
{
  return sizeof(builtin_str) / sizeof(char *);
}

/* ------------------------------------------------------------------ */
/* Builtin implementations. Each returns 1 to keep running, 0 to quit. */
/* ------------------------------------------------------------------ */
int agsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "agsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {   // changes the SHELL's own directory
      perror("agsh");
    }
  }
  return 1;
}

int agsh_help(char **args)
{
  (void)args;   // unused
  printf("Stephen Brennan's agsh\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (int i = 0; i < agsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int agsh_exit(char **args)
{
  (void)args;   // unused
  return 0;     // 0 tells the main loop to stop
}

/* ------------------------------------------------------------------ */
/* Launching external programs: fork + exec + wait                     */
/* ------------------------------------------------------------------ */
int agsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();                       // clone this process
  if (pid == 0) {
    // ---- Child process ----
    if (execvp(args[0], args) == -1) {   // replace child with new program
      perror("agsh");
    }
    exit(EXIT_FAILURE);               // only reached if exec failed
  } else if (pid < 0) {
    // ---- fork() itself failed ----
    perror("agsh");
  } else {
    // ---- Parent process: wait for the child to finish ----
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;   // keep the shell running
}

/* ------------------------------------------------------------------ */
/* Decide: builtin or external program?                                */
/* ------------------------------------------------------------------ */
int agsh_execute(char **args)
{
  if (args[0] == NULL) {
    return 1;   // empty command, just prompt again
  }

  for (int i = 0; i < agsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);   // call through function pointer
    }
  }

  return agsh_launch(args);
}

/* ------------------------------------------------------------------ */
/* Reading a line: growable heap buffer                                */
/* ------------------------------------------------------------------ */
#define agsh_RL_BUFSIZE 1024

char *agsh_read_line(void)
{
  int bufsize = agsh_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);   // heap allocation
  int c;                                           // int, NOT char (EOF is -1)

  if (!buffer) {
    fprintf(stderr, "agsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();   // read one character from stdin

    // End of line or end of input: terminate the string and return it
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // Buffer full? Grow it.
    if (position >= bufsize) {
      bufsize += agsh_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "agsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/* ------------------------------------------------------------------ */
/* Splitting a line into words (tokens) using strtok                   */
/* ------------------------------------------------------------------ */
#define agsh_TOK_BUFSIZE 64
#define agsh_TOK_DELIM " \t\r\n\a"

char **agsh_split_line(char *line)
{
  int bufsize = agsh_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));   // array of pointers
  char *token;

  if (!tokens) {
    fprintf(stderr, "agsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, agsh_TOK_DELIM);   // first word
  while (token != NULL) {
    tokens[position] = token;            // store pointer (no text copied)
    position++;

    if (position >= bufsize) {
      bufsize += agsh_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "agsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, agsh_TOK_DELIM); // next word
  }
  tokens[position] = NULL;               // end marker, like argv
  return tokens;
}

/* ------------------------------------------------------------------ */
/* The main loop: read -> parse -> execute -> free                     */
/* ------------------------------------------------------------------ */
void agsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = agsh_read_line();
    args = agsh_split_line(line);
    status = agsh_execute(args);

    free(line);   // give back the text buffer
    free(args);   // give back the pointer array
  } while (status);
}

/* ------------------------------------------------------------------ */
/* Entry point                                                         */
/* ------------------------------------------------------------------ */
// All main does is start the loop. (void)argc; just tells the compiler we know these are unused.
int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  // Load config files, if any.

  // Run command loop.
  agsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
