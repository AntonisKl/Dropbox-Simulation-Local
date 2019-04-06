#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

static FileList* inputFileList;
static FILE* logFileP;
static char* tempFileContents = NULL;

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName);

void handleExit(int exitValue);

void handleSigInt(int signal);

void handleSignals(int signal);

int tryWrite(int fd, const void* buffer, int bufferSize);

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName);

#endif