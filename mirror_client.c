#include "utils/utils.h"

int main(int argc, char** argv) {
    int clientId;
    char *inputDirName, *logFileName;

    handleArgs(argc, argv, &clientId, &commonDirName, &inputDirName, &mirrorDirName, &bufferSize, &logFileName);

    char idFilePath[strlen(commonDirName) + 1 + MAX_STRING_INT_SIZE + 4];
    doClientInitialChecks(inputDirName, mirrorDirName, commonDirName, clientId, &idFilePath);

    char pidS[MAX_STRING_INT_SIZE];
    sprintf(pidS, "%d", getpid());
    createAndWriteToFile(idFilePath, pidS);

    FileList* inputFileList = initFileList();
    populateFileList(inputFileList, inputDirName, "", 0);
    printf("ha\n");
    initialSync(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId);
    startWatchingCommonDirectory(commonDirName, mirrorDirName, bufferSize, inputFileList, clientId);

    return 0;
}