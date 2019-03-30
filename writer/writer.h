#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

void handleSigIntWriter(int signal);

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize);

#endif