#include "utils.h"

static char *filePath = NULL, *fileContents = NULL;
static int attemptsNum = 0;

void raiseIntAndExit(int num) {
    raise(SIGINT);
    exit(num);
}

void printErrorLn(char* s) {
    printf(ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, s);

    return;
}

void printErrorLnExit(char* s) {
    printf(ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, s);
    raiseIntAndExit(1);
}

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 13) {
        printErrorLnExit("Invalid arguments. Exiting...");
    }

    // validate input arguments one by one
    if (strcmp(argv[1], "-n") == 0) {
        (*clientId) = atoi(argv[2]);
        // if ((*clientId) <= 0) {
        //     printError("Invalid arguments\nExiting...\n");
        //     raiseIntAndExit(1);
        // }
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    if (strcmp(argv[3], "-c") == 0) {
        (*commonDirName) = argv[4];
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    if (strcmp(argv[5], "-i") == 0) {
        (*inputDirName) = argv[6];
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    if (strcmp(argv[7], "-m") == 0) {
        (*mirrorDirName) = argv[8];
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    if (strcmp(argv[9], "-b") == 0) {
        (*bufferSize) = atoi(argv[10]);
        if ((*bufferSize) <= 0) {
            printErrorLnExit("Invalid arguments\nExiting...");
        }
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    if (strcmp(argv[11], "-l") == 0) {
        (*logFileName) = argv[12];
    } else {
        printErrorLnExit("Invalid arguments\nExiting...");
    }

    return;
}

char dirExists(char* dirName) {
    char returnValue = 0;
    DIR* dir = opendir(dirName);
    if (dir) {
        returnValue = 1;
        /* Directory exists. */
        closedir(dir);
    }

    return returnValue;
}

char fileExists(char* fileName) {
    if (access(fileName, F_OK) != -1) {
        return 1;
    }

    return 0;
}

void createDir(char* dirPath) {
    int pid = fork();
    if (pid == 0) {
        char* args[] = {"mkdir", "-p", dirPath, NULL};
        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(1);
        }
    } else if (pid == -1) {
        perror("fork error");
        exit(1);
    } else {
        wait(NULL);
    }
}

void buildIdFileName(char (*idFilePath)[], char* commonDirName, int clientId) {
    // char idFilePath[strlen(commonDirName) + 1 + strlen(clientId) + 4];
    strcpy(*idFilePath, commonDirName);
    strcat(*idFilePath, "/");
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);
    strcat(*idFilePath, clientIdS);
    strcat(*idFilePath, ".id");
}

void createAndWriteToFile(char* fileName, char* contents) {
    FILE* file = fopen(fileName, "w");
    if (file == NULL) {
        perror("fopen failed");
        exit(1);
    }

    fprintf(file, "%s", contents);

    fflush(file);
    if (fclose(file) == EOF) {
        perror("fclose failed");
        exit(1);
    }
    return;
}

void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]) {
    if (!dirExists(inputDirName)) {
        printErrorLnExit("Input directory does not exist");
    }

    if (dirExists(mirrorDirName)) {
        printErrorLnExit("Mirror directory already exists");
    }

    // char idFilePath[strlen(commonDirName) + 1 + strlen(clientId) + 4];
    buildIdFileName(idFilePath, commonDirName, clientId);

    if (fileExists((char*)idFilePath)) {
        printErrorLnExit("Id common file already exists");
    }

    return;
}

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent) {
    DIR* dir;
    struct dirent* entry;
    printf("start of populate\n");

    if ((dir = opendir(inputDirName)) == NULL) {
        perror("Could not open input directory");
        exit(1);
    }

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX], pathNoInputDirName[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, PATH_MAX, "%s/%s", inputDirName, entry->d_name);
        snprintf(pathNoInputDirName, PATH_MAX, "%s/%s", pathWithoutInputDirName, entry->d_name);
        struct stat curStat;
        stat(path, &curStat);

        if (!S_ISREG(curStat.st_mode)) {  // if it is a directory
            // if (parentDir == NULL) {
            //     parentDir = dirTree->rootNode;
            // }

            // TreeNode* addedDir = addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, Directory, NULL);
            printf("dir: %s\n", entry->d_name);
            addFileToFileList(fileList, pathNoInputDirName, path, -1, DIRECTORY);

            printf("Handled directory: %*s[%s]\n", indent, "", entry->d_name);
            populateFileList(fileList, path, pathNoInputDirName, indent + 2);
        } else {  // if it is a file

            // if (parentDir == NULL) {
            //     parentDir = dirTree->rootNode;
            // }

            // addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, File, NULL);

            addFileToFileList(fileList, pathNoInputDirName, path, curStat.st_size, REGULAR_FILE);  // curStat.st_size - 1: because when file is empty, its size is 1 byte by default

            printf("Handled file: %*s- %s\n", indent, "", entry->d_name);
        }
    }
    printf("end of populate\n");

    closedir(dir);
}

