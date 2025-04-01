#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "constants.h"
#include "parsetools.h"

void syserror(const char *);

int getProcessWords(char *pWords[], char *lWords[], int *lIndex);

void forkAndExec(char *words[]);

void forkAndExecWR(char *words[], int *pipes[], int numPipes);

void forkAndExecRD(char *words[], int *pipes[], int numPipes);

void forkAndExecRDWR(char *words[], int *pipes[], int numPipes);

void closeAllPipes(int *pipes[], int numPipes);

void remove_redirect_tokens(char *words[], int index) {
    // Removes both words[index] and words[index+1] from the array
    int i = index;
    while (words[i + 2] != NULL) {
        words[i] = words[i + 2];
        i++;
    }
    words[i] = NULL;
    words[i + 1] = NULL;
}

int main() {

    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char *line_words[MAX_LINE_WORDS + 1];
    // holds the pipes
    // the thought is at most half the words can be commands
    // ex w/ nine "words": ls | wc | wc | wc | tail (four pipes)
    int *pipes[MAX_LINE_WORDS / 2 + 1];
    int numPipes = 0;

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while (fgets(line, MAX_LINE_CHARS, stdin)) {
        split_cmd_line(line, line_words);
    }

    int lineIndex = 0;
    u_int8_t read = 0; // acting as boolean - is this process reading from a pipe?
    while (1) {
        char *pWords[MAX_LINE_WORDS + 1];
        int nullRet = getProcessWords(pWords, line_words, &lineIndex);
        if (nullRet == 0) { // exited from a NULL -> end of input, not writing to a pipe
            if (read == 0) { // not reading from a pipe
                forkAndExec(pWords);
            }
            else { // reading from a pipe
                forkAndExecRD(pWords, pipes, numPipes);
            }
            break;
        }
        else if (read == 0) { // not reading from a pipe but writing to a pipe
            int *pdfs = (int *) malloc(2 * sizeof(int));
            pipe(pdfs);
            pipes[numPipes] = pdfs;
            numPipes++;
            read = 1; // next process will be on the read end
            forkAndExecWR(pWords, pipes, numPipes);
        }
        else { // reading and writing from a pipe
            int *pdfs = (int *) malloc(2 * sizeof(int));
            pipe(pdfs);
            pipes[numPipes] = pdfs;
            numPipes++;
            // read is already 1
            forkAndExecRDWR(pWords, pipes, numPipes);
        }
    }

    closeAllPipes(pipes, numPipes);
    while (wait(NULL) != -1); // reap children

    return 0;
}

void syserror(const char *s) {
    extern int errno;
    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}

// logic for forkAndExec is pretty much the same with tweaks for each other fork function
void forkAndExec(char *words[]) {
    int pid = fork(); // fork the current process
    if (pid > 0) {
        waitpid(pid, NULL, 0);
        return;
    } else if (pid < 0) {
        syserror("ERROR: fork failed");
    }

    for (int i = 0; words[i] != NULL; i++) { // loop through all words to check for output redirection
        if (strcmp(words[i], "<") == 0) {
            int fd = open(words[i + 1], O_RDONLY);
            if (fd == -1) syserror("failed to open input file");
            if (dup2(fd, STDIN_FILENO) == -1) syserror("failed to redirect stdin");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;  // re-check current index
        }

        if (strcmp(words[i], ">") == 0) {  // output redirection (overwrite);
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // open file for overwrite

            if (fd == -1) syserror("failed to open output file");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
        else if (strcmp(words[i], ">>") == 0) {  // output redirection (append)
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) syserror("ERROR: Failed to open output file for append");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("ERROR: Failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
    }

    execvp(words[0], words); // replace process image with specified command

    syserror("ERROR: execvp() failed.");
}


void forkAndExecWR(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid > 0) {
        return;
    } else if (pid < 0) {
        syserror("ERROR: fork failed");
    }
    if (dup2(pipes[numPipes - 1][1], STDOUT_FILENO) == -1) {
        syserror("failed to redirect stdout to pipe");
    }
    closeAllPipes(pipes, numPipes);

    for (int i = 0; words[i] != NULL; i++) {
        if (strcmp(words[i], "<") == 0) {
            int fd = open(words[i + 1], O_RDONLY);
            if (fd == -1) syserror("ERROR: failed to open input file");
            if (dup2(fd, STDIN_FILENO) == -1) syserror("ERROR: failed to redirect stdin");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
        if (strcmp(words[i], ">") == 0) {
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) syserror("Failed to open output file");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
        else if (strcmp(words[i], ">>") == 0) {
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) syserror("failed to open output file for append");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
    }

    execvp(words[0], words);
    syserror("ERROR: execvp() failed.");
}

void forkAndExecRD(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid > 0) {
        return;
    } else if (pid < 0) {
        syserror("ERROR: fork failed");
    }

    if (dup2(pipes[numPipes - 1][0], STDIN_FILENO) == -1)
        syserror("failed to redirect stdin from pipe");

    closeAllPipes(pipes, numPipes);

    for (int i = 0; words[i] != NULL; i++) {
        if (strcmp(words[i], "<") == 0) {

            int fd = open(words[i + 1], O_RDONLY);
            if (fd == -1) syserror("failed to open input file");
            if (dup2(fd, STDIN_FILENO) == -1) syserror("failed to redirect stdin");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
    }

    execvp(words[0], words);
    syserror("ERROR: execvp() failed.");
}


void forkAndExecRDWR(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid > 0) {
        return;
    } else if (pid < 0) {
        syserror("ERROR: fork failed");
    }

    if (dup2(pipes[numPipes - 2][0], STDIN_FILENO) == -1)
        syserror("failed to redirect stdin from previous pipe");
    if (dup2(pipes[numPipes - 1][1], STDOUT_FILENO) == -1)
        syserror("failed to redirect stdout to next pipe");

    closeAllPipes(pipes, numPipes);

    for (int i = 0; words[i] != NULL; i++) {
        if (strcmp(words[i], "<") == 0) {
            int fd = open(words[i + 1], O_RDONLY);
            if (fd == -1) syserror("failed to open input file");
            if (dup2(fd, STDIN_FILENO) == -1) syserror("failed to redirect stdin");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
        else if (strcmp(words[i], ">") == 0) {
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) syserror("failed to open output file");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
        else if (strcmp(words[i], ">>") == 0) {
            int fd = open(words[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) syserror("failed to open output file for append");
            if (dup2(fd, STDOUT_FILENO) == -1) syserror("failed to redirect stdout");
            close(fd);
            remove_redirect_tokens(words, i);
            i--;

        }
    }

    execvp(words[0], words);
    syserror("ERROR: execvp() failed.");
}

int getProcessWords(char *pWords[], char *lWords[], int *lIndex) {
    // lIndex is a pointer bc it will be used further in main
    // and its dereference value must be up-to-date for main
    int pIndex = 0;
    while (lWords[*lIndex] != NULL && strcmp(lWords[*lIndex], "|") != 0) {
        pWords[pIndex++] = lWords[*lIndex];
        (*lIndex)++;
    }
    pWords[pIndex] = NULL; // last word in array for execv() must be NULL

    // return, indicating if hit a NULL or a pipe
    if (lWords[*lIndex] == NULL) { // return from NULL
        return 0;
    } else { // return from pipe
        (*lIndex)++;
        return 1;
    }
}

void closeAllPipes(int *pipes[], int numPipes) {
    for (int i = 0; i < numPipes; i++) { // free pipes
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
}