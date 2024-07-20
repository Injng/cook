#ifndef COOK_H
#define COOK_H

#include <glob.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define COMPILER "gcc"
#define FLAGS "-Wall -Wextra -pedantic -ansi -std=c11"
#define REMOVE "*.o"

int cmd(int argc, char **argv) {
    pid_t pid;
    int status;
    if ((pid = fork()) < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

int wildcard_rm(char *pattern) {
    glob_t globbuf;

    // perform wildcard expansion
    int ret = glob(pattern, 0, NULL, &globbuf);
    if (ret != 0) {
        fprintf(stderr, "Error in glob: %s\n", pattern);
        return -1;
    }

    // construct and execute the rm command for each matched file
    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        char *file = globbuf.gl_pathv[i];
        printf("Removing: %s\n", file);
        if (remove(file) != 0) {
            fprintf(stderr, "WARNING: failed to remove %s\n", file);
        }
    }

    // clean up
    globfree(&globbuf);
    return 0;
}

int clean(int num_dirs, char **dirs, char *targets_in) {
    // ensure targets are mutable
    char targets[strlen(targets_in) + 1];
    strcpy(targets, targets_in);

    // get current directory
    char cwd[1024];    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return -1;
    }

    // clean up targets in specified directories
    for (int i = 0; i < num_dirs; i++) {
        char *target = strtok(targets, " ");
        if (chdir(dirs[i]) < 0) {
            printf("ERROR: failed to change to %s\n", dirs[i]);
            return -1;
        }
        while (target != NULL) {
            if (wildcard_rm(target) < 0) {
                return -1;
            }
            target = strtok(NULL, " ");
        }
        if (chdir(cwd) < 0) {
            printf("ERROR: failed to change to %s\n", cwd);
            return -1;
        }
    }
    return 0;
}

int compile(char *src, char *obj, char *compiler, char *flag_in) {
    // ensure flags are mutable
    char flags[strlen(flag_in) + 1];
    strcpy(flags, flag_in);

    // determine the number of flags
    int num_flags = 1;  // start with 1 for the compiler itself
    char *token = strtok(flags, " ");
    while (token != NULL) {
        num_flags++;
        token = strtok(NULL, " ");
    }

    // allocate memory for argv
    char **argv = (char **) malloc((6 + num_flags) * sizeof(char*));
    if (argv == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // populate argv
    argv[0] = compiler;
    argv[1] = strtok(flags, " ");
    argv[2] = "-c";
    argv[3] = src;
    argv[4] = "-o";
    argv[5] = obj;
    int i = 6;
    token = strtok(NULL, " ");
    while (token != NULL) {
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL; 

    // compile
    int result = cmd(i, argv);

    // clean up
    free(argv);
    return result;
}

int ln(char *objects_in, char *out) {
    // ensure objects are mutable
    char objects[strlen(objects_in) + 1];
    strcpy(objects, objects_in);

    // determine the number of objects
    int num_objects = 1;  // start with 1 for the linker itself
    char *token = strtok(objects, " ");
    while (token != NULL) {
        num_objects++;
        token = strtok(NULL, " ");
    }

    // allocate memory for argv
    char **argv = (char **) malloc((3 + num_objects) * sizeof(char*));
    if (argv == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // populate argv
    argv[0] = COMPILER;
    argv[1] = objects;
    argv[2] = "-o";
    argv[3] = out;
    int i = 4;
    token = strtok(NULL, " ");
    while (token != NULL) {
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL; 

    // link
    int result = cmd(i, argv);

    // clean up
    free(argv);
    return result;
}

#endif /* COOK_H */

