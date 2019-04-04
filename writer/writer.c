#include "writer.h"

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 7) {
        printErrorLnExit("Invalid arguments. Exiting...");
    }

    // validate input arguments one by one
    // if (strcmp(argv[1], "-n") == 0) {
    // (*clientId) = atoi(argv[2]);
    // if ((*clientId) <= 0) {
    //     printError("Invalid arguments\nExiting...\n");
    //     raiseIntAndExit(1);
    // }
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    if (!strcmp(argv[1], EMPTY_FILE_LIST_STRING)) {
        (*fileList) = initFileList();
    } else {
        (*fileList) = stringToFileList(argv[1]);
    }
    (*clientIdFrom) = atoi(argv[2]);
    (*clientIdTo) = atoi(argv[3]);
    (*commonDirName) = argv[4];
    (*bufferSize) = atoi(argv[5]);
    (*logFileName) = argv[6];

    // if (strcmp(argv[3], "-c") == 0) {
    //     (*commonDirName) = argv[4];
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    // if (strcmp(argv[5], "-i") == 0) {
    //     (*inputDirName) = argv[6];
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    // if (strcmp(argv[7], "-m") == 0) {
    //     (*mirrorDirName) = argv[8];
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    // if (strcmp(argv[9], "-b") == 0) {
    //     (*bufferSize) = atoi(argv[10]);
    //     if ((*bufferSize) <= 0) {
    //         printErrorLnExit("Invalid arguments\nExiting...");
    //     }
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    // if (strcmp(argv[11], "-l") == 0) {
    //     (*logFileName) = argv[12];
    // } else {
    //     printErrorLnExit("Invalid arguments\nExiting...");
    // }

    return;
}

void handleExit(int exitValue) {
    freeFileList(&inputFileList);
    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    kill(getppid(), SIGUSR1);

    exit(exitValue);
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Caught wrong signal instead of SIGINT\n");
    }
    printf("Writer process with id %d caught SIGINT\n", getpid());

    handleExit(1);
}

void handleSignals(int signal) {
    // Find out which signal we're handling
    switch (signal) {
        case SIGINT:
            handleSigInt(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

int tryWrite(int fd, const void* buffer, int bufferSize) {
    int returnValue;
    if ((returnValue = write(fd, buffer, bufferSize)) == -1) {
        perror("write error");
        kill(getppid(), SIGUSR1);
        handleExit(1);
    }

    return returnValue;
}

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName) {
    printf("started writerJob with pid %d\n", getpid());

    struct sigaction sigAction;

    // Setup the signal handler
    sigAction.sa_handler = &handleSignals;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    char fifoFileName[MAX_FIFO_FILE_NAME];
    buildFifoFileName(&fifoFileName, clientIdFrom, clientIdTo);
    printf("writer with pid %d of client with id %d will use fifo %s\n", getpid(), clientIdFrom, fifoFileName);

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];
    strcpy(fifoFilePath, commonDirName);
    strcat(fifoFilePath, "/");
    strcat(fifoFilePath, fifoFileName);

    int fifoFd;
    char* fifo = fifoFilePath;
    char buffer[bufferSize + 1];

    if (!fileExists(fifoFilePath)) {
        printf("pid %d, before fifo\n", getpid());
        mkfifo(fifo, 0666);
        printf("pid %d, after fifo\n", getpid());
    }

    fifoFd = open(fifo, O_WRONLY);

    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
        exit(1);
    }

    File* curFile = inputFileList->firstFile;
    while (curFile != NULL) {
        int filePathSize = strlen(curFile->pathNoInputDir);
        char filePathSizeS[MAX_STRING_INT_SIZE];
        sprintf(filePathSizeS, "%d", filePathSize);
        tryWrite(fifoFd, &filePathSize, 2);
        char temp[PATH_MAX];
        printf("-------->writer with pid %d wrote filePathSize: %s\n", getpid(), filePathSizeS);
        memcpy(temp, curFile->pathNoInputDir, filePathSize + 1);
        printf("TEMP: %s\n\n", temp);
        tryWrite(fifoFd, curFile->pathNoInputDir, filePathSize);
        printf("-------->writer with pid %d wrote filePath: %s\n", getpid(), curFile->pathNoInputDir);
        char fileContentsSizeS[MAX_STRING_INT_SIZE];
        sprintf(fileContentsSizeS, "%ld", curFile->contentsSize);
        tryWrite(fifoFd, &curFile->contentsSize, 4);
        printf("-------->writer with pid %d wrote fileContentsSize: %s\n", getpid(), fileContentsSizeS);

        int fd = open(curFile->path, O_RDONLY);
        if (fd < 0) {
            perror("fopen failed");
            exit(1);
        }

        int bytesWritten = 0, remainingContentsSize = curFile->contentsSize, tempBufferSize = bufferSize;
        memset(buffer, 0, bufferSize + 1);

        while (read(fd, buffer, tempBufferSize) > 0) {
            if (remainingContentsSize < bufferSize) {
                tempBufferSize = remainingContentsSize;
            } else {
                tempBufferSize = bufferSize;
            }

            int size = tryWrite(fifoFd, buffer, tempBufferSize);
            printf("-------->writer with pid %d wrote a chunk of size %d of file %s: %s\n", getpid(), size, curFile->pathNoInputDir, buffer);

            memset(buffer, 0, bufferSize + 1);
            bytesWritten += tempBufferSize;
            remainingContentsSize -= tempBufferSize;
        }

        fprintf(logFileP, "Writer with pid %d sent file with path \"%s\" and wrote %d bytes to fifo pipe\n", getpid(), curFile->pathNoInputDir, bytesWritten);
        // write(fifoFd, buffer, bufferSize);
        // printf("-------->writer with pid %d wrote a chunk of file %s: %s\n", getpid(), curFile->path, buffer);
        // if (close(fd) == EOF) {
        //     perror("fclose failed");
        //     kill(getppid(), SIGUSR1);
        //     exit(1);
        // }
        close(fd);
        curFile = curFile->nextFile;
    }

    int end = 0;
    tryWrite(fifoFd, &end, 2);

    // handleExit(0);
    freeFileList(&inputFileList);
    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    return;
}

int main(int argc, char** argv) {
    FileList* inputFileList;
    int clientIdFrom, clientIdTo, bufferSize;
    char *commonDirName, *logFileName;

    handleArgs(argc, argv, &inputFileList, &clientIdFrom, &clientIdTo, &commonDirName, &bufferSize, &logFileName);

    writerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName);

    return 0;
}