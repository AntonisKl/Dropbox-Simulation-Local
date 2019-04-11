#ifndef WRITER_H
#define WRITER_H

#include "../utils/utils.h"

// these variables are declared static to be able to be used from signal handlers
// inputFileList: FileList that contains entries for all the regular files and directories of the input directory of the current client
// tempFileContents: variable to temporarily store the contents of the temp file that contains the inputFileList in string format
// logFileP: pointer to the log file
// fifoFd: fifo pipe file descriptor
// curFd: file descriptor of the current file that will be sent via the fifo pipe
static FileList* inputFileList;
static FILE* logFileP = NULL;
static char* tempFileContents = NULL;
static int fifoFd = -1, curFd = -1;

// handles execute arguments
void handleArgs(int argc, char** argv, FileList** inputFileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName);

// handles exit (frees memory, closes file descriptors etc.)
// exitValue: the integer value with which the process will exit
// parentSignal: the signal to be sent to the parent process which is a client subprocess
// if parentSignal == 0 then no signal is sent to the parent process
void handleExit(int exitValue, int parentSignal);

// handles SIGINT: calls handleExit with exit value 1 and parentSignal 0
void handleSigInt(int signal);

// handles SIGALRM: calls handleExit with exit value 1 and parentSignal SIGUSR2
void handleSigAlarm(int signal);

// handles SIGPIPE: calls handleExit with exit value 1 and parentSignal SIGUSR2
void handleSigPipe(int signal);

// handles signals: calls one of the above specific signal handlers according to the received signal
void handleSignals(int signal);

// calls write for file descriptor fd, writes from buffer buffer, uses bufferSize as buffer size and does error handling
// because write() function is not guaranteed to write all the bufferSize bytes at once, tryWrite() handles this case accordingly
void tryWrite(int fd, const void* buffer, int bufferSize);

#endif