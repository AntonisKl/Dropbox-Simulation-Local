#include "mirror_client.h"

void handleArgs(int argc, char** argv, int* clientId, char** commonDirName, char** inputDirName, char** mirrorDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 13) {
        printErrorLn("Invalid arguments. Exiting...");
        handleExit(1, 0, 0, 0, 0);
    }

    // validate input arguments one by one
    if (strcmp(argv[1], "-n") == 0) {
        (*clientId) = atoi(argv[2]);
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
    createDir(mirrorDirName);  // create mirror directory

    if (!dirExists(commonDirName)) {
        createDir(commonDirName);  // create common directory
    }

    buildIdFileName(idFilePath, commonDirName, clientId);

    if (fileExists((char*)idFilePath)) {
        printErrorLn("Id common file already exists");
        handleExit(1, 0, 1, 0, 0);
    }

    if (!dirExists("tmp")) {
        createDir("tmp");  // create temp files' directory
    }

    return;
}

void populateFileList(FileList* fileList, char* inputDirName, int indent) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(inputDirName)) == NULL) {
        perror("Could not open input directory");
        handleExit(1, 1, 1, 1, 0);
    }

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, PATH_MAX, "%s/%s", inputDirName, entry->d_name);  // append to inputDirName the current file's name
        struct stat curStat;
        stat(path, &curStat);

        if (!S_ISREG(curStat.st_mode)) {                       // is a directory
            addFileToFileList(fileList, path, -1, DIRECTORY);  // add a DIRECTORY node to FileList

            populateFileList(fileList, path, indent + 2);                      // continue traversing directory
        } else {                                                               // is a file
            addFileToFileList(fileList, path, curStat.st_size, REGULAR_FILE);  // add a REGULAR_FILE node to FileList
        }
    }

    closedir(dir);  // close current directory
}

void handleExit(int exitValue, char removeIdFile, char removeMirrorDir, char writeToLogFile, char removeTempFile) {
    freeFileList(&inputFileList);

    if (removeIdFile) {
        char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
        buildIdFileName(&idFilePath, commonDirName, clientIdFrom);  // create client's id file path with format: [commonDirName]/[clientIdTo].id
        removeFileOrDir(idFilePath);                                // delete client's id file
    }

    if (removeMirrorDir) {
        removeFileOrDir(mirrorDirName);  // delete mirror directory
    }

    if (writeToLogFile) {
        // write to log file
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
            removeFileOrDir(tempFileListFileName);  // delete temporary input FileList file
            free(tempFileListFileName);
            tempFileListFileName = NULL;
        }
    }

    if (fileListS != NULL) {
        free(fileListS);
        fileListS = NULL;
    }

    exit(exitValue);
}

