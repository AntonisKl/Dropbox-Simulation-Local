#ifndef MIRROR_CLIENT_H
#define MIRROR_CLIENT_H

#include "utils/utils.h"

typedef struct FileList FileList;

static char *commonDirName, *mirrorDirName, *logFileName;
static int bufferSize, clientIdFrom, clientIdTo;
static FileList* inputFileList;

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent);

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName);

void handleExit();

void handleSigUsr1(int signal);

void handleSigInt(int signal);

void handleSignals(int signal);

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

#endif