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
void forkAndExec(char* words[], int pdfs[], int dupNum);

int main() {

    // Buffer for reading one line of input
    char line[MAX_LINE_CHARS];
    // holds separated words based on whitespace
    char* line_words[MAX_LINE_WORDS + 1];

    // Loop until user hits Ctrl-D (end of input)
    // or some other input error occurs
    while( fgets(line, MAX_LINE_CHARS, stdin) ) {
        int num_words = split_cmd_line(line, line_words);

        /*for (int i=0; i < num_words; i++) {
            printf("%s\n", line_words[i]);
        }*/
    }

    char* p1Words[MAX_LINE_WORDS + 1];
    int lineIndex = 0;
    int numP1Words;
    while (strcmp(line_words[lineIndex], "|") != 0) {
        p1Words[lineIndex] = line_words[lineIndex];
        lineIndex++;
    }
    numP1Words = lineIndex;
    p1Words[numP1Words] = NULL;
    lineIndex++;

    printf("P1Words:\n");
    for (int i = 0; i < numP1Words; i++) {
        printf("%s\n", p1Words[i]);
    }

    char* p2Words[MAX_LINE_WORDS + 1];
    int numP2Words = 0;
    while (line_words[lineIndex] != 0) {
        p2Words[numP2Words] = line_words[lineIndex];
        numP2Words++;
        lineIndex++;
    }
    p2Words[numP2Words] = NULL;

    printf("P2Words:\n");
    for (int i = 0; i < numP2Words; i++) {
        printf("%s\n", p2Words[i]);
    }

    int pdfs[2];
    if (pipe(pdfs) == -1) {
        syserror("ERROR: could not create a pipe.");
    }

    forkAndExec(p1Words, pdfs, 1);
    forkAndExec(p2Words, pdfs, 0);

    close (pdfs[0]);
    close (pdfs[1]);
    while (wait(NULL) != -1);
    printf("Parent reaped child.\n");

    return 0;
}

void syserror(const char *s)
{
    extern int errno;
    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}

void forkAndExec(char* words[], int pdfs[], int dupNum) {
    int pid = fork();
    if (pid != 0) {
        return;
    }
    // pdfs must be piped
    if (pdfs != NULL) {
        if (dupNum != 0 && dupNum != 1) {
            syserror("Invalid dupNum.");
        }
        dup2(pdfs[dupNum], dupNum);
        if (close(pdfs[0]) == -1 || close(pdfs[1]) == -1) {
            syserror("Cannot close pdfs.");
        }
    }
    execvp(words[0], words);
}