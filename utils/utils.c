#include "utils.h"

void printErrorLn(char* s) {
    printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", s);

    return;
}

void printErrorLnExit(char* s) {
    printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET "\n", s);
    raise(SIGINT);

    return;
}

char dirExists(char* dirName) {
    char returnValue = 0;
    DIR* dir = opendir(dirName);
    if (dir) {
        // directory exists
        returnValue = 1;
        closedir(dir);
    }

    // directory does not exist
    return returnValue;
}

char fileExists(char* fileName) {
    if (access(fileName, F_OK) != -1) {
        // file exists
        return 1;
    }

    // file does not exist
    return 0;
}

void createDir(char* dirPath) {
    int pid = fork();
    if (pid == 0) {
        char* args[] = {"mkdir", "-p", dirPath, NULL};  // -p parameter is used to create the whole directory structure if it does not exist
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

void renameFile(char* pathFrom, char* pathTo) {
    int pid = fork();
    if (pid == 0) {
        char* args[] = {"mv", pathFrom, pathTo, NULL};
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
    strcpy(*idFilePath, commonDirName);
    strcat(*idFilePath, "/");
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);
    strcat(*idFilePath, clientIdS);
    strcat(*idFilePath, ".id");

    return;
}

void createAndWriteToFile(char* fileName, char* contents) {
    FILE* file = fopen(fileName, "w");  // overwrite if file exists
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

char isIdFile(char* fileName) {
    char curNameCopy[NAME_MAX];
    strcpy(curNameCopy, fileName);  // make a copy of fileName to manipulate it with strtok

    char* token = strtok(curNameCopy, ".");
    if (token == NULL)
        return 0;

    token = strtok(NULL, "\n");  // until the end of the file's name

    if (token == NULL)
        return 0;

    if (strcmp(token, "id"))  // not a .id file
        return 0;

    return 1;  // is a .id file
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

    if (strcmp(token1, clientIdS))  // not the id file of client with id clientId
        return 0;

    return 1;  // is the id file of client with id clientId
}

void buildFifoFileName(char (*fifoFileName)[], int clientIdFrom, int clientIdTo) {
    sprintf(*fifoFileName, "%d_to_%d.fifo", clientIdFrom, clientIdTo);

    return;
}

void fileListToString(FileList* fileList, char **fileListS) {
    strcpy(*fileListS, "");

    File* curFile = fileList->firstFile;
    while (curFile != NULL) {
        // convert fileList's node to string and concatenate it to fileListS
        sprintf(*fileListS, "%s%s$%ld$%d&", *fileListS, curFile->path, curFile->contentsSize, curFile->type);

        curFile = curFile->nextFile;
    }

    if (!strcmp(*fileListS, "")) {
        // if file list is empty add the symbol that represents an empty file list string
        strcpy(*fileListS, EMPTY_FILE_LIST_STRING);
    }

    return;
}

FileList* stringToFileList(char* fileListS) {
    FileList* fileList = initFileList();

    char* fileToken = strtok_r(fileListS, "&", &fileListS);
    while (fileToken != NULL) {
        char fileS[MAX_FILE_LIST_NODE_STRING_SIZE], *fieldToken, path[PATH_MAX], *endPtr;
        off_t contentsSize;
        FileType type;

        strcpy(fileS, fileToken);

        fieldToken = strtok(fileS, "$");
        strcpy(path, fieldToken);

        fieldToken = strtok(NULL, "$");
        contentsSize = strtol(fieldToken, &endPtr, 10);

        fieldToken = strtok(NULL, "$");
        type = (FileType)atoi(fieldToken);

        addFileToFileList(fileList, path, contentsSize, type);

        fileToken = strtok_r(fileListS, "&", &fileListS);
    }

    return fileList;
}

void createGpgKeyDetailsFile(int clientId, char (*fileName)[]) {
    sprintf(*fileName, "%s%d", "tmp/KeyDetails", clientId);

    char contents[MIN_KEY_DETAILS_FILE_SIZE + (3 * MAX_STRING_INT_SIZE)];
    // use clientId as name, email and passphrase
    sprintf(contents, "%s%d%s%d%s%d%s", "Key-Type: default\nSubkey-Type: default\nName-Real: ", clientId, "\nName-Email: ", clientId, "@example.com\nExpire-Date: 0\nPassphrase: ", clientId, "\n%commit\n%echo done");

    createAndWriteToFile(*fileName, contents);

    return;
}

void generateGpgKey(char* keyDetailsPath) {
    printf("Generating gpg key-pair with details file with path: %s\n", keyDetailsPath);
    int pid = fork();
    if (pid == 0) {
        char* args[] = {"gpg", "--always-trust", "--batch", "--generate-key", keyDetailsPath, NULL};
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

void importGpgPublicKey(char* filePath) {
    int pid = fork();
    if (pid == 0) {
        // --no-verbose parameter is used for no output unless something goes wrong
        char* args[] = {"gpg", "--always-trust", "--quiet", "--no-verbose", "--import", filePath, NULL};
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

void exportGpgPublicKey(char* outputFilePath, int clientId) {
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);
    char nameRealS[strlen(clientIdS) + 3];
    sprintf(nameRealS, "%s", clientIdS);

    printf("Exporting public key of name-real: %s in file with path: %s\n", nameRealS, outputFilePath);
    int pid = fork();
    if (pid == 0) {
        // --armor parameter is used to export public key in ASCII encoding
        char* args[] = {"gpg", "--always-trust", "--quiet", "--no-verbose", "--armor", "--output", outputFilePath, "--export", nameRealS, NULL};
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

void encryptFile(char* filePath, int recipientClientId, char* outputFilePath) {
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", recipientClientId);
    char recipientNameRealS[strlen(clientIdS) + 3];
    sprintf(recipientNameRealS, "%s", clientIdS);

    // printf("Encrypting file with path: %s for recipient with id: %s and writing in file with path:%s\n", filePath, recipientNameRealS, outputFilePath);
    int pid = fork();
    if (pid == 0) {
        // --armor parameter is used to encrypt a file and produce output that uses ASCII encoding
        char* args[] = {"gpg", "--always-trust", "--yes", "--quiet", "--no-verbose", "--output", outputFilePath, "--recipient", recipientNameRealS, "--armor", "--encrypt", filePath, NULL};
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

void decryptFile(char* filePath, char* outputFilePath, int clientId) {
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);
    // printf("Decrypting file with path: %s and writing in file with path:%s\n", filePath, outputFilePath);
    int pid = fork();
    if (pid == 0) {
        // --armor parameter is used to decrypt an encrypted file that uses ASCII encoding
        char* args[] = {"gpg","--pinentry-mode", "loopback", "--batch", "--always-trust", "--yes", "--quiet", "--no-verbose", "--passphrase", clientIdS, "--output", outputFilePath, "--armor", "--decrypt", filePath, NULL};
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

void execReader(int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE], bufferSizeS[MAX_STRING_INT_SIZE];

    // prepare string arguments
    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);
    sprintf(bufferSizeS, "%d", bufferSize);

    char* args[] = {"exe/reader", clientIdFromS, clientIdToS, commonDirName, mirrorDirName, bufferSizeS, logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec reader failed\n");
    }

    return;
}

void execWriter(int clientIdFrom, int clientIdTo, char* commonDirName, int bufferSize, char* logFileName, char* tempFileListFileName,
                unsigned long tempFileListSize) {
    char clientIdFromS[MAX_STRING_INT_SIZE], clientIdToS[MAX_STRING_INT_SIZE], bufferSizeS[MAX_STRING_INT_SIZE], tempFileListSizeS[MAX_STRING_INT_SIZE];

    // prepare string arguments
    sprintf(clientIdFromS, "%d", clientIdFrom);
    sprintf(clientIdToS, "%d", clientIdTo);
    sprintf(bufferSizeS, "%d", bufferSize);
    sprintf(tempFileListSizeS, "%lu", tempFileListSize);

    char* args[] = {"exe/writer", tempFileListFileName, tempFileListSizeS, clientIdFromS, clientIdToS, commonDirName, bufferSizeS, logFileName, NULL};
    if (execvp(args[0], args) < 0) {
        printf("Exec writer failed\n");
    }

    return;
}