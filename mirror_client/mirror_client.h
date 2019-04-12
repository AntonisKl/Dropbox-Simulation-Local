#ifndef MIRROR_CLIENT_H
#define MIRROR_CLIENT_H

#include "../utils/utils.h"

typedef struct FileList FileList;  // forward declaration

// these variables are declared static to be able to be used from signal handlers
// clientIdFrom: client id of the client that is being executed
// clientIdTo: client id of the client with which the current client subprocess will communicate
// commonDirName: the path of the common directory
// mirrorDirName: the path of the mirror directory
// bufferSize: the size of the buffer that was specified by the user
// logFileName: the name of the log file that was specified by the user
// tempFileListFileName: the path of the temporary file that contains the string-converted input FileList
// fileListS: variable to temporarily store the string-converted inputFileList
// tempFileListSize: the size of the temporary file that contains the string-converted input FileList
// inputFileList: the FileList that contains entries for all the regular files and directories of the input directory of the current client
// sigAction: struct for signal handling
// readerPid: the pid of the current reader process
// writerPid: the pid of the current writer process
// clientPid: client's process id
// attemptsNum: current number of attemps that were made so far by the client subprocess to restart the sync procedure
static char *commonDirName, *mirrorDirName, *logFileName, *tempFileListFileName, *fileListS = NULL;
static int bufferSize, clientIdFrom, clientIdTo, clientPid, attemptsNum = 0;
static unsigned long tempFileListSize;
static FileList* inputFileList;
static struct sigaction sigAction;
static pid_t readerPid = -1, writerPid = -1;

// handles execute arguments
void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName);

// does initial checks in order for the client to proceed normally or exit in some cases
// e.g. checks if common directory exists and if it doesn't, it creates it
void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]);

// recursively traverses the directory with name inputDirName and all of its subdirectories and
// adds to fileList all entries of the regular files and directories of the directory with name inputDirName
// indent: it is used only for printing purposes
// inputDirName: in each recursive call of the function it represents the path until the current file
void populateFileList(FileList* fileList, char* inputDirName, int indent);

// handles exit (frees memory, closes file descriptors etc.)
// exitValue: the integer value with which the process will exit
// removeIdFile: if it is 1 -> deletes client's .id file, if it is 0 -> does not delete client's .id file
// removeMirrorDir: if it is 1 -> deletes client's mirror directory, if it is 0 -> does not delete client's mirror directory
// writeToLogFile: if it is 1 -> client writes to log that it exited successfully, if it is 0 -> client doesn not write to log that it exited successfully
// removeTempFile: if it is 1 -> deletes client's temp FileList file, if it is 0 -> does not delete client's temp FileList file
void handleExit(int exitValue, char removeIdFile, char removeMirrorDir, char writeToLogFile, char removeTempFile);

// creates a new reader process and updates readerPid
void createReader();

// creates a new writer process and updates writerPid
void createWriter();

// creates a new reader process, a new writer process and waits until they both exit successfully
// this function is only called from the main/parent client's forked subprocesses that each of them handles the communication with only one other client
void createReaderAndWriter();

// handles SIGINT and is used only by the parent client (the main process)
void handleSigIntParentClient(int signal);

// handles SIGINT and is used only by the child clients (the forked subprocesses)
void handleSigInt(int signal);

// handles SIGUSR1 and is used only by the child clients (the forked subprocesses)
void handleSigUsr1(int signal);

// handles SIGUSR2 and is used only by the child clients (the forked subprocesses)
void handleSigUsr2(int signal);

// handles signals and is used only by the child clients (the forked subprocesses)
// calls one of the above specific signal handlers according to the received signal
void handleSignals(int signal);

// does the initial synchronization: handles each different .id file of the common directory and calls createReaderAndWriter() to handle the communication with each client
// specifically, for each .id file that it finds, it forks a new client subprocess that handles the reader's and writer's spawn
void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

// watches the common directory continuously by using inotify and handles IN_CREATE and IN_DELETE events accordingly
// in case of an IN_CREATE event if the file that was created is an .id file with different name than the current client's .id file
//      it forks a new client subprocess that handles the reader's and writer's spawn to synchronize with the new client
// in case of an IN_DELETE event if the file that was deleted is an .id file with different name than the current client's .id file
//      it deletes the directory of its mirror directory that corresponds to the client that has left the synchronization
//      e.g. if the client with id 5 has left and the client with id 2 that has a mirror directory with name mirror2 senses the event,
//          it deletes recursively the directory with path mirror2/5/
void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName);

#endif