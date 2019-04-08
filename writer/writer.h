#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

static FileList* inputFileList;
static FILE* logFileP = NULL;
static char* tempFileContents = NULL;
static int clientPid, fifoFd = -1;

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName,
                int* clientPid);

void handleExit(int exitValue, int parentSignal);

void handleSigInt(int signal);

void handleSignals(int signal);

int tryWrite(int fd, const void* buffer, int bufferSize);

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName);

#endif