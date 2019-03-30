#ifndef UTILS_H
#define UTILS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../file_list/file_list.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"

#define MAX_STRING_INT_SIZE 12  // including end of string
#define MAX_FIFO_FILE_NAME (MAX_STRING_INT_SIZE * 2 - 1) + 9

// static char* curName; /* curName is a global variable for it to be able to be freed by the signal handler */

typedef struct FileList FileList;  // for forward declaration

static char *commonDirName, *inputDirName, *mirrorDirName;
static int bufferSize, clientIdFrom, clientIdTo;
static FileList* inputFileList;

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short array[1];
} SemAttributes;

void printErrorLn(char* s);

void raiseIntAndExit(int num);

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

char dirExists(char* dirName);

char fileExists(char* fileName);

void createDir(char* dirPath);

void buildIdFileName(char (*idFileFullName)[], char* commonDirName, int clientId);

void createAndWriteToFile(char* fileName, char* contents);

void tryWrite(int fd, const void* buffer, int bufferSize);

int tryRead(int fd, void* buffer, int bufferSize);

void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]);

char isIdFile(char* fileName);

char isSameIdFile(char* fileName, int clientId);

void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo);

#endif