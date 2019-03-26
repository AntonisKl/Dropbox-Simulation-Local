#include "tree.h"

DirTree* initDirTree(char* rootName, char* pathName, INodesList* iNodesList, ino_t id, time_t lastModTime, off_t size) {
    DirTree* dirTree = (DirTree*)malloc(sizeof(DirTree));

    INode* createdINode = addINodeToINodesList(iNodesList, id, lastModTime, size, pathName);
    dirTree->rootNode = initTreeNode(rootName, pathName, createdINode, Directory);
    dirTree->size = 1;

    return dirTree;
}

TreeNode* initTreeNode(char* name, char* pathName, INode* iNodeP, Type type) {
    TreeNode* treeNode = (TreeNode*)malloc(sizeof(TreeNode));
    treeNode->name = (char*)malloc(PATH_MAX);
    strcpy(treeNode->name, name);

    treeNode->pathName = (char*)malloc(PATH_MAX);
    strcpy(treeNode->pathName, pathName);

    treeNode->iNodeP = iNodeP;

    treeNode->iNodeP->destINodeP = NULL;
    treeNode->type = type;
    treeNode->prevNode = NULL;
    treeNode->nextNode = NULL;
    treeNode->firstChildNode = NULL;
    if (type == Directory)
        treeNode->contentsNum = 0;
    else
        treeNode->contentsNum = -1;

    return treeNode;
}

void freeTreeNode(TreeNode** treeNode, INodesList* iNodesList) {
    int deleteRet = deleteNameNodeFromNamesList((*treeNode)->iNodeP->namesList, (*treeNode)->pathName);
    if (deleteRet == -1)
        return;

    if ((*treeNode)->iNodeP->namesList->size == 0) {
        deleteINodeFromINodesList(iNodesList, (*treeNode)->iNodeP->id);
    }

    free((*treeNode)->name);
    (*treeNode)->name = NULL;
    free((*treeNode)->pathName);
    (*treeNode)->pathName = NULL;
    free((*treeNode));
    (*treeNode) = NULL;
    return;
}

WdAndTreeNodesList* initWdAndTreeNodesList() {
    WdAndTreeNodesList* wdAndTreeNodesList = (WdAndTreeNodesList*)malloc(sizeof(WdAndTreeNodesList));
    wdAndTreeNodesList->size = 0;
    wdAndTreeNodesList->firstNode = NULL;

    return wdAndTreeNodesList;
}

WatchDescAndTreeNode* initWatchDescAndTreeNode(int wd, TreeNode* sourceTreeNode, TreeNode* destTreeNode, TreeNode* sourceParentDir, TreeNode* destParentDir) {
    WatchDescAndTreeNode* watchDescAndTreeNode = (WatchDescAndTreeNode*)malloc(sizeof(WatchDescAndTreeNode));
    watchDescAndTreeNode->wd = wd;
    // pointers to corresponding TreeNodes to help afterwards when an event is made
    watchDescAndTreeNode->sourceTreeNodeP = sourceTreeNode;
    watchDescAndTreeNode->destTreeNodeP = destTreeNode;
    watchDescAndTreeNode->sourceParentDirP = sourceParentDir;
    watchDescAndTreeNode->destParentDirP = destParentDir;
    watchDescAndTreeNode->nextNode = NULL;
    watchDescAndTreeNode->prevNode = NULL;
    return watchDescAndTreeNode;
}

void freeWatchDescAndTreeNode(WatchDescAndTreeNode** watchDescAndTreeNode) {
    if ((*watchDescAndTreeNode) == NULL)
        return;

    free(*watchDescAndTreeNode);
    (*watchDescAndTreeNode) = NULL;
    return;
}

void freeWdAndTreeNodesList(WdAndTreeNodesList** wdAndTreeNodesList) {
    if ((*wdAndTreeNodesList) == NULL)
        return;

    WatchDescAndTreeNode* curWatchDescAndTreeNode = (*wdAndTreeNodesList)->firstNode;
    // free all nodes
    while (curWatchDescAndTreeNode != NULL) {
        if (curWatchDescAndTreeNode->nextNode != NULL) {
            curWatchDescAndTreeNode = curWatchDescAndTreeNode->nextNode;
            free(curWatchDescAndTreeNode->prevNode);
            curWatchDescAndTreeNode->prevNode = NULL;
        } else {
            free(curWatchDescAndTreeNode);
            curWatchDescAndTreeNode = NULL;
        }
    }
    free(*wdAndTreeNodesList);
    (*wdAndTreeNodesList) = NULL;
}

WatchDescAndTreeNode* findWatchDescAndTreeNodeInWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd) {
    WatchDescAndTreeNode* curWatchDescAndTreeNode = wdAndTreeNodesList->firstNode;

    while (curWatchDescAndTreeNode != NULL) {
        if (curWatchDescAndTreeNode->wd == wd) {
            return curWatchDescAndTreeNode;
        } else if (wd < curWatchDescAndTreeNode->wd) {
            return NULL;
        }
        curWatchDescAndTreeNode = curWatchDescAndTreeNode->nextNode;
    }
    return NULL;
}

