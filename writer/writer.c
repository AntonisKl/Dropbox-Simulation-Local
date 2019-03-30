#include "writer.h"

void handleSigIntWriter(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Caught wrong signal instead of SIGINT\n");
    }
    printf("Writer process with id %d caught SIGINT\n", getpid());

    freeFileList(&inputFileList);

    exit(0);
}

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize) {
    printf("started writerJob with pid %d\n", getpid());

    struct sigaction sigAction;

    // Setup the signal handler
    sigAction.sa_handler = &handleSigIntWriter;

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
    char buffer[bufferSize];

    if (!fileExists(fifoFilePath)) {
        printf("pid %d, before fifo\n", getpid());
        mkfifo(fifo, 0666);
        printf("pid %d, after fifo\n", getpid());
    }

    fifoFd = open(fifo, O_WRONLY);

    File* curFile = inputFileList->firstFile;
    while (curFile != NULL) {
        int filePathSize = strlen(curFile->pathNoInputDir) + 1;
        tryWrite(fifoFd, &filePathSize, 2);
        char temp[PATH_MAX];
        printf("-------->writer with pid %d wrote filePathSize: %d\n", getpid(), filePathSize);
        memcpy(temp, curFile->pathNoInputDir, filePathSize + 1);
        printf("TEMP: %s\n\n", temp);
        tryWrite(fifoFd, curFile->pathNoInputDir, filePathSize);
        printf("-------->writer with pid %d wrote filePath: %s\n", getpid(), curFile->pathNoInputDir);
        tryWrite(fifoFd, &curFile->contentsSize, 4);
        printf("-------->writer with pid %d wrote fileContentsSize: %ld\n", getpid(), curFile->contentsSize);

        FILE* fp = fopen(curFile->path, "r");
        if (fp == NULL) {
            perror("fopen failed");
            exit(1);
        }
        while (fgets(buffer, bufferSize, fp) != NULL) {
            tryWrite(fifoFd, buffer, bufferSize);
            printf("-------->writer with pid %d wrote a chunk of file %s: %s\n", getpid(), curFile->pathNoInputDir, buffer);
        }
        // write(fifoFd, buffer, bufferSize);
        // printf("-------->writer with pid %d wrote a chunk of file %s: %s\n", getpid(), curFile->path, buffer);
        if (fclose(fp) == EOF) {
            perror("fclose failed");
            kill(getppid(), SIGUSR1);
            exit(1);
        }
        curFile = curFile->nextFile;
    }
    int end = 0;
    tryWrite(fifoFd, &end, 2);

    freeFileList(&inputFileList);
    return;
}