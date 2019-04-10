#include "reader.h"

static char *filePath = NULL, *filePathSizeS = NULL, *fileContentsSizeS = NULL, *tempFileContents = NULL;

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName,
                int* clientPid) {
    // validate argument count
    if (argc != 10) {
        printErrorLnExit("Invalid arguments. Exiting...");
    }

    printf("handling args reader\n");

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
    FILE* fp = fopen(argv[1], "r");
    char* endptr;
    unsigned long fileBufferSize = strtol(argv[2], &endptr, 10) + 1;
    tempFileContents = (char*)malloc(fileBufferSize);
    fgets(tempFileContents, fileBufferSize, fp);
    fclose(fp);

    if (!strcmp(tempFileContents, EMPTY_FILE_LIST_STRING)) {
        (*fileList) = initFileList();
    } else {
        (*fileList) = stringToFileList(tempFileContents);
    }
    free(tempFileContents);
    tempFileContents = NULL;

    printf("---->Reader with pid %d got list with size %u: \n\n", getpid(), (*fileList)->size);
    // File* tempFile = (*fileList)->firstFile;
    // while (tempFile != NULL) {
    //     printf("file path: %s\n", tempFile->path);

    //     tempFile = tempFile->nextFile;
    // }

    (*clientIdFrom) = atoi(argv[3]);
    (*clientIdTo) = atoi(argv[4]);
    (*commonDirName) = argv[5];
    (*mirrorDirName) = argv[6];
    (*bufferSize) = atoi(argv[7]);
    (*logFileName) = argv[8];
    (*clientPid) = atoi(argv[9]);

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

void handleExit(int exitValue, int parentSignal) {
    if (filePath != NULL) {
        free(filePath);
        filePath = NULL;
    }

    if (filePathSizeS != NULL) {
        free(filePathSizeS);
        filePathSizeS = NULL;
    }

    if (fileContentsSizeS != NULL) {
        free(fileContentsSizeS);
        fileContentsSizeS = NULL;
    }

    if (tempFileContents != NULL) {
        free(tempFileContents);
        tempFileContents = NULL;
    }

    freeFileList(&inputFileList);

    if (logFileP != NULL && fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    if (fifoFd >= 0 && close(fifoFd) == -1) {
        perror("close failed");
        handleExit(1, SIGUSR2);
    }

    // printf("client pid: %d\n", clientPid);
    if (parentSignal != 0) {
        kill(getppid(), parentSignal);
    }

    exit(exitValue);
}

void handleSigAlarm(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Reader caught wrong signal instead of SIGALRM");
    }
    printf("Reader process with id %d caught SIGALRM, waited more than 30 seconds to read from pipe\n", getpid());

    // if (filePath != NULL) {
    //     free(filePath);
    //     filePath = NULL;
    // }
    // if (filePathSizeS != NULL) {
    //     free(filePathSizeS);
    //     filePathSizeS = NULL;
    // }
    // if (fileContentsSizeS != NULL) {
    //     free(fileContentsSizeS);
    //     fileContentsSizeS = NULL;
    // }

    // freeFileList(&inputFileList);
    // if (fclose(logFileP) == EOF) {
    //     perror("fclose failed");
    //     exit(1);
    // }

    // kill(getppid(), SIGUSR1);

    handleExit(1, SIGUSR1);
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Reader caught wrong signal instead of SIGINT");
    }
    printf("Reader process with id %d caught SIGINT\n", getpid());

    handleExit(1, 0);
}

void handleSignals(int signal) {
    // Find out which signal we're handling
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
    timeStruct.tv_sec = 30;
    timeStruct.tv_usec = 0;

    fd_set readFdSet;
    FD_ZERO(&readFdSet);
    FD_SET(fifoFd, &readFdSet);

    // printErrorLn("BEEEEEEEEEEEEEEEEEEEEEEEEEEEEEFORE SELECT");
    int selectRetValue = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeStruct);
    // printErrorLn("AAAAAAAAAAAAAAAAAAAAAAFTER SELECT");

    if (selectRetValue == -1)
        perror("select error");
    else if (selectRetValue == 0) {
        printErrorLn("No data in fifo after 30 seconds.");
        raise(SIGALRM);
    }

    return;  // reader can now read from fifo
}

