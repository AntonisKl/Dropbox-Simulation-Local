#include "writer.h"

void handleArgs(int argc, char** argv, FileList** inputFileList, int* clientIdFrom, int* clientIdTo, char** commonDirName, int* bufferSize, char** logFileName) {
    // validate argument count
    if (argc != 8) {
        printErrorLnExit("Invalid arguments. Exiting...");
    }

    // assume that all arguments are valid

    FILE* fp = fopen(argv[1], "r");  // open temp file that contains the input FileList in string format
    if (fp == NULL) {
        // error handling
        perror("fopen failed");
        handleExit(1, SIGUSR2);
    }

    char* endptr;
    unsigned long fileBufferSize = strtol(argv[2], &endptr, 10) + 1;  // get size of temp file
    tempFileContents = (char*)malloc(fileBufferSize);
    fgets(tempFileContents, fileBufferSize, fp);  // get all of the temp file's contents which are contained in one line

    if (fclose(fp) == EOF) {  // close temp file that contains the input FileList in string format
        // error handling
        perror("fclose failed");
        handleExit(1, SIGUSR2);
    }

    if (!strcmp(tempFileContents, EMPTY_FILE_LIST_STRING)) {  // the empty file list string symbol was read
        (*inputFileList) = initFileList();
    } else {
        (*inputFileList) = stringToFileList(tempFileContents);  // convert string to FileList struct
    }

    // temp file contents are not needed anymore so free them
    free(tempFileContents);
    tempFileContents = NULL;

    (*clientIdFrom) = atoi(argv[3]);
    (*clientIdTo) = atoi(argv[4]);
    (*commonDirName) = argv[5];
    (*bufferSize) = atoi(argv[6]);
    (*logFileName) = argv[7];

    return;
}

void handleExit(int exitValue, int parentSignal) {
    freeFileList(&inputFileList);  // free the input FileList

    // try to close log file
    if (logFileP != NULL && fclose(logFileP) == EOF) {
        perror("fclose failed");
        if (parentSignal != 0) {
            // send signal to parent process which is a subprocess of client
            kill(getppid(), parentSignal);
        }
        exit(1);
    }

    // try to free tempFileContents
    if (tempFileContents != NULL) {
        free(tempFileContents);
        tempFileContents = NULL;
    }

    // try to close fifoFd
    if (fifoFd >= 0 && close(fifoFd) == -1) {
        perror("close failed");
        if (parentSignal != 0) {
            // send signal to parent process which is a subprocess of client
            kill(getppid(), parentSignal);
        }
        exit(1);
    }
    fifoFd = -1;

    if (parentSignal != 0) {
        // send signal to parent process which is a subprocess of client
        kill(getppid(), parentSignal);
    }

    // exit with exitValue as status
    exit(exitValue);
}

void handleSigInt(int signal) {
    if (signal != SIGINT) {
        printErrorLn("Caught wrong signal instead of SIGINT\n");
    }
    printf("Writer process with id %d caught SIGINT\n", getpid());

    handleExit(1, 0);
}

void handleSigAlarm(int signal) {
    if (signal != SIGALRM) {
        printErrorLn("Caught wrong signal instead of SIGALRM\n");
    }
    printf("Writer process with id %d caught SIGALRM\n", getpid());

    handleExit(1, SIGUSR2);
}

void handleSigPipe(int signal) {
    if (signal != SIGPIPE) {
        printErrorLn("Caught wrong signal instead of SIGPIPE\n");
    }
    printf("Writer process with id %d caught SIGPIPE\n", getpid());

    handleExit(1, SIGUSR2);
}

void handleSignals(int signal) {
    // Find out which signal we're handling
    switch (signal) {
        case SIGINT:
            handleSigInt(signal);
            break;
        case SIGALRM:
            handleSigAlarm(signal);
            break;
        case SIGPIPE:
            handleSigPipe(signal);
            break;
        default:
            printErrorLn("Caught wrong signal");
    }
    return;
}

void tryWrite(int fd, const void* buffer, int bufferSize) {
    int returnValue, tempBufferSize = bufferSize, progress = 0;

    returnValue = write(fd, buffer, tempBufferSize);
    while (returnValue < tempBufferSize && returnValue != 0) {  // rare case
        // not all desired bytes were written, so write the remaining bytes of buffer
        if (returnValue == -1) {
            perror("write error");
            handleExit(1, SIGUSR2);
        }

        tempBufferSize -= returnValue;
        progress += returnValue;
        returnValue = write(fd, buffer + progress, tempBufferSize); // write remaining bytes that aren't written yet
    }

    return;
}

