#include "reader.h"

void handleArgs(int argc, char** argv, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 7) {
        printErrorLnExit("Invalid arguments. Exiting...");
    }

    // assume that all arguments are valid
    (*clientIdFrom) = atoi(argv[1]);
    (*clientIdTo) = atoi(argv[2]);
    (*commonDirName) = argv[3];
    (*mirrorDirName) = argv[4];
    (*bufferSize) = atoi(argv[5]);
    (*logFileName) = argv[6];

    return;
}

void handleExit(int exitValue, int parentSignal) {
    if (filePath != NULL) {
        free(filePath);
        filePath = NULL;
    }

    // try to close log file
    if (logFileP != NULL && fclose(logFileP) == EOF) {
        perror("fclose failed");
        if (parentSignal != 0) {
            // send signal to parent process which is a subprocess of client
            kill(getppid(), parentSignal);
        }
        exit(1);
    }

    // try to close fifo
    if (fifoFd >= 0 && close(fifoFd) == -1) {
        perror("close failed");
        if (parentSignal != 0) {
            // send signal to parent process which is a subprocess of client
            kill(getppid(), parentSignal);
        }
        exit(1);
    }
    // reset fifoFd
    fifoFd = -1;

    if (parentSignal != 0) {
        // send signal to parent process which is a subprocess of client
        kill(getppid(), parentSignal);
    }

    // exit with exitValue as status
    exit(exitValue);
}

void handleSigAlarm(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Reader caught wrong signal instead of SIGALRM");
    }
    printf("Reader process with pid %d caught SIGALRM\n", getpid());

    handleExit(1, SIGUSR1);
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Reader caught wrong signal instead of SIGINT");
    }
    printf("Reader process with pid %d caught SIGINT\n", getpid());

    handleExit(1, 0);
}

