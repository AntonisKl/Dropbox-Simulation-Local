#include "file_list.h"

File* initFile(char* path, off_t contentsSize, FileType type) {
    File* file = (File*)malloc(sizeof(File));

    file->path = (char*)malloc(strlen(path) + 1);
    strcpy(file->path, path);

    file->contentsSize = contentsSize;
    file->type = type;

    file->nextFile = NULL;

    return file;
}

void freeFile(File** file) {
    if ((*file) == NULL)
        return;

    free((*file)->path);
    (*file)->path = NULL;

    (*file)->nextFile = NULL;

    free(*file);
    (*file) = NULL;
}

void freeFileRec(File** file) {
    if ((*file) == NULL)
        return;

    freeFileRec(&((*file)->nextFile));

    freeFile(&(*file));

    return;
}

FileList* initFileList() {
    FileList* fileList = (FileList*)malloc(sizeof(FileList));
    fileList->size = 0;
    fileList->firstFile = NULL;

    return fileList;
}

void freeFileList(FileList** fileList) {
    if ((*fileList) == NULL)
        return;

    freeFileRec(&(*fileList)->firstFile);

    free(*fileList);
    (*fileList) = NULL;

    return;
}

File* findFileInFileList(FileList* fileList, char* path) {
    if (fileList == NULL)
        return NULL;

    File* curFile = fileList->firstFile;

    while (curFile != NULL) {
        if (strcmp(curFile->path, path) == 0) {
            return curFile;
        } else if (strcmp(path, curFile->path) < 0) {  // no need for searching further since the list is sorted by fileId
            return NULL;
        }
        curFile = curFile->nextFile;
    }
    return NULL;
}

File* addFileToFileList(FileList* fileList, char* path, off_t contentsSize, FileType type) {
    if (fileList->size == 0) {
        fileList->firstFile = initFile(path, contentsSize, type);

        fileList->size++;
        // printf(ANSI_COLOR_MAGENTA "Inserted file with path %s to FileList\n" ANSI_COLOR_RESET, path);
        return fileList->firstFile;
    } else {
        File* curFile = fileList->firstFile;

        if (strcmp(path, curFile->path) < 0) {
            // insert at start
            File* fileToInsert = initFile(path, contentsSize, type);
            fileToInsert->nextFile = curFile;

            fileList->firstFile = fileToInsert;
            fileList->size++;
            // printf(ANSI_COLOR_MAGENTA "Inserted file with path %s to FileList\n" ANSI_COLOR_RESET, path);
            return fileList->firstFile;
        }
        while (curFile != NULL) {
            if (curFile->nextFile != NULL) {
                if (strcmp(path, curFile->nextFile->path) < 0) {
                    File* fileToInsert = initFile(path, contentsSize, type);
                    fileToInsert->nextFile = curFile->nextFile;

                    curFile->nextFile = fileToInsert;
                    fileList->size++;
                    // printf(ANSI_COLOR_MAGENTA "Inserted file with path %s to FileList\n" ANSI_COLOR_RESET, path);
                    return curFile->nextFile;
                }
            } else {
                // insert at the end
                curFile->nextFile = initFile(path, contentsSize, type);

                fileList->size++;
                // printf(ANSI_COLOR_MAGENTA "Inserted file with path %s to FileList\n" ANSI_COLOR_RESET, path);
                return curFile->nextFile;
            }

            curFile = curFile->nextFile;
        }
    }

    return NULL;  // not normal behavior
}
