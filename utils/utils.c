#include "utils.h"

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
        perror("fopen failed: ");
        exit(1);
    }

    fprintf(file, "%s", contents);

    if (fclose(file) == EOF) {
        perror("fclose failed: ");
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

            addFileToFileList(fileList, pathNoInputDirName, -1, DIRECTORY);

            printf("Handled directory: %*s[%s]\n", indent, "", entry->d_name);
            populateFileList(fileList, path, pathNoInputDirName, indent + 2);
        } else {  // if it is a file

            // if (parentDir == NULL) {
            //     parentDir = dirTree->rootNode;
            // }

            // addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, File, NULL);

            addFileToFileList(fileList, pathNoInputDirName, curStat.st_size - 1, REGULAR_FILE);  // curStat.st_size - 1: because when file is empty, its size is 1 byte by default

            printf("Handled file: %*s- %s\n", indent, "", entry->d_name);
        }
    }

    closedir(dir);
}

char isIdFile(char* fileName) {
    char curNameCopy[NAME_MAX];
    strcpy(curNameCopy, fileName);
    char* token = strtok(curNameCopy, ",");
    if (token == NULL)
        return 0;

    token = strtok(NULL, "\n");  // until the end of the file's name

    if (strcmp(token, "id"))  // not a .id file
        return 0;

    return 1;  // is an .id file
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
    char fifoFileName[MAX_FIFO_FILE_NAME];
    buildFifoFileName(&fifoFileName, clientIdTo, clientIdFrom);

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];
    strcpy(fifoFilePath, commonDirName);
    strcat(fifoFilePath, "/");
    strcat(fifoFilePath, fifoFileName);

    int fifoFd;
    char* fifo = fifoFilePath;

    if (!fileExists(fifoFilePath)) {
        mkfifo(fifo, 0666);
    }

    fifoFd = open(fifo, O_RDONLY);

    int filePathSize, fileContentsSize;
    char *filePath = NULL, *fileContents = NULL;

    int readReturnValue = read(fifoFd, &filePathSize, 2);
    while (readReturnValue > 0 && filePathSize != 0) {
        filePath = (char*)malloc(filePathSize);

        read(fifoFd, filePath, filePathSize);
        read(fifoFd, &fileContentsSize, 4);

        fileContents = (char*)malloc(fileContentsSize);

        read(fifoFd, fileContents, fileContentsSize);

        char clientIdFromS[MAX_STRING_INT_SIZE];
        sprintf(clientIdFromS, "%d", clientIdFrom);
        char mirrorFilePath[strlen(filePath) + strlen(mirrorDirName) + strlen(clientIdFromS) + 2];
        strcpy(mirrorFilePath, mirrorDirName);
        strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, clientIdFromS);
        strcat(mirrorFilePath, "/");
        strcat(mirrorFilePath, filePath);
        createAndWriteToFile(mirrorFilePath, fileContents);

        if (filePath != NULL) {
            free(filePath);
            filePath = NULL;
        }
        if (fileContents != NULL) {
            free(fileContents);
            fileContents = NULL;
        }

        readReturnValue = read(fifoFd, &filePathSize, 2);
    }
}

void writerJob(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize) {
    char fifoFileName[MAX_FIFO_FILE_NAME];
    buildFifoFileName(&fifoFileName, clientIdFrom, clientIdTo);

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];
    strcpy(fifoFilePath, commonDirName);
    strcat(fifoFilePath, "/");
    strcat(fifoFilePath, fifoFileName);

    int fifoFd;
    char* fifo = fifoFilePath;
    char buffer[bufferSize];

    if (!fileExists(fifoFilePath)) {
        mkfifo(fifo, 0666);
    }

    fifoFd = open(fifo, O_WRONLY);

    File* curFile = inputFileList->firstFile;
    while (curFile != NULL) {
        int filePathSize = strlen(curFile->path);
        write(fifoFd, &filePathSize, 2);
        write(fifoFd, curFile->path, filePathSize);
        write(fifoFd, &curFile->contentsSize, 4);

        FILE* fp = fopen(curFile->path, "r");
        while (fgets(buffer, bufferSize, fp) != NULL) {
            write(fifoFd, buffer, bufferSize);
        }

        curFile = curFile->nextFile;
    }
    int end = 0;
    write(fifoFd, &end, 2);
}

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork error");
        raiseIntAndExit(1);
    } else if (pid == 0) {
        readerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize);
        exit(0);  // TODO: before exit raise signals to parent
    } else {
        pid = fork();
        if (pid == -1) {
            perror("fork error");
            raiseIntAndExit(1);
        } else if (pid == 0) {
            writerJob(inputFileList, clientIdFrom, clientIdTo, commonDirName, bufferSize);
            exit(0);  // TODO: before exit raise signals to parent
        }
    }
}

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(commonDirName)) == NULL) {
        perror("Could not open input directory");
        exit(1);
    }

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !isIdFile(entry->d_name))
            continue;

        snprintf(path, PATH_MAX, "%s/%s", commonDirName, entry->d_name);
        struct stat curStat;
        stat(path, &curStat);

        if (S_ISREG(curStat.st_mode)) {  // if it is a file
            printf("Handling common id file: %s\n", entry->d_name);

            createReaderAndWriter(inputFileList, clientId, atoi(strtok(entry->d_name, ".")), commonDirName, mirrorDirName, bufferSize);
        }
    }

    closedir(dir);
}

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId) {
    struct inotify_event iNotifyEvent;
    int readRet, wd;
    uint32_t mask, nameLen;
    char* curName = NULL;

    int iNotifyFd = inotify_init();
    inotify_add_watch(iNotifyFd, commonDirName, IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF);

    while ((readRet = read(iNotifyFd, &iNotifyEvent, sizeof(struct inotify_event) + NAME_MAX + 1)) != -1) {
        wd = iNotifyEvent.wd;
        mask = iNotifyEvent.mask;
        nameLen = iNotifyEvent.len;

        if (nameLen > 0) {
            curName = (char*)malloc(nameLen);

            strcpy(curName, iNotifyEvent.name);

            printf("New event->wd: %d, mask: %d, name: %s\n", wd, mask, iNotifyEvent.name);
        } else {
            printf("New event->wd: %d, mask: %d\n", wd, mask);
        }

        char curPathName[PATH_MAX];
        struct stat curStat;

        if (nameLen > 0 && mask != IN_DELETE) {
            if (!isIdFile(curName))
                continue;

            // form the path name of current file
            strcpy(curPathName, commonDirName);
            strcat(curPathName, curName);

            stat(curPathName, &curStat);
        }

        switch (mask) {
            case IN_CREATE: {
                printf("Create event\n");

                break;
            }
            case IN_CLOSE_WRITE: {
                printf("Close-write event\n");

                createReaderAndWriter(inputFileList, clientId, atoi(strtok(curName, ".")), commonDirName, mirrorDirName, bufferSize);

                break;
            }
        }

        // free curName only if it was malloced
        if (curName != NULL) {
            free(curName);
            curName = NULL;
        }
    }

    // error in read
    if (readRet == -1) {
        perror("read inotify desc error");
        exit(1);
    }
}