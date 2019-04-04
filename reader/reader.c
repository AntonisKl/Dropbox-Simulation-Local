#include "reader.h"

static char *filePath = NULL, *filePathSizeS = NULL, *fileContentsSizeS = NULL;

void handleArgs(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 8) {
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
    if (!strcmp(argv[1], EMPTY_FILE_LIST_STRING)) {
        (*fileList) = initFileList();
    } else {
        (*fileList) = stringToFileList(argv[1]);
    }
    (*clientIdFrom) = atoi(argv[2]);
    (*clientIdTo) = atoi(argv[3]);
    (*commonDirName) = argv[4];
    (*mirrorDirName) = argv[5];
    (*bufferSize) = atoi(argv[6]);
    (*logFileName) = argv[7];

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

    freeFileList(&inputFileList);
    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    kill(getppid(), SIGUSR1);

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

    handleExit(1);
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Reader caught wrong signal instead of SIGINT");
    }
    printf("Reader process with id %d caught SIGINT\n", getpid());

    handleExit(1);
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

    printErrorLn("BEEEEEEEEEEEEEEEEEEEEEEEEEEEEEFORE SELECT");
    int selectRetValue = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timeStruct);
    printErrorLn("AAAAAAAAAAAAAAAAAAAAAAFTER SELECT");

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
            raiseIntAndExit(1);
        }

        printf("+++++++++++++++++++++++++++++++++++++++++++++ reader with pid %d read %d bytes from pipe\n", getpid(), returnValue);
        tempBufferSize -= returnValue;
        progress += returnValue;
        trySelect(fd);
        returnValue = read(fd, buffer + progress, tempBufferSize);
    }
    printf("======================================================== reader with pid %d read %d bytes from pipe\n", getpid(), returnValue);

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

    int fifoFd;
    char* fifo = fifoFilePath;

    if (!fileExists(fifoFilePath)) {
        printf("pid %d, before fifo\n", getpid());
        mkfifo(fifo, 0666);
        printf("pid %d, after fifo\n", getpid());
    }

    fifoFd = open(fifo, O_RDONLY | O_NONBLOCK);

    int filePathSize = 0, fileContentsSize = 0;

    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
        exit(1);
    }

    filePathSizeS = malloc(3);

    // alarm(30);
    int readReturnValue = tryRead(fifoFd, filePathSizeS, 2);
    // alarm(0);
    filePathSize = atoi(filePathSizeS);
    printf("-------->reader with pid %d read filePathSize: %s\n", getpid(), filePathSizeS);

    while (readReturnValue > 0 && filePathSize != 0) {
        filePath = (char*)malloc(filePathSize + 1);

        // alarm(30);
        printf("-------->reader with pid %d BEFORE read filePath: %s\n", getpid(), filePath);
        tryRead(fifoFd, filePath, filePathSize);
        printf("-------->reader with pid %d read filePath: %s\n", getpid(), filePath);
        // alarm(0);

        fileContentsSizeS = malloc(5);
        // alarm(30);
        tryRead(fifoFd, fileContentsSizeS, 4);
        // alarm(0);
        fileContentsSize = atoi(fileContentsSizeS);
        printf("-------->reader with pid %d read fileContentsSize: %d\n", getpid(), fileContentsSize);

        char fileContents[fileContentsSize + 1];
        int bytesRead = 0;
        if (fileContentsSize > 0) {
            strcpy(fileContents, "");
            char chunk[bufferSize];
            // alarm(30);
            // tryRead(fifoFd, fileContents, fileContentsSize);
            // printf("-------->reader with pid %d read fileContents: %s\n", getpid(), fileContents);
            // alarm(0);
            int remainingContentsSize = fileContentsSize;
            while (remainingContentsSize > 0) {
                int returnValue, tempBufferSize = bufferSize, inProgress = 0;
                printf("remaining contents size: %d\n", remainingContentsSize);

                returnValue = read(fifoFd, chunk, tempBufferSize);
                while (returnValue < tempBufferSize && returnValue != 0) {
                    if (returnValue == -1) {
                        perror("read error");
                        raiseIntAndExit(1);
                    }
                    printf("+++++++++++++++++++++++++++++++++++++++++++++ reader with pid %d read %d bytes from pipe\n", getpid(), returnValue);
                    tempBufferSize -= returnValue;
                    inProgress += returnValue;
                    returnValue = read(fifoFd, chunk + inProgress, tempBufferSize);
                }

                if (returnValue == 0)
                    break;

                bytesRead += returnValue;

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
        printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        fflush(logFileP);

        char clientIdFromS[MAX_STRING_INT_SIZE];
        sprintf(clientIdFromS, "%d", clientIdFrom);
        char mirrorFilePath[strlen(filePath) + strlen(mirrorDirName) + strlen(clientIdFromS) + 2], mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdFromS) + 1];
        strcpy(mirrorFilePath, mirrorDirName);
        strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, clientIdFromS);
        strcpy(mirrorIdDirPath, mirrorFilePath);
        strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, filePath);

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
        printf("_________________mirrorFilePathCopy = %s\n", mirrorIdDirPath);

        if (!dirExists(mirrorIdDirPath)) {
            createDir(mirrorIdDirPath);
        }
        if (fileContentsSize > 0) {
            createAndWriteToFile(mirrorFilePath, fileContents);
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

        filePathSizeS = malloc(3);
        // alarm(30);
        readReturnValue = tryRead(fifoFd, filePathSizeS, 2);
        // alarm(0);
        filePathSize = atoi(filePathSizeS);
        printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
    }

    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    freeFileList(&inputFileList);
    return;
}

int main(int argc, char** argv) {
    printf("started reader!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    int clientIdFrom, clientIdTo, bufferSize;
    char *commonDirName, *mirrorDirName, *logFileName;

    handleArgs(argc, argv, &inputFileList, &clientIdFrom, &clientIdTo, &commonDirName, &mirrorDirName, &bufferSize, &logFileName);

    readerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);

    return 0;
}