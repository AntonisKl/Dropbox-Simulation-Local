#include "mirror_client.h"

static int attemptsNum = 0;

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

void handleExit() {
    freeFileList(&inputFileList);

    char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
    buildIdFileName(&idFilePath, commonDirName, clientIdFrom);
    removeFileOrDir(idFilePath);

    char clientIdFromS[MAX_STRING_INT_SIZE];
    sprintf(clientIdFromS, "%d", clientIdFrom);
    char mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdFromS) + 2];
    strcpy(mirrorIdDirPath, mirrorDirName);
    strcat(mirrorIdDirPath, "/");
    strcat(mirrorIdDirPath, clientIdFromS);
    removeFileOrDir(mirrorIdDirPath);

    FILE* file = fopen(logFileName, "a");
    if (file == NULL) {
        perror("fopen failed");
        exit(1);
    }

    fprintf(file, "Client with pid %d exited succesfully\n", getpid());

    fflush(file);
    if (fclose(file) == EOF) {
        perror("fclose failed");
        exit(1);
    }
}

void handleSigUsr1(int signal);

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Caught wrong signal instead of SIGINT\n");
    }
    printf("Client process with id %d caught SIGINT\n", getpid());

    handleExit();

    exit(0);
}

void handleSignals(int signal) {
    // Find out which signal we're handling
    switch (signal) {
        case SIGUSR1:
            handleSigUsr1(signal);
            break;
        case SIGINT:
            handleSigInt(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

void createReaderAndWriter(FileList* inputFileList, int clientIdFrom, int clientIdTo, char* commonDirName, char* mirrorDirName, int bufferSize, char* logFileName) {
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
        execReader(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
        // exit(0);  // TODO: before exit raise signals to parent
    } else {
        int writerPid = fork();
        if (writerPid == -1) {
            perror("fork error");
            raiseIntAndExit(1);
        } else if (writerPid == 0) {
            execWriter(inputFileList, clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName);
            // exit(0);  // TODO: before exit raise signals to parent
        } else {
            int readerStatus, writerStatus;
            printf("client %d waiting for children to exit\n", clientIdFrom);
            waitpid(readerPid, &readerStatus, 0);
            waitpid(writerPid, &writerStatus, 0);
            int readerExitStatus = WEXITSTATUS(readerStatus), writerExitStatus = WEXITSTATUS(writerStatus);
            if (readerExitStatus == 0 && writerExitStatus == 0) {
                printf(ANSI_COLOR_GREEN "Transfer completed successfully and both children with pids %d and %d have exited\n" ANSI_COLOR_RESET, readerPid, writerPid);
            } else {
                if (readerExitStatus == 1) {
                    printf(ANSI_COLOR_RED "Reader with pid %d failed and exited\n" ANSI_COLOR_RESET, readerPid);
                }
                if (writerExitStatus == 1) {
                    printf(ANSI_COLOR_RED "Writer with pid %d failed and exited\n" ANSI_COLOR_RESET, writerPid);
                }
            }
        }
    }
}

void handleSigUsr1(int signal) {
    if (signal != SIGUSR1) {
        printErrorLn("Caught wrong signal instead of SIGUSR1\n");
    }
    printf("Client process with id %d caught SIGUSR1\n", getpid());

    if (attemptsNum < 3) {
        createReaderAndWriter(inputFileList, clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
        attemptsNum++;
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
            createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
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
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);

                break;
            }
            case IN_CLOSE_NOWRITE: {
                printf("Close-nowrite event\n");
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
                break;
            }
            case IN_CLOSE_WRITE | IN_CLOSE_NOWRITE: {
                printf("Close event\n");
                clientIdTo = atoi(strtok(curName, "."));
                createReaderAndWriter(inputFileList, clientId, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
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

int main(int argc, char** argv) {
    int clientId;
    char* inputDirName;

    handleArgs(argc, argv, &clientId, &commonDirName, &inputDirName, &mirrorDirName, &bufferSize, &logFileName);
    clientIdFrom = clientId;

    struct sigaction sigAction;

    // Setup the sighub handler
    sigAction.sa_handler = &handleSignals;

    // Restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
    doClientInitialChecks(inputDirName, mirrorDirName, commonDirName, clientId, &idFilePath);

    FILE* file = fopen(logFileName, "a");
    if (file == NULL) {
        perror("fopen failed");
        exit(1);
    }

    fprintf(file, "Client with pid %d logged in succesfully\n", getpid());

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
    printf("first filelist item: %s\n\n\n\n", inputFileList->firstFile->pathNoInputDir);
    initialSync(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);
    startWatchingCommonDirectory(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);

    handleExit();

    return 0;
}