int tryRead(int fd, void* buffer, int bufferSize) {
    int returnValue, tempBufferSize = bufferSize, progress = 0;

    trySelect(fd);
    returnValue = read(fd, buffer, tempBufferSize);
    while (returnValue < tempBufferSize && returnValue != 0) {
        if (returnValue == -1) {
            perror("read error");
            handleExit(1, SIGUSR1);
        }

        //printf("+++++++++++++++++++++++++++++++++++++++++++++ reader with pid %d read %d bytes from pipe\n", getpid(), returnValue);
        tempBufferSize -= returnValue;
        progress += returnValue;
        trySelect(fd);
        returnValue = read(fd, buffer + progress, tempBufferSize);
    }
    if (returnValue == 0) {  // EOF which means that writer failed and closed the pipe early
        handleExit(1, SIGUSR1);
    }
    //printf("======================================================== reader with pid %d read %d bytes from pipe\n", getpid(), returnValue);

    return returnValue;
}

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName) {
    printf("started readerJob with pid %d\n", getpid());
    struct sigaction sigAction;

    // Setup the signal handler
    sigAction.sa_handler = &handleSignals;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGALRM);

    if (sigaction(SIGALRM, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    char fifoFileName[MAX_FIFO_FILE_NAME];
    buildFifoFileName(&fifoFileName, clientIdTo, clientIdFrom);
    printf("reader with pid %d of client with id %d will use fifo %s\n", getpid(), clientIdFrom, fifoFileName);

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];
    strcpy(fifoFilePath, commonDirName);
    strcat(fifoFilePath, "/");
    strcat(fifoFilePath, fifoFileName);

    char* fifo = fifoFilePath;

    if (!fileExists(fifoFilePath)) {
        printf("pid %d, before fifo\n", getpid());
        mkfifo(fifo, 0666);
        printf("pid %d, after fifo\n", getpid());
    }

    alarm(30);
    fifoFd = open(fifo, O_RDONLY);
    alarm(0);

    if (fifoFd == -1) {
        perror("open failed");
        handleExit(1, SIGUSR1);
    }

    short int filePathSize = 0;
    int fileContentsSize = 0;

    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
        handleExit(1, SIGUSR1);
    }

    // filePathSizeS = malloc(3);
    // memcpy(filePathSizeS, "\0", 3);

    // alarm(30);
    int readReturnValue = tryRead(fifoFd, &filePathSize, 2);
    fprintf(logFileP, "Reader with pid %d read 2 bytes of metadata from fifo pipe\n", getpid());
    // alarm(0);
    // filePathSize = atoi(filePathSizeS);
    //printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);

    while (readReturnValue > 0 && filePathSize != 0) {
        filePath = (char*)malloc(filePathSize + 1);
        memset(filePath, 0, filePathSize + 1);

        // alarm(30);
        //printf("-------->reader with pid %d BEFORE read filePath: %s\n", getpid(), filePath);
        tryRead(fifoFd, filePath, filePathSize);
        fprintf(logFileP, "Reader with pid %d read %d bytes of metadata from fifo pipe\n", getpid(), filePathSize);
        // printf("-------->reader with pid %d read filePath: %s\n", getpid(), filePath);
        // alarm(0);

        fileContentsSizeS = malloc(5);
        memset(fileContentsSizeS, 0, 5);
        // alarm(30);
        tryRead(fifoFd, &fileContentsSize, 4);
        fprintf(logFileP, "Reader with pid %d read 4 bytes of metadata from fifo pipe\n", getpid());

        // alarm(0);
        // fileContentsSize = atoi(fileContentsSizeS);
        //printf("-------->reader with pid %d read fileContentsSize: %d\n", getpid(), fileContentsSize);

        char fileContents[fileContentsSize + 1];
        int bytesRead = 0;
        if (fileContentsSize > 0) {
            strcpy(fileContents, "");
            char chunk[bufferSize + 1];
            // alarm(30);
            // tryRead(fifoFd, fileContents, fileContentsSize);
            // printf("-------->reader with pid %d read fileContents: %s\n", getpid(), fileContents);
            // alarm(0);
            int remainingContentsSize = fileContentsSize;
            while (remainingContentsSize > 0) {
                memset(chunk, 0, bufferSize + 1);

                int returnValue, tempBufferSize, inProgress = 0;
                //printf("remaining contents size: %d\n", remainingContentsSize);
                if (remainingContentsSize < bufferSize) {
                    tempBufferSize = remainingContentsSize;
                } else {
                    tempBufferSize = bufferSize;
                }

                tryRead(fifoFd, chunk, tempBufferSize);

                // trySelect(fifoFd);
                // returnValue = read(fifoFd, chunk, tempBufferSize);
                // //printf("RETURN VALUE = %d", returnValue);
                // while (returnValue < tempBufferSize && returnValue != 0) {
                //     if (returnValue == -1) {
                //         perror("read error");
                //         handleExit(1, SIGUSR1);
                //     }
                //     //printf("+++++++++++++++++++++++++++++++++++++++++++++ reader with pid %d read %d bytes from pipe: %s\n", getpid(), returnValue, chunk);
                //     tempBufferSize -= returnValue;
                //     inProgress += returnValue;
                //     trySelect(fifoFd);
                //     returnValue = read(fifoFd, chunk + inProgress, tempBufferSize);
                // }

                // if (returnValue == 0) {  // EOF which means that writer failed and closed the pipe early
                //     handleExit(1, SIGUSR1);
                // }

                //printf("-------->reader with pid %d read a chunk of file %s: %s\n", getpid(), filePath, chunk);

                bytesRead += tempBufferSize;
                strcat(fileContents, chunk);
                remainingContentsSize -= bufferSize;
            }
        }

        // if (fileContentsSize == -1) {
        //     fprintf(logFileP, "Reader with pid %d received directory with path %s\n", getpid(), filePath);
        //     printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        // } else {
        //     fprintf(logFileP, "Reader with pid %d received file with path \"%s\" and contents \"%s\"\n", getpid(), filePath, fileContentsSize > 0 ? fileContents : "");
        //     printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        // }

        fprintf(logFileP, "Reader with pid %d received file with path \"%s\" and read %d bytes from fifo pipe\n", getpid(), filePath, bytesRead);
        //printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        fflush(logFileP);

        char clientIdToS[MAX_STRING_INT_SIZE];
        sprintf(clientIdToS, "%d", clientIdTo);
        char mirrorFilePath[strlen(filePath) + strlen(mirrorDirName) + strlen(clientIdToS) + 2], mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdToS) + 1];
        strcpy(mirrorFilePath, mirrorDirName);
        strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, clientIdToS);
        strcpy(mirrorIdDirPath, mirrorFilePath);
        // strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, filePath);
        // printf("reader read filepath: %s\n", filePath);
        // char dirOfFilePath[strlen(filePath) + strlen(mirrorDirName) + strlen(clientIdFromS) + 2];
        // strcpy(dirOfFilePath, mirrorFilePath);
        // char* tempS = dirOfFilePath;

        // while ((tempS = strchr(tempS, '/')) != NULL) {
        //     printf("in while\n");
        //     tempS = tempS + 1;
        //     if (strchr(tempS, '/') == NULL) {
        //         *(tempS-1) = '\0';
        //         break;
        //     }
        //     // tempS = strchr(tempS, '/');

        // }
        //printf("_________________mirrorFilePathCopy = %s\n", mirrorIdDirPath);

        if (!dirExists(mirrorIdDirPath)) {
            createDir(mirrorIdDirPath);
        }
        if (fileContentsSize > 0) {
            createAndWriteToFile(mirrorFilePath, fileContents);

            if (GPG_ENCRYPTION_ON) {
            char tempPath[strlen(mirrorFilePath) + 5];
            sprintf(tempPath, "%stemp", mirrorFilePath);
            decryptFile(mirrorFilePath, tempPath);
            removeFileOrDir(mirrorFilePath);
            renameFile(tempPath, mirrorFilePath);
        }
        } else if (fileContentsSize == 0) {
            createAndWriteToFile(mirrorFilePath, "");
        } else if (fileContentsSize == -1) {
            createDir(mirrorFilePath);
        }

        if (filePath != NULL) {
            free(filePath);
            filePath = NULL;
        }
        if (filePathSizeS != NULL) {
            free(filePathSizeS);
            filePathSizeS = NULL;
        }
        if (fileContentsSizeS != NULL) {
            free(fileContentsSizeS);
            fileContentsSizeS = NULL;
        }

        // filePathSizeS = malloc(3);
        // memcpy(filePathSizeS, "\0", 3);
        // alarm(30);
        readReturnValue = tryRead(fifoFd, &filePathSize, 2);
        fprintf(logFileP, "Reader with pid %d read 2 bytes of metadata from fifo pipe\n", getpid());
        // alarm(0);
        // filePathSize = atoi(filePathSizeS);
        // printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
    }

    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    freeFileList(&inputFileList);

    if (close(fifoFd) == -1) {
        perror("close failed");
        handleExit(1, SIGUSR2);
    }
    fifoFd = -1;

    return;
}

int main(int argc, char** argv) {
    printf("started reader!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    int clientIdFrom, clientIdTo, bufferSize;
    char *commonDirName, *mirrorDirName, *logFileName;

    handleArgs(argc, argv, &inputFileList, &clientIdFrom, &clientIdTo, &commonDirName, &mirrorDirName, &bufferSize, &logFileName, &clientPid);

    readerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);

    return 0;
}