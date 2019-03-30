#ifndef MIRROR_CLIENT_H
#define MIRROR_CLIENT_H

#include "utils/utils.h"

typedef struct FileList FileList;

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent);

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize);

void handleSigUsr1(int signal);

void handleSigInt(int signal);

void handleSignals(int signal);

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId);

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId);

#endif