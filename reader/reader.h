#ifndef READER_H
#define READER_H

#include "../utils/utils.h"

static FileList* inputFileList;
static FILE* logFileP;

void handleArgsReader(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

void handleSigAlarm(int signal);

void handleSigIntReader(int signal);

void handleSignalsReader(int signal);

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName);

#endif