WatchDescAndTreeNode* addWatchDescAndTreeNodeToWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd, TreeNode* sourceTreeNode, TreeNode* destTreeNode, TreeNode* sourceParentDir, TreeNode* destParentDir) {
    if (wdAndTreeNodesList->size == 0) {
        wdAndTreeNodesList->firstNode = initWatchDescAndTreeNode(wd, sourceTreeNode, destTreeNode, sourceParentDir, destParentDir);

        wdAndTreeNodesList->size++;
        return wdAndTreeNodesList->firstNode;
    } else {
        WatchDescAndTreeNode* curWatchDescAndTreeNode = wdAndTreeNodesList->firstNode;

        if (wd < curWatchDescAndTreeNode->wd) {
            // insert to the start of the list
            WatchDescAndTreeNode* watchDescAndTreeNodeToInsert = initWatchDescAndTreeNode(wd, sourceTreeNode, destTreeNode, sourceParentDir, destParentDir);
            watchDescAndTreeNodeToInsert->nextNode = curWatchDescAndTreeNode;

            curWatchDescAndTreeNode->prevNode = watchDescAndTreeNodeToInsert;
            wdAndTreeNodesList->firstNode = watchDescAndTreeNodeToInsert;
            wdAndTreeNodesList->size++;
            return wdAndTreeNodesList->firstNode;
        }
        while (curWatchDescAndTreeNode != NULL) {
            if (curWatchDescAndTreeNode->nextNode != NULL) {
                if (wd < curWatchDescAndTreeNode->nextNode->wd) {
                    WatchDescAndTreeNode* watchDescAndTreeNodeToInsert = initWatchDescAndTreeNode(wd, sourceTreeNode, destTreeNode, sourceParentDir, destParentDir);
                    watchDescAndTreeNodeToInsert->prevNode = curWatchDescAndTreeNode;
                    watchDescAndTreeNodeToInsert->nextNode = curWatchDescAndTreeNode->nextNode;

                    curWatchDescAndTreeNode->nextNode->prevNode = watchDescAndTreeNodeToInsert;
                    curWatchDescAndTreeNode->nextNode = watchDescAndTreeNodeToInsert;
                    wdAndTreeNodesList->size++;
                    return curWatchDescAndTreeNode->nextNode;
                }
            } else {
                // insert to the end of the list
                curWatchDescAndTreeNode->nextNode = initWatchDescAndTreeNode(wd, sourceTreeNode, destTreeNode, sourceParentDir, destParentDir);
                curWatchDescAndTreeNode->nextNode->prevNode = curWatchDescAndTreeNode;

                wdAndTreeNodesList->size++;
                return curWatchDescAndTreeNode->nextNode;
            }
            curWatchDescAndTreeNode = curWatchDescAndTreeNode->nextNode;
        }
    }

    return NULL;
}

int deleteWatchDescAndTreeNodeFromWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd) {
    WatchDescAndTreeNode* watchDescAndTreeNodeToDelete = findWatchDescAndTreeNodeInWdAndTreeNodesList(wdAndTreeNodesList, wd);

    if (watchDescAndTreeNodeToDelete == NULL) {
        return -1;
    }

    if (watchDescAndTreeNodeToDelete->wd < wdAndTreeNodesList->firstNode->wd)  // if it is the first node of the list
    {
        wdAndTreeNodesList->firstNode = watchDescAndTreeNodeToDelete->nextNode;
    }

    if (watchDescAndTreeNodeToDelete->prevNode != NULL) {
        watchDescAndTreeNodeToDelete->prevNode->nextNode = watchDescAndTreeNodeToDelete->nextNode;
    }
    if (watchDescAndTreeNodeToDelete->nextNode != NULL) {
        watchDescAndTreeNodeToDelete->nextNode->prevNode = watchDescAndTreeNodeToDelete->prevNode;
    }

    // free the node after deleting it
    freeWatchDescAndTreeNode(&watchDescAndTreeNodeToDelete);
    wdAndTreeNodesList->size--;

    return 0;
}

