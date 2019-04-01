#include "reader.h"

static char *filePath = NULL, *fileContents = NULL;

void handleArgsReader(int argc, char** argv, FileList** fileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
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

void handleSigAlarm(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Caught wrong signal instead of SIGALRM\n");
    }
    printf("Reader process with id %d caught SIGALRM, waited more than 30 seconds to read from pipe", getpid());

    if (filePath != NULL) {
        free(filePath);
        filePath = NULL;
    }
    if (fileContents != NULL) {
        free(fileContents);
        fileContents = NULL;
    }

    free(inputFileList);
    if (fclose(logFileP) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    kill(getppid(), SIGUSR1);

    exit(1);
}

void handleSigIntReader(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Caught wrong signal instead of SIGINT\n");
    }
    printf("Reader process with id %d caught SIGINT", getpid());

    if (filePath != NULL) {
        free(filePath);
        filePath = NULL;
    }
    if (fileContents != NULL) {
        free(fileContents);
        fileContents = NULL;
    }

    freeFileList(&inputFileList);

    exit(1);
}

void handleSignalsReader(int signal) {
    // Find out which signal we're handling
    switch (signal) {
        case SIGALRM:
            handleSigAlarm(signal);
            break;
        case SIGINT:
            handleSigIntReader(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName) {
    printf("started readerJob with pid %d\n", getpid());
    struct sigaction sigAction;

    // Setup the signal handler
    sigAction.sa_handler = &handleSignalsReader;

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

    fifoFd = open(fifo, O_RDONLY);

    int filePathSize = 0;
    int fileContentsSize = 0;

    if ((logFileP = fopen(logFileName, "a")) == NULL) {
        perror("fopen failed");
        exit(1);
    }

    alarm(30);
    int readReturnValue = tryRead(fifoFd, &filePathSize, 2);
    printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
    alarm(0);
    while (readReturnValue > 0 && filePathSize != 0) {
        filePath = (char*)malloc(filePathSize + 1);

        alarm(30);
        printf("-------->reader with pid %d BEFORE read filePath: %s\n", getpid(), filePath);

        tryRead(fifoFd, filePath, filePathSize);
        printf("-------->reader with pid %d read filePath: %s\n", getpid(), filePath);
        alarm(0);

        alarm(30);
        tryRead(fifoFd, &fileContentsSize, 4);
        printf("-------->reader with pid %d read fileContentsSize: %d\n", getpid(), fileContentsSize);
        alarm(0);

        if (fileContentsSize > 0) {
            fileContents = (char*)malloc(fileContentsSize + 1);
            alarm(30);
            tryRead(fifoFd, fileContents, fileContentsSize);
            printf("-------->reader with pid %d read fileContents: %s\n", getpid(), fileContents);
            alarm(0);
        }

        if (fileContentsSize == -1) {
            fprintf(logFileP, "Reader with pid %d received directory with path %s\n", getpid(), filePath);
            printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        } else {
            fprintf(logFileP, "Reader with pid %d received file with path \"%s\" and contents \"%s\"\n", getpid(), filePath, fileContentsSize > 0 ? fileContents : "");
            printf("READER WITH PID %d wrote to log!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n", getpid());
        }
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
        if (fileContents != NULL) {
            free(fileContents);
            fileContents = NULL;
        }

        alarm(30);
        readReturnValue = tryRead(fifoFd, &filePathSize, 2);
        printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
        alarm(0);
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

    handleArgsReader(argc, argv, &inputFileList, &clientIdFrom, &clientIdTo, &commonDirName, &mirrorDirName, &bufferSize, &logFileName);

    readerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);

    return 0;
}