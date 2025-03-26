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

void forkAndExec(char *words[]) {
    int pid = fork();
    if (pid != 0) {
        return;
    }
    execvp(words[0], words);
}

void forkAndExecWR(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid != 0) {
        return;
    }
    // pdfs must be piped
    dup2(pipes[numPipes - 1][1], 1);
    closeAllPipes(pipes, numPipes);
    execvp(words[0], words);
}

void forkAndExecRD(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid != 0) {
        return;
    }
    // pdfs must be piped
    dup2(pipes[numPipes - 1][0], 0);
    closeAllPipes(pipes, numPipes);
    execvp(words[0], words);
}

void forkAndExecRDWR(char *words[], int *pipes[], int numPipes) {
    int pid = fork();
    if (pid != 0) {
        return;
    }
    // pdfsRD/WR must be piped
    dup2(pipes[numPipes - 2][0], 0);
    dup2(pipes[numPipes - 1][1], 1);
    closeAllPipes(pipes, numPipes);
    execvp(words[0], words);
}

int getProcessWords(char *pWords[], char *lWords[], int *lIndex) {
    // lIndex is a pointer bc it will be used further in main
    // and its dereference value must be up-to-date for main
    int pIndex = 0;
    while (lWords[*lIndex] != NULL && strcmp(lWords[*lIndex], "|") != 0) {
        pWords[pIndex] = lWords[*lIndex];
        pIndex++;
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