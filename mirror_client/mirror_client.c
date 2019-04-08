#include "mirror_client.h"

static int attemptsNum = 0, updateAttemptsNum = 0, clientChildShouldExit = 0;

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 13) {
        printErrorLn("Invalid arguments. Exiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    // validate input arguments one by one
    if (strcmp(argv[1], "-n") == 0) {
        (*clientId) = atoi(argv[2]);
        // if ((*clientId) <= 0) {
        //     printError("Invalid arguments\nExiting...\n");
        //     raiseIntAndExit(1);
        // }
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    if (strcmp(argv[3], "-c") == 0) {
        (*commonDirName) = argv[4];
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    if (strcmp(argv[5], "-i") == 0) {
        (*inputDirName) = argv[6];
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    if (strcmp(argv[7], "-m") == 0) {
        (*mirrorDirName) = argv[8];
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    if (strcmp(argv[9], "-b") == 0) {
        (*bufferSize) = atoi(argv[10]);
        if ((*bufferSize) <= 0) {
            printErrorLn("Invalid arguments\nExiting...");
            handleExit(1, 0, 0, 0, 0);
        }
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    if (strcmp(argv[11], "-l") == 0) {
        (*logFileName) = argv[12];
    } else {
        printErrorLn("Invalid arguments\nExiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    return;
}

void doClientInitialChecks(char* inputDirName, char* mirrorDirName, char* commonDirName, int clientId, char (*idFilePath)[]) {
    if (!dirExists(inputDirName)) {
        printErrorLn("Input directory does not exist");
        handleExit(1, 0, 0, 0, 0);
    }

    if (dirExists(mirrorDirName)) {
        printErrorLn("Mirror directory already exists");
        handleExit(1, 0, 0, 0, 0);
    }
    createDir(mirrorDirName);
    printf("dir name: %s\n", mirrorDirName);

    if (!dirExists(commonDirName)) {
        createDir(commonDirName);
    }

    // char idFilePath[strlen(commonDirName) + 1 + strlen(clientId) + 4];
    buildIdFileName(idFilePath, commonDirName, clientId);

    if (fileExists((char*)idFilePath)) {
        printErrorLn("Id common file already exists");
        handleExit(1, 0, 1, 0, 0);
    }

    return;
}

void populateFileList(FileList* fileList, char* inputDirName, char* pathWithoutInputDirName, int indent) {
    DIR* dir;
    struct dirent* entry;
    // printf("start of populate\n");

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
            // printf("dir: %s\n", entry->d_name);
            addFileToFileList(fileList, pathNoInputDirName, path, -1, DIRECTORY);

            // printf("Handled directory: %*s[%s]\n", indent, "", entry->d_name);
            populateFileList(fileList, path, pathNoInputDirName, indent + 2);
        } else {  // if it is a file

            // if (parentDir == NULL) {
            //     parentDir = dirTree->rootNode;
            // }

            // addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, File, NULL);

            addFileToFileList(fileList, pathNoInputDirName, path, curStat.st_size, REGULAR_FILE);  // curStat.st_size - 1: because when file is empty, its size is 1 byte by default

            // printf("Handled file: %*s- %s\n", indent, "", entry->d_name);
        }
    }
    // printf("end of populate\n");

    closedir(dir);
}

void handleExit(int exitValue, char removeIdFile, char removeMirrorDir, char writeToLogFile, char removeTempFile) {
    freeFileList(&inputFileList);

    if (removeIdFile) {
        char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
        buildIdFileName(&idFilePath, commonDirName, clientIdFrom);
        removeFileOrDir(idFilePath);
    }

    // char clientIdFromS[MAX_STRING_INT_SIZE];
    // sprintf(clientIdFromS, "%d", clientIdFrom);
    // char mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdFromS) + 2];
    // strcpy(mirrorIdDirPath, mirrorDirName);
    // strcat(mirrorIdDirPath, "/");
    // strcat(mirrorIdDirPath, clientIdFromS);
    // removeFileOrDir(mirrorIdDirPath);

    if (removeMirrorDir) {
        // printf("HAAY %s\n", mirrorDirName);
        removeFileOrDir(mirrorDirName);
    }

    if (writeToLogFile) {
        FILE* file = fopen(logFileName, "a");
        if (file == NULL) {
            perror("fopen failed");
            exit(1);
        }

        fprintf(file, "Client with pid %d and id %d exited succesfully\n", getpid(), clientIdFrom);
        fflush(file);

        if (fclose(file) == EOF) {
            perror("fclose failed");
            exit(1);
        }
    }

    if (removeTempFile) {
        if (tempFileListFileName != NULL) {
            removeFileOrDir(tempFileListFileName);
            free(tempFileListFileName);
            tempFileListFileName = NULL;
        }
    }

    printf("handled exit\n");

    exit(exitValue);
}

void handleSigIntParentClient(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Client caught wrong signal instead of SIGINT\n");
    }
    printf("Client process with id %d caught SIGINT\n", getpid());

    handleExit(1, 1, 1, 1, 1);
}

void createReader();

void handleSigUsr1(int signal) {
    if (signal != SIGUSR1) {
        printErrorLn("Client caught wrong signal instead of SIGUSR1\n");
    }
    printf(ANSI_COLOR_BLUE "Client process with id %d caught SIGUSR1" ANSI_COLOR_RESET "\n", getpid());

    // if (readerPid != -1 && !kill(readerPid, 0)) {
    //     kill(readerPid, SIGINT);
    // }
    // if (writerPid != -1 && !kill(writerPid, 0)) {
    //     kill(writerPid, SIGINT);
    // }
    if (attemptsNum < 3) {
        // char otherClientIdFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
        // buildIdFileName(&otherClientIdFilePath, commonDirName, clientIdTo);

        // FILE* fp = fopen(otherClientIdFilePath, "r");
        // if (fp == NULL) {
        //     perror("fopen failed");
        //     exit(1);
        // }

        // char pidToS[MAX_STRING_INT_SIZE];
        // fgets((char*)idS, MAX_STRING_INT_SIZE, fp);

        // fflush(file);

        // if (fclose(file) == EOF) {
        //     perror("fclose failed");
        //     exit(1);
        // }

        createReader();

        attemptsNum++;
    } else {
        printf(ANSI_COLOR_RED "Client with id %d exceeded the maximum restart attemts number, so it will exit" ANSI_COLOR_RESET "\n", getpid());
        raiseIntAndExit(1);
    }
}

void handleSigUsr2(int signal) {
    if (signal != SIGUSR2) {
        printErrorLn("Client caught wrong signal instead of SIGUSR2\n");
    }
    printf(ANSI_COLOR_BLUE "Client process with id %d caught SIGUSR2" ANSI_COLOR_RESET "\n", getpid());

    // if (readerPid != -1 && !kill(readerPid, 0)) {
    //     kill(readerPid, SIGINT);
    // }
    // if (writerPid != -1 && !kill(writerPid, 0)) {
    //     kill(writerPid, SIGINT);
    // }
    if (attemptsNum < 3) {
        createWriter();

        attemptsNum++;
    } else {
        printf(ANSI_COLOR_RED "Client with id %d exceeded the maximum restart attemts number, so it will exit" ANSI_COLOR_RESET "\n", getpid());
        raiseIntAndExit(1);
    }
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Child of client caught wrong signal instead of SIGINT\n");
    }
    printf("Child of client process with id %d caught SIGINT\n", getpid());

    handleExit(1, 1, 1, 1, 1);
}

void handleSignals(int signal) {
    // Find out which signal we're handling
    switch (signal) {
        case SIGUSR1:  // reader
            handleSigUsr1(signal);
            break;
        case SIGUSR2:  // writer
            handleSigUsr2(signal);
            break;
        case SIGINT:
            handleSigInt(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

void createReader() {
    readerPid = fork();
    if (readerPid == -1) {
        perror("fork error");
        raiseIntAndExit(1);
    } else if (readerPid == 0) {
        execReader(clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize, clientPid);
        // exit(0);  // TODO: before exit raise signals to parent
    }

    return;
}

void createWriter() {
    writerPid = fork();
    if (writerPid == -1) {
        perror("fork error");
        raiseIntAndExit(1);
    } else if (writerPid == 0) {
        execWriter(clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize, clientPid);
        // exit(0);  // TODO: before exit raise signals to parent
    }

    return;
}

void createReaderAndWriter() {
    // struct sigaction sigAction;

    // // Setup the sighub handler
    // sigAction.sa_handler = &handleSigUsr1;

    // // Restart the system call, if at all possible
    // sigAction.sa_flags = SA_RESTART;

    // // Block every signal during the handler
    // sigemptyset(&sigAction.sa_mask);
    // sigaddset(&sigAction.sa_mask, SIGUSR1);

    // if (sigaction(SIGUSR1, &sigAction, NULL) == -1) {
    //     perror("Error: cannot handle SIGALRM");  // Should not happen
    // }

    // struct sigaction sigAction;

    // // Setup the sighub handler
    // sigAction.sa_handler = &handleSignals;

    // // Restart the system call, if at all possible
    // sigAction.sa_flags = SA_RESTART;

    // // Block every signal during the handler
    // sigemptyset(&sigAction.sa_mask);
    // sigaddset(&sigAction.sa_mask, SIGINT);
    // sigaddset(&sigAction.sa_mask, SIGUSR1);

    // if (sigaction(SIGINT, &sigAction, NULL) == -1) {
    //     perror("Error: cannot handle SIGINT");  // Should not happen
    // }

    // if (sigaction(SIGUSR1, &sigAction, NULL) == -1) {
    //     perror("Error: cannot handle SIGUSR1");  // Should not happen
    // }

    if (getpid() != clientPid) {
        sigAction.sa_handler = &handleSignals;

        sigaddset(&sigAction.sa_mask, SIGUSR1);
        sigaddset(&sigAction.sa_mask, SIGUSR2);

        if (sigaction(SIGINT, &sigAction, NULL) == -1) {
            perror("Error: cannot handle SIGINT");  // Should not happen
        }
        if (sigaction(SIGUSR1, &sigAction, NULL) == -1) {
            perror("Error: cannot handle SIGUSR1");  // Should not happen
        }
        if (sigaction(SIGUSR2, &sigAction, NULL) == -1) {
            perror("Error: cannot handle SIGUSR2");  // Should not happen
        }
    }

    readerPid = fork();
    if (readerPid == -1) {
        perror("fork error");
        raiseIntAndExit(1);
    } else if (readerPid == 0) {
        execReader(clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize, clientPid);
        // exit(0);  // TODO: before exit raise signals to parent
    } else {
        writerPid = fork();
        if (writerPid == -1) {
            perror("fork error");
            raiseIntAndExit(1);
        } else if (writerPid == 0) {
            execWriter(clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize, clientPid);
            // exit(0);  // TODO: before exit raise signals to parent
        } else {
            while (1) {
                int readerStatus, writerStatus;
                printf("Client with id %d waiting for children to exit\n", clientIdFrom);
                while (readerPid == -1) {  // rare
                    sleep(1);
                }
                waitpid(readerPid, &readerStatus, 0);
                while (writerPid == -1) {  // rare
                    sleep(1);
                }
                waitpid(writerPid, &writerStatus, 0);
                int readerExitStatus = WEXITSTATUS(readerStatus), writerExitStatus = WEXITSTATUS(writerStatus);
                if (readerExitStatus == 0 && writerExitStatus == 0) {
                    printf(ANSI_COLOR_GREEN "Transfer completed successfully and both children with pids %d and %d have exited\n" ANSI_COLOR_RESET, readerPid, writerPid);
                    exit(0);
                } else {
                    if (readerExitStatus == 1) {
                        printf(ANSI_COLOR_RED "Reader with pid %d failed and exited\n" ANSI_COLOR_RESET, readerPid);
                        readerPid = -1;
                    }
                    if (writerExitStatus == 1) {
                        printf(ANSI_COLOR_RED "Writer with pid %d failed and exited\n" ANSI_COLOR_RESET, writerPid);
                        writerPid = -1;
                    }
                }
                // readerPid = -1;
                // writerPid = -1;
            }
        }
    }
}

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(commonDirName)) == NULL) {
        perror("Could not open input directory");
        exit(1);
    }

    printf("in initial sync");

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        // printf("entry name: %s\n", entry->d_name);
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

            int pid = fork();
            if (pid == 0) {
                createReaderAndWriter();
                printf("child-client exitting\n");
                exit(0);
            } else if (pid == -1) {
                perror("fork error");
                handleExit(1, 1, 1, 1, 1);
            }
        }
    }
    printf("end of initial sync\n");
    closedir(dir);
}

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName) {
    struct inotify_event iNotifyEvent;
    int readRet, wd;
    uint32_t mask, nameLen;
    char curName[NAME_MAX], prevName[NAME_MAX];
    strcpy(prevName, "");

    int iNotifyFd = inotify_init();
    inotify_add_watch(iNotifyFd, commonDirName, IN_CREATE | IN_DELETE);

    // char eventFileNames[20][PATH_MAX];
    // int eventFileNamesIndex = 0;

    while ((readRet = read(iNotifyFd, &iNotifyEvent, sizeof(struct inotify_event) + NAME_MAX + 1)) != -1) {
        wd = iNotifyEvent.wd;
        mask = iNotifyEvent.mask;
        nameLen = iNotifyEvent.len;

        if (nameLen > 0) {
            // if (strcmp(prevName, "") && !strcmp(iNotifyEvent.name, prevName))
            //     continue;

            strcpy(curName, iNotifyEvent.name);

            // if (mask == IN_CLOSE_WRITE || mask == IN_CLOSE_WRITE || (mask == (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE))) {
            //     strcpy(prevName, curName);
            // }

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
                clientIdTo = atoi(strtok(curName, "."));
                int pid = fork();
                if (pid == 0) {
                    createReaderAndWriter();
                    exit(0);
                } else if (pid == -1) {
                    perror("fork error");
                    handleExit(1, 1, 1, 1, 1);
                }
                break;
            }
            case IN_DELETE: {
                printf("DELETE event\n");
                char* clientIdDeletedS = strtok(curName, ".");

                char mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdDeletedS) + 1];
                strcpy(mirrorIdDirPath, mirrorDirName);
                strcat(mirrorIdDirPath, "/");
                strcat(mirrorIdDirPath, clientIdDeletedS);

                removeFileOrDir(mirrorIdDirPath);

                break;
            }
                // case IN_CLOSE_WRITE: {
                //     printf("Close-write event\n");
                //     clientIdTo = atoi(strtok(curName, "."));
                //     createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);

                //     break;
                // }
                // case IN_CLOSE_NOWRITE: {
                //     printf("Close-nowrite event\n");
                //     clientIdTo = atoi(strtok(curName, "."));
                //     createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
                //     break;
                // }
                // case IN_CLOSE_WRITE | IN_CLOSE_NOWRITE: {
                //     printf("Close event\n");
                //     clientIdTo = atoi(strtok(curName, "."));
                //     createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
                //     break;
                // }
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

int main(int argc, char** argv) {
    printf("client piiiiiiiiiiiiiiiiiiiiiiiiid %d", getpid());
    int clientId;
    char* inputDirName;

    handleArgs(argc, argv, &clientId, &commonDirName, &inputDirName, &mirrorDirName, &bufferSize, &logFileName);
    clientIdFrom = clientId;
    clientPid = getpid();

    // Setup the sighub handler
    sigAction.sa_handler = &handleSigIntParentClient;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);
    // sigaddset(&sigAction.sa_mask, SIGUSR1);

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    // if (sigaction(SIGUSR1, &sigAction, NULL) == -1) {
    //     perror("Error: cannot handle SIGUSR1");  // Should not happen
    // }

    char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
    doClientInitialChecks(inputDirName, mirrorDirName, commonDirName, clientId, &idFilePath);

    FILE* file = fopen(logFileName, "a");
    if (file == NULL) {
        perror("fopen failed");
        exit(1);
    }

    fprintf(file, "Client with pid %d and id %d logged in succesfully\n", getpid(), clientId);
    fflush(file);

    if (fclose(file) == EOF) {
        perror("fclose failed");
        exit(1);
    }

    char pidS[MAX_STRING_INT_SIZE];
    sprintf(pidS, "%d", getpid());
    createAndWriteToFile(idFilePath, pidS);
    inputFileList = initFileList();
    populateFileList(inputFileList, inputDirName, "", 0);

    tempFileListFileName = (char*)malloc(MAX_TEMP_FILELIST_FILE_NAME_SIZE);
    strcpy(tempFileListFileName, "TempFileList");
    char clientIdS[MAX_STRING_INT_SIZE];
    sprintf(clientIdS, "%d", clientId);
    strcat(tempFileListFileName, clientIdS);

    printf("list size: %u\n", inputFileList->size * (MAX_FILE_LIST_NODE_STRING_SIZE));

    char fileListS[inputFileList->size * (MAX_FILE_LIST_NODE_STRING_SIZE)];
    printf("1\n");
    fileListToString(inputFileList, &fileListS);

    printf("before create file\n\n");

    tempFileListSize = strlen(fileListS);
    createAndWriteToFile(tempFileListFileName, fileListS);
    printf("after create file\n\n");

    printf("first filelist item: %s\n\n\n\n", inputFileList->firstFile->pathNoInputDir);
    initialSync(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);
    startWatchingCommonDirectory(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);

    handleExit(0, 1, 1, 1, 1);
}