void handleSignals(int signal) {
    // find out which signal we're handling
    switch (signal) {
        case SIGALRM:
            handleSigAlarm(signal);
            break;
        case SIGINT:
            handleSigInt(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

void trySelect(int fifoFd) {
    struct timeval timeStruct;
    timeStruct.tv_sec = 30;  // 30 seconds
    timeStruct.tv_usec = 0;

    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    FD_SET(fifoFd, &readFdSet);  // add only one file descriptor to the set

    // block until fifoFd is available for read
    int selectRetValue = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeStruct);

    if (selectRetValue == -1) {
        // error handling
        perror("select error");
        handleExit(1, SIGUSR1);
    } else if (selectRetValue == 0) {  // timeout ended
        printErrorLn("No data in fifo after 30 seconds");
        raise(SIGALRM);
    }

    return;  // reader can now read from fifo
}

void tryRead(int fd, void* buffer, int bufferSize) {
    int returnValue, tempBufferSize = bufferSize, progress = 0;

    trySelect(fd);
    returnValue = read(fd, buffer, tempBufferSize);
    while (returnValue < tempBufferSize && returnValue != 0) {  // rare case
        // not all desired bytes were read, so read the remaining bytes and add them to buffer
        if (returnValue == -1) {
            perror("read error");
            handleExit(1, SIGUSR1);
        }

        tempBufferSize -= returnValue;
        progress += returnValue;
        trySelect(fd);
        returnValue = read(fd, buffer + progress, tempBufferSize);
    }

    if (returnValue == 0) {  // 0 = EOF which means that writer failed and closed the pipe early
        handleExit(1, SIGUSR1);
    }

    return;
}

int main(int argc, char** argv) {
    // START OF SIGNAL HANLDING

    struct sigaction sigAction;

    // setup the signal handler
    sigAction.sa_handler = &handleSignals;

    // restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // handle only SIGINT and SIGALRM
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGALRM);

    if (sigaction(SIGALRM, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    // END OF SIGNAL HANLDING

    // see file "utils.h" above the execReader() function for information about each of the following variables
    int clientIdFrom, clientIdTo, bufferSize;
    char *commonDirName, *mirrorDirName, *logFileName;

    handleArgs(argc, argv, &clientIdFrom, &clientIdTo, &commonDirName, &mirrorDirName, &bufferSize, &logFileName);

    char fifoFileName[MAX_FIFO_FILE_NAME];                       // fifoFileName: name of fifo file
    buildFifoFileName(&fifoFileName, clientIdTo, clientIdFrom);  // format: [clientIdTo]_to_[clientIdFrom].fifo

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];  // fifoFilePath: path of fifo file
    sprintf(fifoFilePath, "%s/%s", commonDirName, fifoFileName);          // format: [commonDir]/[clientIdTo]_to_[clientIdFrom].fifo

    if (!fileExists(fifoFilePath)) {
        mkfifo(fifoFilePath, 0666);  // create fifo pipe
    }

    // block at open for 30 seconds and if open didn't return, raise SIGALRM
    alarm(30);
    fifoFd = open(fifoFilePath, O_RDONLY);  // fifoFd: declared globally in file "reader.h"
    alarm(0);

    if (fifoFd == -1) {
        // error handling
        perror("open failed");
        handleExit(1, SIGUSR1);
    }

    // open log file
    if ((logFileP = fopen(logFileName, "a")) == NULL) {  // logFileP: declared globally in file "reader.h"
        // error handling
        perror("fopen failed");
        handleExit(1, SIGUSR1);
    }

    char mirrorIdDirPath[strlen(mirrorDirName) + MAX_STRING_INT_SIZE + 1];
    sprintf(mirrorIdDirPath, "%s/%d", mirrorDirName, clientIdTo);  // format: [mirrorDir]/[id]

    if (!dirExists(mirrorIdDirPath)) {
        createDir(mirrorIdDirPath);
    }

    // filePathSize: variable to store the file path's size that will be read from fifo pipe
    // fileContentsSize: variable to store the file content's size that will be read from fifo pipe
    short int filePathSize = 0;
    int fileContentsSize = 0;

    // read file path's size from pipe and write to log file
    tryRead(fifoFd, &filePathSize, 2);
    fprintf(logFileP, "Reader with pid %d read 2 bytes of metadata from fifo pipe\n", getpid());
    fflush(logFileP);

    while (filePathSize != 0) {
        filePath = (char*)malloc(filePathSize + 1);  // filePath: declared globally in file "reader.h"
        memset(filePath, 0, filePathSize + 1);       // clear file path

        // read file path from pipe and write to log file
        tryRead(fifoFd, filePath, filePathSize);
        fprintf(logFileP, "Reader with pid %d read %d bytes of metadata from fifo pipe\n", getpid(), filePathSize);
        fflush(logFileP);

        // read file content's size from pipe and write to log
        tryRead(fifoFd, &fileContentsSize, 4);
        fprintf(logFileP, "Reader with pid %d read 4 bytes of metadata from fifo pipe\n", getpid());
        fflush(logFileP);

        // fileContents: buffer in which all of the read contents of the file will be stored in order to eventually write them to a file
        // bytesRead: the bytes of file contents that are read from fifo pipe (for logging purposes)
        char fileContents[fileContentsSize + 1];
        int bytesRead = 0;
        if (fileContentsSize > 0) {
            memset(fileContents, 0, fileContentsSize + 1);  // clear fileContents buffer
            char chunk[bufferSize + 1];                     // chunk: a part of the file contents

            // remainingContentsSize: the number of bytes that are remaining to be read from the fifo pipe
            // tempBufferSize: temporary buffer size to read from pipe in each while loop
            int remainingContentsSize = fileContentsSize;
            int tempBufferSize;
            while (remainingContentsSize > 0) {
                memset(chunk, 0, bufferSize + 1);  // clear chunk buffer

                // if the remaining contents of file have size less than the buffer size, read only the remaining contents from fifo pipe
                tempBufferSize = (remainingContentsSize < bufferSize ? remainingContentsSize : bufferSize);

                tryRead(fifoFd, chunk, tempBufferSize);  // read a chunk of size tempBufferSize from pipe

                bytesRead += tempBufferSize;              // for logging purposes
                strcat(fileContents, chunk);              // concatenated the read chunk to the total file content's buffer
                remainingContentsSize -= tempBufferSize;  // proceed tempBufferSize bytes
            }
        }

        // write to log
        fprintf(logFileP, "Reader with pid %d received file with path \"%s\" and read %d bytes from fifo pipe\n", getpid(), filePath, bytesRead);
        fflush(logFileP);

        char mirrorFilePath[strlen(mirrorIdDirPath) + strlen(filePath) + 2];  // mirrorFilePath: the path of the mirrored file
        sprintf(mirrorFilePath, "%s/%s", mirrorIdDirPath, filePath);           // format: [mirrorDir]/[id]/[filePath]

        if (fileContentsSize > 0) {  // is a regular file
            createAndWriteToFile(mirrorFilePath, fileContents);

            if (GPG_ENCRYPTION_ON) {
                char tempPath[strlen(mirrorFilePath) + 5];
                sprintf(tempPath, "%stemp", mirrorFilePath);
                decryptFile(mirrorFilePath, tempPath, clientIdFrom);  // decrypt the encrypted read file and put the output in a temp file
                removeFileOrDir(mirrorFilePath);                      // remove the old encrypted file
                renameFile(tempPath, mirrorFilePath);                 // rename the new decrypted file to have the correct path
            }
        } else if (fileContentsSize == 0) {            // is an empty file
            createAndWriteToFile(mirrorFilePath, "");  // create and write an empty string to the file
        } else if (fileContentsSize == -1) {           // is a directory
            createDir(mirrorFilePath);                 // create a directory
        }

        // free filePath
        if (filePath != NULL) {
            free(filePath);
            filePath = NULL;
        }

        // read file path's size from pipe and write to log file
        tryRead(fifoFd, &filePathSize, 2);
        fprintf(logFileP, "Reader with pid %d read 2 bytes of metadata from fifo pipe\n", getpid());
        fflush(logFileP);
    }

    handleExit(0, 0);  // handle normal exit
}