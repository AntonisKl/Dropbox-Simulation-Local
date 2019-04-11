#ifndef READER_H
#define READER_H

#include "../utils/utils.h"

// these variables are declared static to be able to be used from signal handlers
// logFileP: pointer to the log file
// fifoFd: fifo pipe file descriptor
// filePath: variable to store the file's path that will be read from the fifo pipe
static FILE* logFileP = NULL;
static int fifoFd = -1;
static char* filePath = NULL;

// handles execute arguments
void handleArgs(int argc, char** argv, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

// handles exit (frees memory, closes file descriptors etc.)
// exitValue: the integer value with which the process will exit
// parentSignal: the signal to be sent to the parent process which is a client subprocess
// if parentSignal == 0 then no signal is sent to the parent process
void handleExit(int exitValue, int parentSignal);

// handles SIGALRM: calls handleExit with exit value 1 and parentSignal SIGUSR1
void handleSigAlarm(int signal);

// handles SIGINT: calls handleExit with exit value 1 and parentSignal 0
void handleSigInt(int signal);

// handles signals: calls one of the above specific signal handlers according to the received signal
void handleSignals(int signal);

// calls select for file descriptor fifoFd with 30 second timeout and does error handling
void trySelect(int fifoFd);

// calls read for file descriptor fd, writes the results to buffer, uses bufferSize as buffer size and does error handling
// because read() function is not guaranteed to read all the bufferSize bytes at once, tryRead() handles this case accordingly
void tryRead(int fd, void* buffer, int bufferSize);

#endif