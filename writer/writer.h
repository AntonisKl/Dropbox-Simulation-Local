#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

static FileList* inputFileList;

void handleArgsWriter(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize);

void handleSigIntWriter(int signal);

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize);

#endif