char isIdFile(char* fileName) {
    char curNameCopy[NAME_MAX];
    strcpy(curNameCopy, fileName);
    char* token = strtok(curNameCopy, ".");
    if (token == NULL)
        return 0;

    token = strtok(NULL, "\n");  // until the end of the file's name

    if (token == NULL)
        return 0;

    if (strcmp(token, "id"))  // not a .id file
        return 0;
    printf("last token1: %s\n", token);

    return 1;  // is an .id file
}

char isSameIdFile(char* fileName, int clientId) {
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);

    char curNameCopy[NAME_MAX];
    strcpy(curNameCopy, fileName);
    char* token1 = strtok(curNameCopy, ".");
    if (token1 == NULL)
        return 0;

    char* token2 = strtok(NULL, "\n");  // until the end of the file's name

    if (token2 == NULL)
        return 0;

    if (strcmp(token2, "id"))  // not a .id file
        return 0;

    if (strcmp(token1, clientIdS))
        return 0;

    return 1;
}

void handleSigAlarm(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Caught wrong signal instead of SIGALRM\n");
    }
    printf("Reader process with id %d caught SIGALRM", getpid());

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

void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE];
    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);

    strcpy(*fifoFileName, clientIdFromS);
    strcat(*fifoFileName, "_to_");
    strcat(*fifoFileName, clientIdToS);
    strcat(*fifoFileName, ".fifo");
}

void readerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize) {
    printf("started readerJob with pid %d\n", getpid());

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
    struct sigaction sigAction;

    // Setup the sighub handler
    sigAction.sa_handler = &handleSigAlarm;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGALRM);

    if (sigaction(SIGALRM, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    alarm(30);
    int readReturnValue = read(fifoFd, &filePathSize, 2);
    printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
    alarm(0);
    while (readReturnValue > 0 && filePathSize != 0) {
        filePath = (char*)malloc(filePathSize + 1);

        alarm(30);
        printf("-------->reader with pid %d BEFORE read filePath: %s\n", getpid(), filePath);

        read(fifoFd, filePath, filePathSize);
        printf("-------->reader with pid %d read filePath: %s\n", getpid(), filePath);
        alarm(0);

        alarm(30);
        read(fifoFd, &fileContentsSize, 4);
        printf("-------->reader with pid %d read fileContentsSize: %d\n", getpid(), fileContentsSize);
        alarm(0);

        if (fileContentsSize > 0) {
            fileContents = (char*)malloc(fileContentsSize + 1);
            alarm(30);
            read(fifoFd, fileContents, fileContentsSize);
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
        readReturnValue = read(fifoFd, &filePathSize, 2);
        printf("-------->reader with pid %d read filePathSize: %d\n", getpid(), filePathSize);
        alarm(0);
    }
}

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize) {
    printf("started writerJob with pid %d\n", getpid());

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
        write(fifoFd, &filePathSize, 2);
        char temp[PATH_MAX];
        printf("-------->writer with pid %d wrote filePathSize: %d\n", getpid(), filePathSize);
        memcpy(temp, curFile->pathNoInputDir, filePathSize + 1);
        printf("TEMP: %s\n\n", temp);
        write(fifoFd, curFile->pathNoInputDir, filePathSize);
        printf("-------->writer with pid %d wrote filePath: %s\n", getpid(), curFile->pathNoInputDir);
        write(fifoFd, &curFile->contentsSize, 4);
        printf("-------->writer with pid %d wrote fileContentsSize: %ld\n", getpid(), curFile->contentsSize);

        FILE* fp = fopen(curFile->path, "r");
        if (fp == NULL) {
            perror("fopen failed");
            exit(1);
        }
        while (fgets(buffer, bufferSize, fp) != NULL) {
            write(fifoFd, buffer, bufferSize);
            printf("-------->writer with pid %d wrote a chunk of file %s: %s\n", getpid(), curFile->pathNoInputDir, buffer);
        }
        // write(fifoFd, buffer, bufferSize);
        // printf("-------->writer with pid %d wrote a chunk of file %s: %s\n", getpid(), curFile->path, buffer);
        if (fclose(fp) == EOF) {
            perror("fclose failed");
            exit(1);
        }
        curFile = curFile->nextFile;
    }
    int end = 0;
    write(fifoFd, &end, 2);
}