int main(int argc, char** argv) {
    // START OF SIGNAL HANLDING

    struct sigaction sigAction;

    // setup the signal handler
    sigAction.sa_handler = &handleSignals;

    // restart the system call, if at all possible
    sigAction.sa_flags = SA_RESTART;

    // handle only SIGINT and SIGALRM
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGALRM);

    if (sigaction(SIGINT, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGINT");  // Should not happen
    }

    if (sigaction(SIGALRM, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGALRM");  // Should not happen
    }

    if (sigaction(SIGPIPE, &sigAction, NULL) == -1) {
        perror("Error: cannot handle SIGPIPE");  // Should not happen
    }

    // END OF SIGNAL HANLDING

    // inputFileList: the variable which will contain the FileList that will be read from the temp file list file
    // see file "utils.h" above the execWriter() function for information about each of the following variables
    FileList* inputFileList;
    int clientIdFrom, clientIdTo, bufferSize;
    char *commonDirName, *logFileName;

    handleArgs(argc, argv, &inputFileList, &clientIdFrom, &clientIdTo, &commonDirName, &bufferSize, &logFileName);

    char fifoFileName[MAX_FIFO_FILE_NAME];                       // fifoFileName: name of fifo file
    buildFifoFileName(&fifoFileName, clientIdFrom, clientIdTo);  // format: [clientIdFrom]_to_[clientIdTo].fifo

    char fifoFilePath[strlen(fifoFileName) + strlen(commonDirName) + 2];  // fifoFilePath: path of fifo file
    sprintf(fifoFilePath, "%s/%s", commonDirName, fifoFileName);          // format: [commonDir]/[clientIdFrom]_to_[clientIdTo].fifo

    if (!fileExists(fifoFilePath)) {
        mkfifo(fifoFilePath, 0666);  // create fifo pipe
    }

    // block at open for 30 seconds and if open didn't return, raise SIGALRM
    alarm(30);
    fifoFd = open(fifoFilePath, O_WRONLY);  // fifoFd: declared globally in file "writer.h"
    alarm(0);

    if (fifoFd == -1) {
        // error handling
        perror("open failed");
        handleExit(1, SIGUSR2);
    }

    // open log file
    if ((logFileP = fopen(logFileName, "a")) == NULL) {  // logFileP: declared globally in file "reader.h"
        // error handling
        perror("fopen failed");
        handleExit(1, SIGUSR2);
    }

    // tmpEncryptedFilePath: path of the temporary encrypted file in each while loop
    // buffer: buffer in which a chunk of the file will be stored with each read() call
    char tmpEncryptedFilePath[MAX_TMP_ENCRYPTED_FILE_PATH_SIZE], buffer[bufferSize + 1];

    // start traversing the FileList and send write each file to fifo pipe
    File* curFile = inputFileList->firstFile;
    while (curFile != NULL) {
        char curFilePathCopy[strlen(curFile->path) + 1];
        strcpy(curFilePathCopy, curFile->path);  // temporarily store current file's path in order to manipulate it and get cut the input directory's name from it
        // pathNoInputDirName: current file's path without the input directory's name
        char* pathNoInputDirName = strtok(curFilePathCopy, "/");  // cut the input directory's name
        pathNoInputDirName = strtok(NULL, "\n");                  // until end of path

        short int filePathSize = strlen(pathNoInputDirName);

        // write file path's size to fifo pipe and write to log
        tryWrite(fifoFd, &filePathSize, 2);
        fprintf(logFileP, "Writer with pid %d wrote 2 bytes of metadata to fifo pipe\n", getpid());
        fflush(logFileP);

        // write file's path to fifo pipe and write to log
        tryWrite(fifoFd, pathNoInputDirName, filePathSize);
        fprintf(logFileP, "Writer with pid %d wrote %d bytes of metadata to fifo pipe\n", getpid(), filePathSize);
        fflush(logFileP);

        if (curFile->type == REGULAR_FILE) {  // is a regular file (not a directory)
            struct stat encryptedFileStat;    // i-node information of the encrypted file (if gpg encryption mode is ON)
            if (GPG_ENCRYPTION_ON) {
                char recipientIdFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];  // path of the .id file of the client that will receive the current file

                buildIdFileName(&recipientIdFilePath, commonDirName, clientIdTo);  // format: [commonDirName]/[clientIdTo].id
                importGpgPublicKey(recipientIdFilePath);                           // import gpg public key from recipient's .id file
                sprintf(tmpEncryptedFilePath, "%s%d", "tmp/enc", getpid());        // create temporary encrypted file's path

                encryptFile(curFile->path, clientIdTo, tmpEncryptedFilePath);  // encrypt file and write the output to the temporary encrypted file

                stat(tmpEncryptedFilePath, &encryptedFileStat);  // stat the new encrypted file

                // write encrypted file contents' size to fifo pipe and write to log
                tryWrite(fifoFd, &encryptedFileStat.st_size, 4);
                fprintf(logFileP, "Writer with pid %d wrote 4 bytes of metadata to fifo pipe\n", getpid());
                fflush(logFileP);

                // open the new encrypted file
                curFd = open(tmpEncryptedFilePath, O_RDONLY | O_NONBLOCK);
                if (curFd < 0) {
                    perror("open failed");
                    handleExit(1, SIGUSR2);
                }
            } else {  // no gpg encryption
                // write original file contents' size to fifo pipe and write to log
                tryWrite(fifoFd, &curFile->contentsSize, 4);
                fprintf(logFileP, "Writer with pid %d wrote 4 bytes of metadata to fifo pipe\n", getpid());
                fflush(logFileP);

                // open the original file
                curFd = open(curFile->path, O_RDONLY | O_NONBLOCK);
                if (curFd < 0) {
                    perror("open failed");
                    handleExit(1, SIGUSR2);
                }
            }

            // bytesWritten: the bytes of file contents that are written to fifo pipe (for logging purposes)
            // remainingContentsSize: the number of bytes that are remaining to be read from the file
            // tempBufferSize: temporary buffer size to read from file and write to fifo pipe in each while loop
            int bytesWritten = 0, remainingContentsSize, tempBufferSize = bufferSize;

            // use the encrypted file's size or the original file's size according to the gpg encryption mode
            remainingContentsSize = (GPG_ENCRYPTION_ON ? encryptedFileStat.st_size : curFile->contentsSize);

            // if the remaining contents of file have size less than the buffer size, read only the remaining contents from file to then write them to fifo pipe
            tempBufferSize = (remainingContentsSize < bufferSize ? remainingContentsSize : bufferSize);

            memset(buffer, 0, bufferSize + 1);                       // clear buffer
            int readRetValue = read(curFd, buffer, tempBufferSize);  // read a chunk of the current file
            while (readRetValue > 0) {
                if (readRetValue == -1) {
                    perror("read failed");
                    handleExit(1, SIGUSR2);
                }

                tryWrite(fifoFd, buffer, tempBufferSize);  // write a chunk of the current file to fifo pipe

                bytesWritten += tempBufferSize;           // for logging purposes
                remainingContentsSize -= tempBufferSize;  // proceed tempBufferSize bytes

                // if the remaining contents of file have size less than the buffer size, read only the remaining contents from file to then write them to fifo pipe
                tempBufferSize = (remainingContentsSize < bufferSize ? remainingContentsSize : bufferSize);

                memset(buffer, 0, bufferSize + 1);                   // clear buffer
                readRetValue = read(curFd, buffer, tempBufferSize);  // read a chunk of the current file
            }

            // write to log
            fprintf(logFileP, "Writer with pid %d sent file with path \"%s\" and wrote %d bytes to fifo pipe\n", getpid(), pathNoInputDirName, bytesWritten);
            fflush(logFileP);

            // try to close current file
            if (close(curFd) == -1) {
                perror("close failed");
                handleExit(1, SIGUSR2);
            }
        } else {  // is a directory
            // write directory's contents size which is always -1 to fifo pipe and write to log
            tryWrite(fifoFd, &curFile->contentsSize, 4);
            fprintf(logFileP, "Writer with pid %d wrote 4 bytes of metadata to fifo pipe\n", getpid());
            fflush(logFileP);

            // write to log
            fprintf(logFileP, "Writer with pid %d sent file with path \"%s\" and wrote %d bytes to fifo pipe\n", getpid(), pathNoInputDirName, 0);
            fflush(logFileP);
        }
        curFile = curFile->nextFile;
    }

    if (GPG_ENCRYPTION_ON) {
        // delete the temporary file that was used for gpg encryption
        removeFileOrDir(tmpEncryptedFilePath);
    }

    // write the ending indicator to fifo pipe and write to log
    int end = 0;
    tryWrite(fifoFd, &end, 2);
    fprintf(logFileP, "Writer with pid %d wrote 2 bytes of metadata to fifo pipe\n", getpid());
    fflush(logFileP);

    handleExit(0, 0);  // handle normal exit
}