TreeNode* addTreeNodeToDir(DirTree* tree, TreeNode* parentDir, char* name, char* pathName, INodesList* iNodesList, ino_t id, time_t lastModTime, off_t size, Type type, TreeNode* sourceTreeNode /*this is null while populating tree*/) {
    TreeNode* existingTreeNode = findTreeNodeInDir(parentDir, name, -1);

    INode* existingINode = NULL;

    if (sourceTreeNode != NULL && existingTreeNode != NULL) {  // we are trying to add an existing tree node to a *destination* directory
        if (existingTreeNode->type != type) {
            if (deleteTreeNodeFromDir(tree, parentDir, name, pathName, iNodesList) == 0) {  // if tree node with the same name exists in current directory then delete it first
                deleteFileOrDirectory(pathName);
                printf("A %s with name %s already existed in directory as a %s with name %s so it has been deleted\n",
                       type == File ? "file" : "directory", name, type == Directory ? "file" : "directory", parentDir->name);
            }
        } else {
            if (sourceTreeNode->iNodeP->lastModTime != existingTreeNode->iNodeP->lastModTime || sourceTreeNode->iNodeP->size != existingTreeNode->iNodeP->size) {
                if (existingTreeNode->type == Directory) {
                    // special treatment for already existing directories
                    copyDirAttributes(sourceTreeNode->pathName, existingTreeNode->pathName);
                    struct stat curStat;
                    stat(pathName, &curStat);

                    existingTreeNode->iNodeP->lastModTime = curStat.st_mtime;
                    existingTreeNode->iNodeP->size = curStat.st_size;

                    sourceTreeNode->iNodeP->destINodeP = existingTreeNode->iNodeP;

                    return existingTreeNode;
                }

                if (deleteTreeNodeFromDir(tree, parentDir, name, pathName, iNodesList) == 0) {  // if tree node with the same name exists in current directory then delete it first
                    deleteFileOrDirectory(pathName);
                    printf("A %s with name %s already existed in directory as a %s with name %s so it has been deleted\n",
                           type == File ? "file" : "directory", name, type == Directory ? "file" : "directory", parentDir->name);
                }

                // if sourceTreeNode->iNodeP->destINodeP is != NULL then there is no need to create a new i-node
                existingINode = sourceTreeNode->iNodeP->destINodeP;
            } else {
                printf("Directory/File with name %s already in directory with name %s\n", name, parentDir->name);
                return existingTreeNode;
            }
        }
    } else {
        // check if INode already exists
        if (sourceTreeNode == NULL) {
            // if there is already an i-node with the same ino_t/id then there is no need to create a new i-node
            existingINode = findINodeInINodesList(iNodesList, id);
        } else {
            // if sourceTreeNode->iNodeP->destINodeP is != NULL then there is no need to create a new i-node
            existingINode = sourceTreeNode->iNodeP->destINodeP;
        }
    }

    INode* createdINode = NULL;
    if (sourceTreeNode == NULL && existingINode == NULL) {
        // create a new INode and add the current pathName to its NamesList
        createdINode = addINodeToINodesList(iNodesList, id, lastModTime, size, pathName);
    }

    TreeNode* curTreeNode = parentDir->firstChildNode;
    if (curTreeNode == NULL) {
        if (sourceTreeNode != NULL) {
            // handle copy
            if (type == File) {
                copyFileOrDirectory(sourceTreeNode->pathName, pathName);
            } else {
                createDirAndCopyAttributes(sourceTreeNode->pathName, pathName);
            }
        }
        if (sourceTreeNode != NULL && existingINode == NULL && createdINode == NULL) {
            struct stat curStat;
            stat(pathName, &curStat);
            // create a new INode with the statistics from the destination's file/directory and add the current pathName to its NamesList
            createdINode = addINodeToINodesList(iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, pathName);
        }
        parentDir->firstChildNode = initTreeNode(name, pathName, existingINode == NULL ? createdINode : existingINode, type);
        tree->size++;
        parentDir->contentsNum++;
        printf("Directory/File with name %s added successfully to directory with name %s\n", name, parentDir->name);
        return parentDir->firstChildNode;
    }

    if (strcmp(name, curTreeNode->name) <= 0) {
        // add the TreeNode to the start of the list
        if (sourceTreeNode != NULL) {
            // handle copy
            if (type == File) {
                copyFileOrDirectory(sourceTreeNode->pathName, pathName);
            } else {
                createDirAndCopyAttributes(sourceTreeNode->pathName, pathName);
            }
        }

        if (sourceTreeNode != NULL && existingINode == NULL && createdINode == NULL) {
            struct stat curStat;
            stat(pathName, &curStat);
            // create a new INode with the statistics from the destination's file/directory and add the current pathName to its NamesList
            createdINode = addINodeToINodesList(iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, pathName);
        }

        TreeNode* treeNodeToAdd = initTreeNode(name, pathName, existingINode == NULL ? createdINode : existingINode, type);

        treeNodeToAdd->nextNode = curTreeNode;

        curTreeNode->prevNode = treeNodeToAdd;
        parentDir->firstChildNode = treeNodeToAdd;
        tree->size++;
        parentDir->contentsNum++;

        if (sourceTreeNode != NULL) {
            // link destination's INode to the source's INode
            sourceTreeNode->iNodeP->destINodeP = treeNodeToAdd->iNodeP;
        }

        printf("Directory/File with name %s added successfully to directory with name %s\n", name, parentDir->name);
        return treeNodeToAdd;
    }

    while (curTreeNode != NULL) {
        if (curTreeNode->nextNode != NULL) {
            if (strcmp(name, curTreeNode->nextNode->name) <= 0) {
                if (sourceTreeNode != NULL) {
                    // handle copy
                    if (type == File) {
                        copyFileOrDirectory(sourceTreeNode->pathName, pathName);
                    } else {
                        createDirAndCopyAttributes(sourceTreeNode->pathName, pathName);
                    }
                }

                if (sourceTreeNode != NULL && existingINode == NULL && createdINode == NULL) {
                    struct stat curStat;
                    stat(pathName, &curStat);
                    // create a new INode with the statistics from the destination's file/directory and add the current pathName to its NamesList
                    createdINode = addINodeToINodesList(iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, pathName);
                }

                TreeNode* treeNodeToAdd = initTreeNode(name, pathName, existingINode == NULL ? createdINode : existingINode, type);
                treeNodeToAdd->prevNode = curTreeNode;
                treeNodeToAdd->nextNode = curTreeNode->nextNode;
                curTreeNode->nextNode->prevNode = treeNodeToAdd;
                curTreeNode->nextNode = treeNodeToAdd;

                tree->size++;
                parentDir->contentsNum++;

                if (sourceTreeNode != NULL) {
                    // link destination's INode to the source's INode
                    sourceTreeNode->iNodeP->destINodeP = treeNodeToAdd->iNodeP;
                }

                printf("Directory/File with name %s added successfully to directory with name %s\n", name, parentDir->name);
                return treeNodeToAdd;
            }
        } else {
            // add the TreeNode to the end of the list
            if (sourceTreeNode != NULL) {
                // handle copy
                if (type == File) {
                    copyFileOrDirectory(sourceTreeNode->pathName, pathName);
                } else {
                    createDirAndCopyAttributes(sourceTreeNode->pathName, pathName);
                }
            }

            if (sourceTreeNode != NULL && existingINode == NULL && createdINode == NULL) {
                struct stat curStat;
                stat(pathName, &curStat);
                // create a new INode with the statistics from the destination's file/directory and add the current pathName to its NamesList
                createdINode = addINodeToINodesList(iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, pathName);
            }

            TreeNode* treeNodeToAdd = initTreeNode(name, pathName, existingINode == NULL ? createdINode : existingINode, type);
            treeNodeToAdd->prevNode = curTreeNode;
            curTreeNode->nextNode = treeNodeToAdd;

            tree->size++;
            parentDir->contentsNum++;

            if (sourceTreeNode != NULL) {
                // link destination's INode to the source's INode
                sourceTreeNode->iNodeP->destINodeP = treeNodeToAdd->iNodeP;
            }
            printf("Directory/File with name %s added successfully to directory with name %s\n", name, parentDir->name);
            return treeNodeToAdd;
        }
        curTreeNode = curTreeNode->nextNode;
    }

    printf("Oops\n");
    return NULL;  // not normal behavior
}

