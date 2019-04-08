#ifndef MIRROR_CLIENT_H
#define MIRROR_CLIENT_H

#include "../utils/utils.h"

typedef struct FileList FileList;

static char *commonDirName, *mirrorDirName, *logFileName, *tempFileListFileName;
static int bufferSize, clientIdFrom, clientIdTo, clientPid;
static unsigned long tempFileListSize;
static FileList* inputFileList;
static struct sigaction sigAction;
static pid_t readerPid = -1, writerPid = -1;

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]);

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent);

void createReader();

void createWriter();

void createReaderAndWriter();

void handleExit(int exitValue, char removeIdFile, char removeMirrorDir, char writeToLogFile, char removeTempFile);

void handleSigInt(int signal);

void handleSigInt(int signal);

void handleSignals(int signal);

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

#endif