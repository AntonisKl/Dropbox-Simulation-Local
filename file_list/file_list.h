#ifndef LIST_H
#define LIST_H

#include "../utils/utils.h"

typedef enum FileType {
    DIRECTORY,
    REGULAR_FILE
} FileType;

typedef struct File {
    char* pathNoInputDir, *path;
    off_t contentsSize;
    FileType type;
    struct File* nextFile;
} File;

// FileList is sorted by fileId in ascending order
typedef struct FileList {
    File* firstFile;
    unsigned int size;
} FileList;

// File

File* initFile(char* pathNoInputDir, char* path, off_t contentsSize, FileType type);

void freeFile(File** file);

void freeFileRec(File** file);

// End

// File List

FileList* initFileList();

void freeFileList(FileList** fileList);

// find file in file list by taking advantage of sorting order
File* findFileInFileList(FileList* fileList, char* fileId);

// add file to file list by maintaining the sorting order
File* addFileToFileList(FileList* fileList, char* pathNoInputDir, char* path, off_t contentsSize, FileType type);

#endif