int deleteTreeNodeFromDir(DirTree* tree, TreeNode* parentDir, char* name, char* pathName, INodesList* iNodesList) {
    if (parentDir == NULL) {
        printf("Delete -> NULL parent directory given as parameter\n");
        return -1;
    }

    if (parentDir->firstChildNode == NULL) {
        printf("Delete -> Directory/File with name %s not found inside directory with name %s\n", name, parentDir->name);
        return -1;  // not found
    }

    TreeNode* curTreeNode = parentDir->firstChildNode;

    if (strcmp(name, curTreeNode->name) < 0) {
        printf("Delete -> Directory/File with name %s not found inside directory with name %s\n", name, parentDir->name);
        return -1;  // not found
    }

    if (strcmp(name, curTreeNode->name) == 0) {
        // it is the first child node
        parentDir->firstChildNode = curTreeNode->nextNode;

        if (parentDir->firstChildNode != NULL) {
            if (parentDir->firstChildNode->prevNode != NULL) {
                freeTreeNode(&(parentDir->firstChildNode->prevNode), iNodesList);
                parentDir->firstChildNode->prevNode = NULL;
            }
        }
        tree->size--;

        printf("Delete -> Directory/File with name %s deleted successfully from directory with name %s\n", name, parentDir->name);
        return 0;
    }

    curTreeNode = curTreeNode->nextNode;

    while (curTreeNode != NULL) {
        if (strcmp(name, curTreeNode->name) < 0)  // no need to search further since the list is sorted
            break;

        if (strcmp(name, curTreeNode->name) == 0) {
            curTreeNode->prevNode->nextNode = curTreeNode->nextNode;  // no need to check for NULL cause we are past the first tree node of this directory

            if (curTreeNode->nextNode != NULL) {
                curTreeNode->nextNode->prevNode = curTreeNode->prevNode;
            }

            freeTreeNode(&curTreeNode, iNodesList);
            tree->size--;

            printf("Delete -> Directory/File with name %s deleted successfully from directory with name %s\n", name, parentDir->name);
            return 0;
        }

        curTreeNode = curTreeNode->nextNode;
    }

    printf("Delete -> Directory/File with name %s not found inside directory with name %s\n", name, parentDir->name);
    return -1;  // not found
}

TreeNode* findTreeNodeInDir(TreeNode* parentDir, char* name, Type type) {  // if type = -1: we don't care about the type of the tree node

    if (parentDir == NULL) {
        printf("Find -> NULL parent directory given as parameter\n");
        return NULL;
    }

    TreeNode* curTreeNode = parentDir->firstChildNode;

    while (curTreeNode != NULL) {
        if (strcmp(name, curTreeNode->name) < 0)  // no need to search further since the list is sorted
            break;

        if ((type == -1 && strcmp(name, curTreeNode->name) == 0) || (curTreeNode->type == type && strcmp(name, curTreeNode->name) == 0))
            return curTreeNode;

        curTreeNode = curTreeNode->nextNode;
    }

    printf("Find -> Directory/File with name %s not found inside directory with name %s\n", name, parentDir->name);
    return NULL;  // not found
}

