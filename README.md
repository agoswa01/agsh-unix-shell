# agsh: A Tiny Unix Shell in C

A minimal command-line shell written in C, built to learn **systems programming**: how programs read input, manage heap memory, and ask the operating system to run other programs.

It reads a line, splits it into words, and either runs a built-in command or launches an external program using `fork`, `exec`, and `wait`.

> Based on Stephen Brennan's tutorial, ["Write a Shell in C"](https://brennan.io/2015/01/16/write-a-shell-in-c/). This version adds extra comments and a few small cleanups for learning purposes.

---

## Features

- Interactive prompt (`> `)
- Runs any program found on your `PATH` (`ls`, `echo`, `gcc`, ...)
- Passes arguments to programs (`ls -l /home`)
- Built-in commands: `cd`, `help`, `exit`
- Handles input of any length by growing its buffers dynamically
- Exits cleanly on `Ctrl-D` (end of input)

## Requirements

- A Unix-like system (Linux, macOS, or WSL on Windows)
- A C compiler such as `gcc` or `clang`

## Build and run

```bash
gcc -Wall -o agsh agsh.c
./agsh
```

## Example session

```
> echo hello world
hello world
> cd /tmp
> pwd
/tmp
> ls -l
...
> help
Stephen Brennan's agsh
Type program names and arguments, and hit enter.
The following are built in:
  cd
  help
  exit
Use the man command for information on other programs.
> exit
```

---

## Project structure

```
.
├── agsh.c      # the entire shell (single file)
└── README.md  # this file
```

### Functions in `agsh.c`

| Function | Role |
|---|---|
| `main` | Entry point; starts the loop |
| `agsh_loop` | Repeats: prompt, read, parse, execute, free |
| `agsh_read_line` | Reads one line from stdin into a growable heap buffer |
| `agsh_split_line` | Splits the line into words using `strtok` |
| `agsh_execute` | Chooses between a builtin and an external program |
| `agsh_launch` | Runs a program with `fork` + `execvp` + `waitpid` |
| `agsh_cd`, `agsh_help`, `agsh_exit` | The built-in commands |

---

## How it works

A shell has three phases: **initialize, interpret (loop), terminate.** The loop runs these steps:

```
print "> "  ->  read line  ->  split into words  ->  execute  ->  free memory  ->  repeat
```

### Memory

| Area | Used for |
|---|---|
| **Stack** | Small local variables (`line`, `args`, `status`); freed automatically when a function returns |
| **Heap** | The input text buffer and the array of word pointers; allocated with `malloc`/`realloc`, released with `free` |

Each loop iteration allocates two heap blocks (the line and the word array) and frees both, so memory does not leak.

### Launching programs

```
        shell (parent)
             | fork()
      +------+------+
   parent           child (copy of the shell)
      |                  | execvp("ls")
   waitpid()...       becomes "ls" and runs
      |                  | exits
      +----- resumes ----+
```

| System call | What it does |
|---|---|
| `fork()` | Clones the current process. Memory pages are shared until written to (copy-on-write). |
| `execvp()` | Replaces the child with a new program loaded from disk |
| `waitpid()` | Parent sleeps (using no CPU) until the child finishes |

### Why `cd` and `exit` are built in

Each process has its own private state, such as its current directory. If `cd` were a separate program, it would change *its own* directory and exit, leaving the shell unchanged. So these commands must run inside the shell process itself.

### Adding a new builtin

1. Declare the function and add it to `builtin_str[]` (name) and `builtin_func[]` (function pointer).
2. Implement it, returning `1` to keep running or `0` to quit.

---

## Known limitations

- No quoting or escaping: `echo "a b"` passes two arguments, `"a` and `b"`
- No pipes (`|`) or redirection (`>`, `<`)
- No wildcards (`*`), variables (`$HOME`), or background jobs (`&`)
- No command history or tab completion
- Only three built-in commands

## Ideas for extending it

1. Add more builtins (`pwd`, `echo`, `history`)
2. Support quoted arguments
3. Implement output redirection with `open` and `dup2`
4. Implement pipes with `pipe` and `dup2`
5. Replace `agsh_read_line` with `getline()`
6. Read a config file at startup (the "initialize" phase)

## Customizing the name

`agsh` is just a prefix. To use your own name (for example `asha`):

```bash
sed -i 's/agsh_/asha_/g; s/agsh_/ASHA_/g' agsh.c
```

Then edit the prompt, error prefixes, and help banner by hand, and compile:

```bash
mv agsh.c asha.c
gcc -Wall -o asha asha.c
```

## Further reading

- `man 2 fork`, `man 3 exec`, `man 2 waitpid`, `man 2 chdir`
- [POSIX specification](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [Stephen Brennan's original tutorial](https://brennan.io/2015/01/16/write-a-shell-in-c/)

## Credits

Design and original code by Stephen Brennan. If you use this for a class assignment, check your instructor's policy first, since many courses ask students to write their own shell.
