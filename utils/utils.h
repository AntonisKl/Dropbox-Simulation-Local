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

// ENABLE / DISABLE GPG ENCRYPTION
#define GPG_ENCRYPTION_ON 0  // 0: gpg encryption disabled, 1: gpg encryption enabled

// COLOR CODES
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"

// USEFUL SIZES
#define MAX_STRING_INT_SIZE 12
#define MAX_FIFO_FILE_NAME (MAX_STRING_INT_SIZE * 2 - 1) + 9
#define MAX_FILE_LIST_NODE_STRING_SIZE MAX_STRING_INT_SIZE + 1 + (2 * PATH_MAX) + 1
#define EMPTY_FILE_LIST_STRING "**"
#define MAX_TEMP_FILELIST_FILE_NAME_SIZE 15 + MAX_STRING_INT_SIZE + 1  // "tmp/" + "TempFileList" + int
#define MIN_KEY_DETAILS_FILE_SIZE 119
#define MAX_TMP_ENCRYPTED_FILE_PATH_SIZE 8 + MAX_STRING_INT_SIZE

typedef struct FileList FileList;  // forward declaration

// prints string s in red color with newline
void printErrorLn(char* s);

// prints string s in red color with newline and raises SIGINT
void printErrorLnExit(char* s);

// returns:
// 1 if directory with path dirName exists
// 0 if directory with path dirName dose not exist
char dirExists(char* dirName);

// returns:
// 1 if file with path fileName exists
// 0 if file with path fileName dose not exist
char fileExists(char* fileName);

// creates a directory
void createDir(char* dirPath);

// removes a file or a directory recursively
void removeFileOrDir(char* path);

// renames (with mv) a file with path pathFrom to pathTo
void renameFile(char* pathFrom, char* pathTo);

// builds the id file path of the client with id clientId and puts it in the preallocated (*idFileFullName)[] variable
void buildIdFileName(char (*idFileFullName)[], char* commonDirName, int clientId);

// creates a file with path fileName and writes the contents to it
void createAndWriteToFile(char* fileName, char* contents);

// returns:
// 1 if file with path fileName is a client id file
// 0 if file with path fileName is not a client id file
char isIdFile(char* fileName);

// returns:
// 1 if file with path fileName is the client id file of the client with id clientId
// 0 if file with path fileName is not the client id file of the client with id clientId
char isSameIdFile(char* fileName, int clientId);

// builds the fifo file path that is used to send data from client with id clientIdFrom to client with id clientIdTo
// and puts it in the preallocated (*fifoFileName)[] variable
void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo);

// converts a FileList to a single string and puts it in the preallocated (*fileListS)[] variable
void fileListToString(FileList* fileList, char **fileListS);

// converts a string to a FileList and returns it
FileList* stringToFileList(char* fileListS);

// GPG FUNCTIONS START

// creates a gpg file with details that is used afterwards for batch key-pair generation and puts its name/path in the preallocated (*fileName)[] variable
void createGpgKeyDetailsFile(int clientId, char (*fileName)[]);

// generates a gpg key by using the details provided in the file with path keyDetailsPath
void generateGpgKey(char* keyDetailsPath);

// imports a gpg public key from the file with path filePath
void importGpgPublicKey(char* filePath);

// exports a gpg public key of the client with id clientId to the file with path outputFilePath
void exportGpgPublicKey(char* outputFilePath, int clientId);

// encrypts with gpg a file with path filePath using the public key of the client with id recipientClientId and puts the result in the file with path outputFilePath
void encryptFile(char* filePath, int recipientClientId, char* outputFilePath);

// decrypts with gpg a file with path filePath using the private key of the client with id clientId and puts the result in the file with path outputFilePath
void decryptFile(char* filePath, char* outputFilePath, int clientId);

// GPG FUNCTIONS END

// prepares reader's arguments and executes the reader's executable
// clientIdFrom: client id of the client that is executing the reader
// clientIdTo: client id of the client with which the reader will communicate
// commonDirName: the path of the common directory
// bufferSize: the size of the buffer that was specified by the user
// logFileName: the name of the log file that was specified by the user
void execReader(int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName);

// prepares writer's arguments and executes the writer's executable
// clientIdFrom: client id of the client that is executing the writer
// clientIdTo: client id of the client with which the writer will communicate
// commonDirName: the path of the common directory
// bufferSize: the size of the buffer that was specified by the user
// logFileName: the name of the log file that was specified by the user
// tempFileListFileName: the path of the temporary file that contains the string-converted FileList
// tempFileListSize: the size of the temporary file that contains the string-converted FileList
void execWriter(int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName, char* tempFileListFileName,
                unsigned long tempFileListSize);

#endif