void freeDirTreeNodes(TreeNode* treeNode, INodesList* iNodesList) {
    if (treeNode == NULL)
        return;

    if (treeNode->type == Directory) {
        freeDirTreeNodes(treeNode->firstChildNode, iNodesList);
    }

    freeDirTreeNodes(treeNode->nextNode, iNodesList);

    freeTreeNode(&treeNode, iNodesList);
}

void populateTree(const char* dirName, int indent, DirTree* dirTree, TreeNode* parentDir, INodesList* iNodesList) {
    DIR* dir;
    struct dirent* entry;

    if ((dir = opendir(dirName)) == NULL) {
        perror("Could not open directory");
        return;
    }

    // traverse the whole directory recursively
    while ((entry = readdir(dir)) != NULL) {
        char path[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
        struct stat curStat;
        stat(path, &curStat);

        if (!S_ISREG(curStat.st_mode)) {  // if it is a directory
            if (parentDir == NULL) {
                parentDir = dirTree->rootNode;
            }

            TreeNode* addedDir = addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, Directory, NULL);

            printf("Handled directory: %*s[%s]\n", indent, "", entry->d_name);
            populateTree(path, indent + 2, dirTree, addedDir, iNodesList);
        } else {  // if it is a file

            if (parentDir == NULL) {
                parentDir = dirTree->rootNode;
            }

            addTreeNodeToDir(dirTree, parentDir, entry->d_name, path, iNodesList, curStat.st_ino, curStat.st_mtime, curStat.st_size, File, NULL);

            printf("Handled file: %*s- %s\n", indent, "", entry->d_name);
        }
    }

    closedir(dir);
}

void dfsFor2Trees(DirTree* sourceDirTree, DirTree* destDirTree, INodesList* sourceINodesList, INodesList* destINodesList, TreeNode* curSourceDir,
                  TreeNode* curDestDir, WdAndTreeNodesList* wdAndTreeNodesList, int iNotifyFd) {
    TreeNode *curSourceTreeNode, *curDestTreeNode;
    curSourceTreeNode = curSourceDir->firstChildNode;
    curDestTreeNode = curDestDir->firstChildNode;
    struct stat curStat;

    // recursively synchronize source's and destination's structures
    while (curSourceTreeNode != NULL || curDestTreeNode != NULL) {
        // printf("-----------------> current source tree node: %s, current destination tree node: %s\n",
        //        curSourceTreeNode == NULL ? "NULL" : curSourceTreeNode->pathName, curDestTreeNode == NULL ? "NULL" : curDestTreeNode->pathName);
        if (curDestTreeNode != NULL) {
            TreeNode* foundSourceTreeNode = findTreeNodeInDir(curSourceDir, curDestTreeNode->name, curDestTreeNode->type);
            if (foundSourceTreeNode == NULL) {
                char tempPathName[PATH_MAX], tempName[PATH_MAX];
                strcpy(tempPathName, curDestTreeNode->pathName);
                strcpy(tempName, curDestTreeNode->name);

                curDestTreeNode = curDestTreeNode->nextNode;  // do this assignment now because otherwise afterwards curDestTreeNode will be NULL
                deleteTreeNodeFromDir(destDirTree, curDestDir, tempName, tempPathName, destINodesList);
                deleteFileOrDirectory(tempPathName);
            }
        }

        if (curSourceTreeNode == NULL)
            continue;

        // handle existing source TreeNode
        stat(curSourceTreeNode->pathName, &curStat);

        if (curSourceTreeNode->type == File) {
            // try to find a hard link
            TreeNode* tempDestTreeNode = curDestTreeNode;
            while (tempDestTreeNode != NULL) {
                if (tempDestTreeNode->iNodeP->id == curStat.st_ino) {
                    curSourceTreeNode->iNodeP->destINodeP = tempDestTreeNode->iNodeP;
                    linkFile(curSourceTreeNode->pathName, tempDestTreeNode->pathName);

                    break;
                }
                tempDestTreeNode = tempDestTreeNode->nextNode;
            }
        }

        // form destination's file/directory path
        char newPath[PATH_MAX];
        strcpy(newPath, curDestDir->pathName);
        strcat(newPath, "/");
        strcat(newPath, curSourceTreeNode->name);

        curDestTreeNode = addTreeNodeToDir(destDirTree, curDestDir, curSourceTreeNode->name, newPath, destINodesList,
                                           curStat.st_ino, curStat.st_mtime, curStat.st_size, curSourceTreeNode->type, curSourceTreeNode);

        if (curSourceTreeNode != NULL && curDestTreeNode != NULL) {
            if (curSourceTreeNode->type == Directory && curDestTreeNode->type == Directory) {
                // add entry to WdAndTreeNodesList with wd = -1 temporarily
                // watches will be set afterwards from mirr.c
                addWatchDescAndTreeNodeToWdAndTreeNodesList(wdAndTreeNodesList, -1, curSourceTreeNode, curDestTreeNode, curSourceDir, curDestDir);

                dfsFor2Trees(sourceDirTree, destDirTree, sourceINodesList, destINodesList, curSourceTreeNode, curDestTreeNode,
                             wdAndTreeNodesList, iNotifyFd);
            }
        }
        if (curSourceTreeNode != NULL) {
            curSourceTreeNode = curSourceTreeNode->nextNode;
        }
        if (curDestTreeNode != NULL) {
            curDestTreeNode = curDestTreeNode->nextNode;
        }
    }
    return;
}