void createReader() {
    readerPid = fork();  // new reader process
    if (readerPid == -1) {
        perror("fork error");
        raise(SIGINT);
    } else if (readerPid == 0) {
        execReader(clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
    }

    return;
}

void createWriter() {
    writerPid = fork();  // new writer process
    printf("writer pid: %d\n", writerPid);
    if (writerPid == -1) {
        perror("fork error");
        raise(SIGINT);
    } else if (writerPid == 0) {
        execWriter(clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize);
    }

    return;
}

void createReaderAndWriter() {
    if (getpid() != clientPid) {
        // we are in a subprocess of the main/parent client process so change signal handler and signals

        sigAction.sa_handler = &handleSignals;

        sigaddset(&sigAction.sa_mask, SIGINT);
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

    readerPid = fork();  // new reader process
    if (readerPid == -1) {
        perror("fork error");
        raise(SIGINT);
    } else if (readerPid == 0) {
        execReader(clientIdFrom, clientIdTo, commonDirName, mirrorDirName, bufferSize, logFileName);
    } else {
        writerPid = fork();  // new writer process
        if (writerPid == -1) {
            perror("fork error");
            raise(SIGINT);
        } else if (writerPid == 0) {
            execWriter(clientIdFrom, clientIdTo, commonDirName, bufferSize, logFileName, tempFileListFileName, tempFileListSize);
        } else {
            while (1) {  // subprocess of client will wait until both children (reader and writer) have exited successfully
                int readerStatus, writerStatus;
                printf("Child with pid %d of client with id %d is waiting for reader and writer to exit\n", getpid(), clientIdFrom);
                while (readerPid == -1) {  // while there is not a child reader process running (rare case)
                    sleep(1);
                }
                waitpid(readerPid, &readerStatus, 0);  // wait for reader to exit
                while (writerPid == -1) {              // while there is not a child writer process running (rare case)
                    sleep(1);
                }
                waitpid(writerPid, &writerStatus, 0);  // wait for writer to exit

                // check statuses
                int readerExitStatus = WEXITSTATUS(readerStatus), writerExitStatus = WEXITSTATUS(writerStatus);
                if (readerExitStatus == 0 && writerExitStatus == 0) {
                    // sync with the current client completed successfully
                    printf(ANSI_COLOR_GREEN "Transfer from client with id %d to client with id %d completed successfully and both children (reader and writer) with pids %d and %d have exited succesfully\n" ANSI_COLOR_RESET, clientIdFrom, clientIdTo, readerPid, writerPid);
                    handleExit(0, 0, 0, 0, 0);  // exit normally
                } else {
                    if (readerExitStatus == 1) {
                        printf(ANSI_COLOR_RED "Reader with pid %d failed and exited\n" ANSI_COLOR_RESET, readerPid);
                        readerPid = -1;  // reset readerPid
                    }
                    if (writerExitStatus == 1) {
                        printf(ANSI_COLOR_RED "Writer with pid %d failed and exited\n" ANSI_COLOR_RESET, writerPid);
                        writerPid = -1;  // reset writerPid
                    }
                }
            }
        }
    }
}

void handleSigIntParentClient(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Client caught wrong signal instead of SIGINT\n");
    }
    printf("Client process with id %d caught SIGINT\n", getpid());

    handleExit(1, 1, 1, 1, 1);
}

void handleSigUsr1(int signal) {
    if (signal != SIGUSR1) {
        printErrorLn("Client caught wrong signal instead of SIGUSR1\n");
    }
    printf(ANSI_COLOR_BLUE "Client subprocess with id %d caught SIGUSR1" ANSI_COLOR_RESET "\n", getpid());

    if (attemptsNum < 3) {
        createReader();  // create new reader child process since the previous failed

        attemptsNum++;
    } else {
        printf(ANSI_COLOR_RED "Client subprocess of client with id %d exceeded the maximum restart attemts number, so it will exit" ANSI_COLOR_RESET "\n", getpid());
        // printf("sigusr1 writer pid: %d\n", writerPid);

        if (writerPid > 0 && !kill(writerPid, 0)) {
            kill(writerPid, SIGINT);  // kill writer child process
        }

        raise(SIGINT);
    }
}

void handleSigUsr2(int signal) {
    if (signal != SIGUSR2) {
        printErrorLn("Client subprocess caught wrong signal instead of SIGUSR2\n");
    }
    printf(ANSI_COLOR_BLUE "Client subprocess of client with id %d caught SIGUSR2" ANSI_COLOR_RESET "\n", getpid());

    if (attemptsNum < 3) {
        createWriter();  // create new writer child process since the previous failed

        attemptsNum++;
    } else {
        printf(ANSI_COLOR_RED "Client subprocess of client with id %d exceeded the maximum restart attemts number, so it will exit" ANSI_COLOR_RESET "\n", getpid());

        if (readerPid > 0 && !kill(readerPid, 0)) {
            kill(readerPid, SIGINT);  // kill reader child process
        }
        raise(SIGINT);
    }
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Client subprocess caught wrong signal instead of SIGINT\n");
    }
    printf("Client subprocess of client with id %d caught SIGINT\n", getpid());

    kill(getppid(), SIGINT);  // send SIGINT signal to main/parent client process

    handleExit(1, 1, 1, 1, 1);
}

void handleSignals(int signal) {
    // find out which signal we're handling
    switch (signal) {
        case SIGUSR1:  // reader failed
            handleSigUsr1(signal);
            break;
        case SIGUSR2:  // writer failed
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

void initialSync(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(commonDirName)) == NULL) {
        perror("Could not open input directory");
        handleExit(1, 1, 1, 1, 1);
    }

    // traverse the whole 1st level of the common directory
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !isIdFile(entry->d_name) || isSameIdFile(entry->d_name, clientId))
            continue;

        snprintf(path, PATH_MAX, "%s/%s", commonDirName, entry->d_name);  // append to commonDirName the current file's name
        struct stat curStat;
        stat(path, &curStat);  // stat current file

        if (S_ISREG(curStat.st_mode)) {  // is a regular file
            clientIdFrom = clientId;
            clientIdTo = atoi(strtok(entry->d_name, "."));  // save new client's id

            int pid = fork();  // create a client subprocess to handle the synchronization with this specific client with id clientIdTo
            if (pid == 0) {
                createReaderAndWriter();
            } else if (pid == -1) {
                perror("fork error");
                handleExit(1, 1, 1, 1, 1);
            }
        }
    }

    closedir(dir);  // close common directory
}

