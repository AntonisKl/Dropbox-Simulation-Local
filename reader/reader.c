#include "reader.h"

static char *filePath = NULL, *fileContents = NULL;

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

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize) {
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

    freeFileList(&inputFileList);
    return;
}
