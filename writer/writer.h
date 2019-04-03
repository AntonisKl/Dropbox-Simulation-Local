#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

static FileList* inputFileList;

void handleArgsWriter(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName);

void handleSigIntWriter(int signal);

int tryWrite(int fd, const void* buffer, int bufferSize);

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName);

#endif