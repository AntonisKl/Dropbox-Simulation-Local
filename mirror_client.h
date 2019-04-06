#ifndef MIRROR_CLIENT_H
#define MIRROR_CLIENT_H

#include "utils/utils.h"

typedef struct FileList FileList;

static char *commonDirName, *mirrorDirName, *logFileName, *tempFileListFileName;
static int bufferSize, clientIdFrom, clientIdTo;
static unsigned long tempFileListSize;
static FileList* inputFileList;
pid_t readerPid = -1, writerPid = -1;

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]);

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent);

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName);

void handleExit(int exitValue, char removeIdFile, char removeMirrorDir, char writeToLogFile, char removeTempFile);

void handleSigUsr1(int signal);

void handleSigInt(int signal);

void handleSignals(int signal);

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

#endif