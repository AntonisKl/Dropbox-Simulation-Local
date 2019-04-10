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
#include <sys/select.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../file_list/file_list.h"

#define GPG_ENCRYPTION_ON 0

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"

#define MAX_STRING_INT_SIZE 12  // including end of string
#define MAX_FIFO_FILE_NAME (MAX_STRING_INT_SIZE * 2 - 1) + 9
#define MAX_FILE_LIST_NODE_STRING_SIZE MAX_STRING_INT_SIZE + 1 + (2 * PATH_MAX) + 1
#define EMPTY_FILE_LIST_STRING "**"
#define MAX_TEMP_FILELIST_FILE_NAME_SIZE 12 + MAX_STRING_INT_SIZE + 1  // TempFileList + int
#define MIN_KEY_DETAILS_FILE_SIZE 141
#define MAX_TMP_ENCRYPTED_FILE_PATH_SIZE 8 + MAX_STRING_INT_SIZE

#define UNUSED(x) (void)(x)

// static char* curName; /* curName is a global variable for it to be able to be freed by the signal handler */

typedef struct FileList FileList;  // for forward declaration

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short array[1];
} SemAttributes;

void printErrorLn(char* s);

void printErrorLnExit(char* s);

void raiseIntAndExit(int num);

char dirExists(char* dirName);

char fileExists(char* fileName);

void createDir(char* dirPath);

void removeFileOrDir(char* path);

void renameFile(char* pathFrom, char* pathTo);

void buildIdFileName(char (*idFileFullName)[], char* commonDirName, int clientId);

void createAndWriteToFile(char* fileName, char* contents);

char isIdFile(char* fileName);

char isSameIdFile(char* fileName, int clientId);

void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo);

void fileListToString(FileList* fileList, char (*fileListS)[]);

FileList* stringToFileList(char* fileListS);

void createGpgKeyDetailsFile(int clientId, char (*fileName)[]);

void generateGpgKey(char* keyDetailsPath);

void importGpgPublicKey(char* filePath);

void exportGpgPublicKey(char* outputFilePath, int clientId);

void encryptFile(char* filePath, int recipientClientId, char* outputFilePath);

void decryptFile(char* filePath, char* outputFilePath);

void execReader(int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName, char* tempFileListFileName,
                unsigned long tempFileListSize, int clientPid);

void execWriter(int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName, char* tempFileListFileName,
                unsigned long tempFileListSize, int clientPid);

#endif