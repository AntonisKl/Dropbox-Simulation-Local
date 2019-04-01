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

    return;
}

void removeFileOrDir(char* path) {
    int pid = fork();
    if (pid == 0) {
        char* args[] = {"rm", "-rf", path, NULL};
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
    return;
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

void tryWrite(int fd, const void* buffer, int bufferSize) {
    if (write(fd, buffer, bufferSize) == -1) {
        perror("write error");
        kill(getppid(), SIGUSR1);
        raiseIntAndExit(1);
    }
    return;
}

int tryRead(int fd, void* buffer, int bufferSize) {
    int returnValue = read(fd, buffer, bufferSize);
    if (returnValue == -1) {
        perror("read error");
        kill(getppid(), SIGUSR1);
        raiseIntAndExit(1);
    }
    return returnValue;
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

void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE];
    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);

    strcpy(*fifoFileName, clientIdFromS);
    strcat(*fifoFileName, "_to_");
    strcat(*fifoFileName, clientIdToS);
    strcat(*fifoFileName, ".fifo");
}

void fileListToString(FileList* fileList, char (*fileListS)[]) {
    printf("heey\n\n");
    // char* fileListS = (char*)malloc(fileList->size * MAX_FILE_LIST_NODE_STRING_SIZE);
    strcpy(*fileListS, "");

    File* curFile = fileList->firstFile;
    while (curFile != NULL) {
        // char* contentsSizeS, *typeS;
        // sprintf(contentsSizeS, "%ld", curFile->contentsSize);
        // sprintf(typeS, "%d", curFile->type);
        printf("in while\n");
        sprintf(*fileListS, "%s%s$%s$%ld$%d&",* fileListS, curFile->pathNoInputDir, curFile->path, curFile->contentsSize, curFile->type);

        curFile = curFile->nextFile;
    }

    if (!strcmp(*fileListS, "")) {
        // fileListS = (char*)malloc(3);
        strcpy(*fileListS, EMPTY_FILE_LIST_STRING);
    }

    return;
}

FileList* stringToFileList(char* fileListS) {
    FileList* fileList = initFileList();

    char* fileToken = strtok_r(fileListS, "&", &fileListS);
    while (fileToken != NULL) {
        char fileS[MAX_FILE_LIST_NODE_STRING_SIZE], *fieldToken, pathNoInputDir[PATH_MAX], path[PATH_MAX], *endPtr;
        off_t contentsSize;
        FileType type;

        strcpy(fileS, fileToken);

        fieldToken = strtok(fileS, "$");
        strcpy(pathNoInputDir, fieldToken);

        fieldToken = strtok(NULL, "$");
        strcpy(path, fieldToken);

        fieldToken = strtok(NULL, "$");
        contentsSize = strtol(fieldToken, &endPtr, 10);

        fieldToken = strtok(NULL, "$");
        type = (FileType)atoi(fieldToken);

        addFileToFileList(fileList, pathNoInputDir, path, contentsSize, type);

        fileToken = strtok_r(fileListS, "&", &fileListS);
    }

    return fileList;
}

void execReader(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE], bufferSizeS[MAX_STRING_INT_SIZE], fileListS[inputFileList->size * MAX_FILE_LIST_NODE_STRING_SIZE];
    printf("---------------------------------------- %d\n", clientIdFrom);
    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);
    sprintf(bufferSizeS, "%d", bufferSize);

    fileListToString(inputFileList, &fileListS);

    char* args[] = {"exe/reader", fileListS, clientIdFromS, clientIdToS, commonDirName, mirrorDirName, bufferSizeS, logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec reader failed\n");
    }

    return;
}

void execWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE], bufferSizeS[MAX_STRING_INT_SIZE], fileListS[inputFileList->size * MAX_FILE_LIST_NODE_STRING_SIZE];

    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);
    sprintf(bufferSizeS, "%d", bufferSize);
    fileListToString(inputFileList, &fileListS);

    char* args[] = {"exe/writer", fileListS, clientIdFromS, clientIdToS, commonDirName, bufferSizeS, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec writer failed\n");
    }

    return;
}