void handleSigUsr1(int signal);

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize) {
    struct sigaction sigAction;

    // Setup the sighub handler
    sigAction.sa_handler = &handleSigUsr1;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGUSR1);

    if (sigaction(SIGUSR1, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    pid_t readerPid = fork();
    if (readerPid == -1) {
        perror("fork error");
        raiseIntAndExit(1);
    } else if (readerPid == 0) {
        readerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize);
        exit(0);  // TODO: before exit raise signals to parent
    } else {
        int writerPid = fork();
        if (writerPid == -1) {
            perror("fork error");
            raiseIntAndExit(1);
        } else if (writerPid == 0) {
            writerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, bufferSize);
            exit(0);  // TODO: before exit raise signals to parent
        } else {
            waitpid(readerPid, NULL, 0);
        }
    }
}

void handleSigUsr1(int signal) {
    if (signal != SIGUSR1) {
        printErrorLn("Caught wrong signal instead of SIGUSR1\n");
    }
    printf("Reader process with id %d caught SIGUSR1", getpid());

    if (attemptsNum < 3) {
        createReaderAndWriter(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize);
        attemptsNum++;
    }
}

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(commonDirName)) == NULL) {
        perror("Could not open input directory");
        exit(1);
    }

    printf("in initial sync");

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        printf("entry name: %s\n", entry->d_name);
        char path[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !isIdFile(entry->d_name) || isSameIdFile(entry->d_name, clientId))
            continue;

        snprintf(path, PATH_MAX, "%s/%s", commonDirName, entry->d_name);
        struct stat curStat;
        stat(path, &curStat);

        if (S_ISREG(curStat.st_mode)) {  // if it is a file
            printf("Handling common id file: %s\n", entry->d_name);

            clientIdFrom = clientId;
            clientIdTo = atoi(strtok(entry->d_name, "."));
            createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize);
        }
    }
    printf("end of initial sync\n");
    closedir(dir);
}

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId) {
    struct inotify_event iNotifyEvent;
    int readRet, wd;
    uint32_t mask, nameLen;
    char curName[NAME_MAX], prevName[NAME_MAX];
    strcpy(prevName, "");

    int iNotifyFd = inotify_init();
    inotify_add_watch(iNotifyFd, commonDirName, IN_CREATE | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_DELETE | IN_DELETE_SELF);

    while ((readRet = read(iNotifyFd, &iNotifyEvent, sizeof(struct inotify_event) + NAME_MAX + 1)) != -1) {
        wd = iNotifyEvent.wd;
        mask = iNotifyEvent.mask;
        nameLen = iNotifyEvent.len;

        if (nameLen > 0) {
            // curName = (char*)malloc(nameLen);
            if (strcmp(prevName, "") && !strcmp(iNotifyEvent.name, prevName))
                continue;

            strcpy(curName, iNotifyEvent.name);

            if (mask == IN_CLOSE_WRITE || mask == IN_CLOSE_WRITE || (mask == (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE))) {
                strcpy(prevName, curName);
            }

            printf("New event->wd: %d, mask: %d, name: %s\n", wd, mask, iNotifyEvent.name);
        } else {
            printf("New event->wd: %d, mask: %d\n", wd, mask);
        }

        char curPathName[PATH_MAX];
        struct stat curStat;

        if (nameLen > 0 && mask != IN_DELETE) {
            if (!isIdFile(curName) || isSameIdFile(curName, clientId))
                continue;

            // form the path name of current file
            strcpy(curPathName, commonDirName);
            strcat(curPathName, curName);

            stat(curPathName, &curStat);
        }

        switch (mask) {
            case IN_CREATE: {
                printf("Create event\n");
                // clientIdFrom = clientId;
                // clientIdTo = atoi(strtok(curName, "."));
                // createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize);

                break;
            }
            case IN_CLOSE_WRITE: {
                printf("Close-write event\n");
                clientIdFrom = clientId;
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize);

                break;
            }
            case IN_CLOSE_NOWRITE: {
                printf("Close-nowrite event\n");
                clientIdFrom = clientId;
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize);
                break;
            }
            case IN_CLOSE_WRITE | IN_CLOSE_NOWRITE: {
                printf("Close event\n");
                clientIdFrom = clientId;
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize);
                break;
            }
        }

        // free curName only if it was malloced
        // if (curName != NULL) {
        //     free(curName);
        //     curName = NULL;
        // }
    }

    // error in read
    if (readRet == -1) {
        perror("read inotify desc error");
        exit(1);
    }
}