void freeDirTreeAndINodesList(DirTree* dirTree, INodesList* iNodesList) {
    // free the whole DirTree and INodesList
    freeDirTreeNodes(dirTree->rootNode, iNodesList);
    free(iNodesList);
    free(dirTree);
}

void addWatches(TreeNode* sourceRootDir, TreeNode* destRootDir, WdAndTreeNodesList* wdAndTreeNodesList, int iNotifyFd) {
    WatchDescAndTreeNode* curWdAndTreeNode = wdAndTreeNodesList->firstNode;
    // set valid watch descriptors
    while (curWdAndTreeNode != NULL) {
        if (curWdAndTreeNode->wd == -1) {
            curWdAndTreeNode->wd = inotify_add_watch(iNotifyFd, curWdAndTreeNode->sourceTreeNodeP->pathName, IN_CREATE | IN_ATTRIB | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
        }
        curWdAndTreeNode = curWdAndTreeNode->nextNode;
    }
    // add the entry for the root directory
    addWatchDescAndTreeNodeToWdAndTreeNodesList(wdAndTreeNodesList,
                                                inotify_add_watch(iNotifyFd, sourceRootDir->pathName, IN_CREATE | IN_ATTRIB | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO),
                                                sourceRootDir, destRootDir, NULL, NULL);
}

void handleCreateEvent(DirTree* sourceDirTree, DirTree* destDirTree, TreeNode* curSourceParentDir, TreeNode* curDestParentDir, char* curName,
                       char* pathName, INodesList* sourceINodesList, INodesList* destINodesList, struct stat curStat) {
    // add tree node to source directory
    TreeNode* createdSourceTreeNode = addTreeNodeToDir(sourceDirTree, curSourceParentDir, curName, pathName, sourceINodesList,
                                                       curStat.st_ino, curStat.st_mtime, curStat.st_size, File, NULL);

    if (createdSourceTreeNode == NULL) {
        printf("Oops...\n");
        return;
    }

    TreeNode* tempDestTreeNode = curDestParentDir->firstChildNode;
    // try to find a hard link
    while (tempDestTreeNode != NULL) {
        if (tempDestTreeNode->iNodeP->id == curStat.st_ino) {
            createdSourceTreeNode->iNodeP->destINodeP = tempDestTreeNode->iNodeP;
            linkFile(createdSourceTreeNode->pathName, tempDestTreeNode->pathName);

            break;
        }
        tempDestTreeNode = tempDestTreeNode->nextNode;
    }

    // form destination's file/directory path
    char newPath[PATH_MAX];
    strcpy(newPath, curDestParentDir->pathName);
    strcat(newPath, "/");
    strcat(newPath, curName);

    // add tree node to destination directory
    addTreeNodeToDir(destDirTree, curDestParentDir, curName, newPath, destINodesList,
                     curStat.st_ino, curStat.st_mtime, curStat.st_size, createdSourceTreeNode->type,
                     createdSourceTreeNode);
}

void startWatchingDirectory(int iNotifyFd, WdAndTreeNodesList* wdAndTreeNodesList, DirTree* sourceDirTree, INodesList* sourceINodesList,
                            DirTree* destDirTree, INodesList* destINodesList, char** curName /* this is a global variable for it to be able to be freed by the signal handler */) {
    struct inotify_event iNotifyEvent;
    int readRet, wd;
    uint32_t mask, nameLen, cookieFrom, cookieTo;
    TreeNode *prevSourceParentDir = NULL, *prevDestParentDir = NULL;
    (*curName) = NULL;
    char* prevName = NULL;
    while ((readRet = read(iNotifyFd, &iNotifyEvent, sizeof(struct inotify_event) + NAME_MAX + 1)) != -1) {
        wd = iNotifyEvent.wd;
        mask = iNotifyEvent.mask;
        nameLen = iNotifyEvent.len;

        if (nameLen > 0) {
            (*curName) = (char*)malloc(nameLen);

            strcpy((*curName), iNotifyEvent.name);

            printf("New event->wd: %d, mask: %d, name: %s\n", wd, mask, iNotifyEvent.name);
        } else {
            printf("New event->wd: %d, mask: %d\n", wd, mask);
        }
        // find the entry in WdAndTreeNodesList
        WatchDescAndTreeNode* curWdAndTreeNode = findWatchDescAndTreeNodeInWdAndTreeNodesList(wdAndTreeNodesList, wd);  // it should NOT return NULL

        TreeNode* curSourceParentDir = curWdAndTreeNode->sourceTreeNodeP;
        TreeNode* curDestParentDir = curWdAndTreeNode->destTreeNodeP;
        // the below 2 TreeNodes are used only for SELF events like IN_DELETE_SELF
        TreeNode* curSourceGParentDir = curWdAndTreeNode->sourceParentDirP;
        TreeNode* curDestGParentDir = curWdAndTreeNode->destParentDirP;

        char pathName[PATH_MAX];
        struct stat curStat;

        if (nameLen > 0 && mask != IN_DELETE) {
            // form the path name of current file
            strcpy(pathName, curSourceParentDir->pathName);
            strcat(pathName, "/");
            strcat(pathName, (*curName));
            stat(pathName, &curStat);
        }

        char oldSourcePath[PATH_MAX], oldDestPath[PATH_MAX];
        if (cookieFrom != 0 && mask != IN_MOVED_TO && prevName != NULL && prevSourceParentDir != NULL && prevDestParentDir != NULL) {
            // if an IN_MOVED_FROM event was triggered previously and the current event is not IN_MOVED_TO
            // form previous source path
            strcpy(oldSourcePath, prevSourceParentDir->pathName);
            strcat(oldSourcePath, "/");
            strcat(oldSourcePath, prevName);

            // form previous destination path
            strcpy(oldDestPath, prevDestParentDir->pathName);
            strcat(oldDestPath, "/");
            strcat(oldDestPath, prevName);

            // update trees
            deleteTreeNodeFromDir(sourceDirTree, prevSourceParentDir, prevName, oldSourcePath, sourceINodesList);
            deleteTreeNodeFromDir(destDirTree, prevDestParentDir, prevName, oldDestPath, destINodesList);

            // update destination directory
            deleteFileOrDirectory(oldDestPath);

            // reset values that are related to "moved" event
            cookieFrom = 0;
            cookieTo = 0;
            free(prevName);
            prevName = NULL;
            prevSourceParentDir = NULL;
            prevDestParentDir = NULL;
        }

        switch (mask) {
            case IN_CREATE: {
                printf("Create\n");
                handleCreateEvent(sourceDirTree, destDirTree, curSourceParentDir, curDestParentDir, (*curName), pathName,
                                  sourceINodesList, destINodesList, curStat);
                break;
            }
            case IN_CREATE | IN_ISDIR: {
                printf("Create dir\n");

                // add tree node to source directory
                TreeNode* createdSourceTreeNode = addTreeNodeToDir(sourceDirTree, curSourceParentDir, (*curName), pathName, sourceINodesList,
                                                                   curStat.st_ino, curStat.st_mtime, curStat.st_size, Directory, NULL);

                if (createdSourceTreeNode == NULL) {
                    printf("Oops...\n");
                    break;
                }

                // form destination path
                char newPath[PATH_MAX];
                strcpy(newPath, curDestParentDir->pathName);  // check for curDestDir == NULL ??????????????????????????????????????????????????????????????????????
                strcat(newPath, "/");
                strcat(newPath, createdSourceTreeNode->name);

                // add tree node to destination directory
                TreeNode* createdDestTreeNode = addTreeNodeToDir(destDirTree, curDestParentDir, createdSourceTreeNode->name, newPath, destINodesList,
                                                                 curStat.st_ino, curStat.st_mtime, curStat.st_size, createdSourceTreeNode->type,
                                                                 createdSourceTreeNode);

                // because a new Directory was created, add a watch on it and add an entry to WdAndTreeNodesList
                addWatchDescAndTreeNodeToWdAndTreeNodesList(wdAndTreeNodesList, inotify_add_watch(iNotifyFd, createdSourceTreeNode->pathName, IN_CREATE | IN_ATTRIB | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO),
                                                            createdSourceTreeNode, createdDestTreeNode, curSourceParentDir, curDestParentDir);

                break;
            }
            case IN_ATTRIB: {
                printf("attrib\n");

                if (nameLen <= 0)  // if it is called for watched Directory
                    break;

                TreeNode* modSourceTreeNode = findTreeNodeInDir(curSourceParentDir, (*curName), S_ISREG(curStat.st_mode) ? File : Directory);
                if (modSourceTreeNode->iNodeP->lastModTime != curStat.st_mtime) {  // if the saved modification time is different
                    // update saved modification time for source TreeNode
                    modSourceTreeNode->iNodeP->lastModTime = curStat.st_mtime;

                    TreeNode* foundDestTreeNode = findTreeNodeInDir(curDestParentDir, (*curName), S_ISREG(curStat.st_mode) ? File : Directory);
                    if (foundDestTreeNode == NULL) {
                        printf("Oops...\n");
                        break;
                    }

                    // update saved modification time for destination TreeNode
                    foundDestTreeNode->iNodeP->lastModTime = curStat.st_mtime;
                }
                break;
            }
            case IN_MODIFY:
            case IN_CLOSE_WRITE: {
                printf("modify\n");

                TreeNode* modSourceTreeNode = findTreeNodeInDir(curSourceParentDir, (*curName), S_ISREG(curStat.st_mode) ? File : Directory);
                if (modSourceTreeNode == NULL) {
                    printf("Oops...\n");
                    break;
                }

                TreeNode* modDestTreeNode = findTreeNodeInDir(curDestParentDir, (*curName), S_ISREG(curStat.st_mode) ? File : Directory);
                if (modDestTreeNode == NULL) {
                    printf("Oops...\n");
                    break;
                }

                // update source and destination TreeNodes
                if (modSourceTreeNode->iNodeP->size != curStat.st_size) {
                    modSourceTreeNode->iNodeP->size = curStat.st_size;
                    modDestTreeNode->iNodeP->size = curStat.st_size;
                }

                modSourceTreeNode->iNodeP->lastModTime = curStat.st_mtime;
                modDestTreeNode->iNodeP->lastModTime = curStat.st_mtime;

                char newPath[PATH_MAX];
                strcpy(newPath, curDestParentDir->pathName);
                strcat(newPath, "/");
                strcat(newPath, (*curName));

                // copy the editted file to the destination
                copyFileOrDirectory(modSourceTreeNode->pathName, newPath);

                break;
            }
            case IN_DELETE: {
                printf("delete\n");

                char sourcePath[PATH_MAX];
                strcpy(sourcePath, curSourceParentDir->pathName);
                strcat(sourcePath, "/");
                strcat(sourcePath, (*curName));

                char destPath[PATH_MAX];
                strcpy(destPath, curDestParentDir->pathName);
                strcat(destPath, "/");
                strcat(destPath, (*curName));

                // delete destination file
                deleteFileOrDirectory(destPath);

                // delete TreeNodes
                deleteTreeNodeFromDir(sourceDirTree, curSourceParentDir, (*curName), sourcePath, sourceINodesList);
                deleteTreeNodeFromDir(destDirTree, curDestParentDir, (*curName), destPath, destINodesList);

                break;
            }
            case IN_DELETE_SELF: {
                printf("delete self\n");

                char sourcePath[PATH_MAX];
                strcpy(sourcePath, curSourceParentDir->pathName);

                char destPath[PATH_MAX];
                strcpy(destPath, curDestParentDir->pathName);
                
                // delete current directory
                deleteFileOrDirectory(destPath);

                // delete TreeNodes
                deleteTreeNodeFromDir(sourceDirTree, curSourceGParentDir, basename(sourcePath), sourcePath, sourceINodesList);
                deleteTreeNodeFromDir(destDirTree, curDestGParentDir, basename(sourcePath), destPath, destINodesList);
                break;
            }
            // case IN_DELETE | IN_ISDIR: {
            //     printf("isdir\n");
            //     break;
            // }
            case IN_MOVED_FROM: {
                printf("moved from\n");

                // set the needed variables
                cookieFrom = iNotifyEvent.cookie;
                prevName = (char*)malloc(NAME_MAX);
                strcpy(prevName, (*curName));
                prevSourceParentDir = curSourceParentDir;
                prevDestParentDir = curDestParentDir;

                break;
            }
            case IN_MOVED_TO: {
                printf("moved to\n");

                cookieTo = iNotifyEvent.cookie;

                if (prevName != NULL && prevSourceParentDir != NULL && prevDestParentDir != NULL) {
                    strcpy(oldSourcePath, prevSourceParentDir->pathName);
                    strcat(oldSourcePath, "/");
                    strcat(oldSourcePath, prevName);

                    strcpy(oldDestPath, prevDestParentDir->pathName);
                    strcat(oldDestPath, "/");
                    strcat(oldDestPath, prevName);

                    // delete the old TreeNodes
                    deleteTreeNodeFromDir(sourceDirTree, prevSourceParentDir, prevName, oldSourcePath, sourceINodesList);
                    deleteTreeNodeFromDir(destDirTree, prevDestParentDir, prevName, oldDestPath, destINodesList);
                }

                // add the new TreeNodes
                handleCreateEvent(sourceDirTree, destDirTree, curSourceParentDir, curDestParentDir, (*curName), pathName, sourceINodesList,
                                  destINodesList, curStat);

                if (cookieFrom != cookieTo) {
                    // just delete the old file
                    deleteFileOrDirectory(oldDestPath);
                } else {
                    char newDestPath[PATH_MAX];
                    strcpy(newDestPath, curDestParentDir->pathName);  // check for curDestDir == NULL ??????????????????????????????????????????????????????????????????????
                    strcat(newDestPath, "/");
                    strcat(newDestPath, (*curName));

                    // move the file from the old destination path to the new destination path
                    moveFile(oldDestPath, newDestPath);
                }

                // reset values that are related to "moved" event
                cookieFrom = 0;
                cookieTo = 0;
                if (prevName != NULL) {
                    free(prevName);
                    prevName = NULL;
                }
                prevSourceParentDir = NULL;
                prevDestParentDir = NULL;

                break;
            }
        }

        // free curName only if it was malloced
        if ((*curName) != NULL) {
            free((*curName));
            (*curName) = NULL;
        }
    }

    // error in read
    if (readRet == -1) {
        perror("read inotify desc error");
        exit(1);
    }
}