void startWatchingCommonDirectory(char* commonDirName, char* mirrorDirName, int bufferSize, FileList* inputFileList, int clientId, char* logFileName) {
    struct inotify_event iNotifyEvent;
    int readRet;
    uint32_t mask, nameLen;
    char curName[NAME_MAX], curPathName[PATH_MAX];

    int iNotifyFd = inotify_init();
    inotify_add_watch(iNotifyFd, commonDirName, IN_CREATE | IN_DELETE);  // add a watch to common directory with only IN_CREATE and IN_DELETE events

    // start listening for events
    while ((readRet = read(iNotifyFd, &iNotifyEvent, sizeof(struct inotify_event) + NAME_MAX + 1)) != -1) {
        mask = iNotifyEvent.mask;
        nameLen = iNotifyEvent.len;

        if (nameLen > 0) {
            strcpy(curName, iNotifyEvent.name);

            if (!isIdFile(curName) || isSameIdFile(curName, clientId))  // if file is irrelevant, ignore it
                continue;

            sprintf(curPathName, "%s%s", commonDirName, curName);  // create the path name of current file
        }

        switch (mask) {
            case IN_CREATE: {
                printf("Create event\n");

                clientIdTo = atoi(strtok(curName, "."));

                int pid = fork();  // create a client subprocess to handle the synchronization with this specific client with id clientIdTo
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
                printf("Delete event\n");

                char* clientIdDeletedS = strtok(curName, ".");

                char mirrorIdDirPath[strlen(mirrorDirName) + strlen(clientIdDeletedS) + 1];
                sprintf(mirrorIdDirPath, "%s/%s", mirrorDirName, clientIdDeletedS);

                removeFileOrDir(mirrorIdDirPath);  // delete leaving client's directory from local mirror directory

                break;
            }
        }
    }

    if (readRet == -1) {
        // error handling
        perror("read inotify desc failed");
        handleExit(1, 1, 1, 1, 1);
    }
}

int main(int argc, char** argv) {
    // setup the sighub handler
    sigAction.sa_handler = &handleSigIntParentClient;

    // restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // add only SIGINT signal (SIGUSR1 and SIGUSR2 signals are handled by the client's forked subprocesses)
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    int clientId;        // clientId: this client's id
    char* inputDirName;  // inputDirName: path of the input directory

    handleArgs(argc, argv, &clientId, &commonDirName, &inputDirName, &mirrorDirName, &bufferSize, &logFileName);

    clientIdFrom = clientId;  // initialize global variable clientIdFrom
    clientPid = getpid();

    // do initial checks and store client's id file path to idFilePath
    char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
    doClientInitialChecks(inputDirName, mirrorDirName, commonDirName, clientId, &idFilePath);

    // open log file
    FILE* file = fopen(logFileName, "a");
    if (file == NULL) {
        // error handling
        perror("fopen failed");
        handleExit(1, 1, 1, 1, 0);
    }

    // write to log
    fprintf(file, "Client with pid %d and id %d logged in succesfully\n", getpid(), clientId);
    fflush(file);

    // close log file
    if (fclose(file) == EOF) {
        // error handling
        perror("fclose failed");
        handleExit(1, 1, 1, 1, 0);
    }

    char pidS[MAX_STRING_INT_SIZE];
    sprintf(pidS, "%d", clientPid);

    if (GPG_ENCRYPTION_ON) {
        // GPG INITIALIZATION START

        char keyDetailsFileName[MAX_STRING_INT_SIZE + 14];
        createGpgKeyDetailsFile(clientId, &keyDetailsFileName);  // create temporary gpg key details file
        generateGpgKey(keyDetailsFileName);                      // generate gpg key-pair
        exportGpgPublicKey(idFilePath, clientId);                // export gpg public key to client's .id file
        removeFileOrDir(keyDetailsFileName);                     // delete temporary gpg key details file

        // GPG INITIALIZATION END

        // open client's .id file
        file = fopen(idFilePath, "a");
        if (file == NULL) {
            perror("fopen failed");
            handleExit(1, 1, 1, 1, 0);
        }

        fprintf(file, "\n%s", pidS);  // append pid to client's .id file
        fflush(file);

        // close client's .id file
        if (fclose(file) == EOF) {
            perror("fclose failed");
            handleExit(1, 1, 1, 1, 0);
        }
    } else {
        // create and write client's pid to client's .id file
        createAndWriteToFile(idFilePath, pidS);
    }

    // create input directory's file list
    inputFileList = initFileList();
    populateFileList(inputFileList, inputDirName, 0);

    tempFileListFileName = (char*)malloc(MAX_TEMP_FILELIST_FILE_NAME_SIZE);
    sprintf(tempFileListFileName, "tmp/TempFileList%d", clientId);  // create temp FileList file's path

    fileListS = (char*)malloc(inputFileList->size * (MAX_FILE_LIST_NODE_STRING_SIZE));  // fileListS: string in which the converted inputFileList will be stored
    fileListToString(inputFileList, &fileListS);                                        // convert inputFileList to string and store it in fileListS

    tempFileListSize = strlen(fileListS);                   // initialize tempFileListSize variable to pass it to writers afterwards
    createAndWriteToFile(tempFileListFileName, fileListS);  // create and write to temp file list file the string-converted FileList

    // fileListS no longer needed
    if (fileListS != NULL) {
        free(fileListS);
        fileListS = NULL;
    }

    initialSync(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);                   // do initial sync
    startWatchingCommonDirectory(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId, logFileName);  // start listening for inotify events on common directory

    handleExit(0, 1, 1, 1, 1);  // handle normal exit
}
