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

// Writes an error message to stderr and exits.
void syserror(const char *);

// Given lWords, a list of all words in the line, and lIndex,
// it extracts one process's words from lWords
// starting at lIndex and puts them in pWords.
// Returns 0 if ended on a NULL or 1 if ended on a |.
u_int8_t getProcessWords(char *pWords[], char *lWords[], int *lIndex);

// Given words[], the words used to call the process, this function forks
// a new process and execs according to words.
void forkAndExec(char *words[]);

// Given words[], the words used to call the process, this function forks
// a new process, creates a new pipe to write to (storing it in pipes[numPipes]),
// and execs according to words.
void forkAndExecWR(char *words[], int *pipes[], int numPipes);

// Given words[], the words used to call the process, this function forks
// a new process, reads from pipes[numPipes - 1], and execs according to words. 
void forkAndExecRD(char *words[], int *pipes[], int numPipes);

// Given words[], the words used to call the process, this function forks
// a new process, reads from pipes[numPipes - 1], creates a new pipe to write to
// (storing it in pipes[numPipes]), and execs according to words. 
void forkAndExecRDWR(char *words[], int *pipes[], int numPipes);

// Given pipes[], the list of all created pipes, and numPipes, this function
// loops through pipes[0] to pipes[numPipes - 1], closing both ends of the pipe
// and freeing them.
void closeAllPipes(int *pipes[], int numPipes);

// Given words[], the words used to call the process, this function detects 
// stdin and stdout overrides, executes them, and removes the override tokens
// from words so it can be used in any of the forkAndExec functions.
void handleRedirects(char *words[]);

// Given words[], this function removes the override tokens
// from words so it can be used in any of the forkAndExec functions.
// It is called in handleRedirects()
void remove_redirect_tokens(char *words[], int index);

// Replaces phrases within quotes with a single string with quotes
// removed. Words are separated by a comma.
void replaceQuotes(char *words[]);

// Replaces words at start through end index with newStr.
void replace_section(char *words[], int start, int end, char *newStr);

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
        return;
    } else if (pid < 0) {
        syserror("ERROR: fork failed");
    }
    // no pipes to close here
    replaceQuotes(words);
    handleRedirects(words);

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
    replaceQuotes(words);
    handleRedirects(words);

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
    replaceQuotes(words);
    handleRedirects(words);

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
    replaceQuotes(words);
    handleRedirects(words);

    execvp(words[0], words);
    syserror("ERROR: execvp() failed.");
}

u_int8_t getProcessWords(char *pWords[], char *lWords[], int *lIndex) {
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

void handleRedirects(char *words[]) {
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
}

void replaceQuotes(char *words[]) {
    int i = 0;
    int quoteStart = -1; // -1 indicates no start quote found
    char *newString = (char *) malloc(1024);
    while (words[i] != NULL) {
        char startChar = words[i][0];
        char endChar = words[i][strlen(words[i]) - 1];
        if (startChar == '"') { // start quote
            quoteStart = i;
            strcpy(newString, &words[i][1]);
        }
        else if (endChar == '"') { // this is an end quote
            words[i][strlen(words[i]) - 1] = '\0'; // replace quote with null to concatenate
            strcat(newString, " ");
            strcat(newString, words[i]);
            replace_section(words, quoteStart, i, newString);
            quoteStart = -1;
        }
        else if (quoteStart != -1) { // between quotes
            strcat(newString, " ");
            strcat(newString, words[i]);
        }
        i++;
    }
}

void replace_section(char *words[], int start, int end, char *newStr) {
    int range = end - start;
    words[start] = newStr;
    start++;
    while (words[start] != NULL && words[start + range] != NULL) {
        words[start] = words[start + range];
        start++;
    }
    words[start] = NULL;
    int i = 0;
    puts("words:");
    while (words[i] != NULL) {
        puts(words[i]);
        i++;
    }
}