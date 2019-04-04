#ifndef READER_H
#define READER_H

#include "../utils/utils.h"

static FileList* inputFileList;
static FILE* logFileP;

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

void handleSigAlarm(int signal);

void handleSigInt(int signal);

void handleSignals(int signal);

void trySelect(int fifoFd);

int tryRead(int fd, void* buffer, int bufferSize);

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName);

#endif