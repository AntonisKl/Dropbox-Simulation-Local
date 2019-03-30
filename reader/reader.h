#ifndef READER_H
#define READER_H

#include "../utils/utils.h"

void handleSigAlarm(int signal);

void handleSigIntReader(int signal);

void handleSignalsReader(int signal);

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize);

#endif