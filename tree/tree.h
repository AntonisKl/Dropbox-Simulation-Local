#ifndef TREE_H
#define TREE_H

#include "../utils/utils.h"

typedef struct TreeNode {
    char *name, *pathName;  // name and path name of the File/Directory
    INode* iNodeP;          // pointer pointing to the corresponding destination's INode
    struct TreeNode *nextNode, *prevNode, *firstChildNode;
    Type type;        // can be File or Directory
    int contentsNum;  // will be -1 for Files and >=0 for Diretories
} TreeNode;

// in each level of the DirTree the TreeNodes are sorted in an alphabetical order of their names (not the path names)
typedef struct DirTree {
    unsigned int size;
    TreeNode* rootNode;
} DirTree;

typedef struct WatchDescAndTreeNode {
    int wd;                                                                         // watch descriptor
    TreeNode *sourceTreeNodeP, *destTreeNodeP, *sourceParentDirP, *destParentDirP;  // sourceTreeNodeP: TreeNode pointer for the source TreeNode (the one who will be watched)
                                                                                    // destTreeNodeP: TreeNode pointer for the corresponding destination TreeNode
                                                                                    // sourceParentDirP: TreeNode pointer for the parent of the source TreeNode
                                                                                    // destParentDirP: TreeNode pointer for the parent of the corresponding destination TreeNode
    struct WatchDescAndTreeNode *nextNode, *prevNode;
} WatchDescAndTreeNode;

// WdAndTreeNodesList: sorted list of WatchDescAndTreeNodes in ascending order of wd
typedef struct WdAndTreeNodesList {
    unsigned int size;
    WatchDescAndTreeNode* firstNode;
} WdAndTreeNodesList;

// initializes DirTree and adds the root directory as the root node
DirTree* initDirTree(char* rootName, char* pathName, INodesList* iNodesList, ino_t id, time_t lastModTime, off_t size);

// initializes TreeNode
TreeNode* initTreeNode(char* name, char* pathName, INode* iNodeP, Type type);

// frees TreeNode and its included structures
void freeTreeNode(TreeNode** treeNode, INodesList* iNodesList);

// // initializes WdAndTreeNodesList
// WdAndTreeNodesList* initWdAndTreeNodesList();

// // initializes WatchDescAndTreeNode
// WatchDescAndTreeNode* initWatchDescAndTreeNode(int wd, TreeNode* sourceTreeNode, TreeNode* destTreeNode, TreeNode* sourceParentDir, TreeNode* destParentDir);

// frees WatchDescAndTreeNode
void freeWatchDescAndTreeNode(WatchDescAndTreeNode** watchDescAndTreeNode);

// frees WdAndTreeNodesList
void freeWdAndTreeNodesList(WdAndTreeNodesList** wdAndTreeNodesList);

// searches for and returns a WatchDescAndTreeNode inside a WdAndTreeNodesList, if it doesn't exist it returns NULL
WatchDescAndTreeNode* findWatchDescAndTreeNodeInWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd);

// adds a WatchDescAndTreeNode to a WdAndTreeNodesList and maintains the ascending sort order of wd
WatchDescAndTreeNode* addWatchDescAndTreeNodeToWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd, TreeNode* sourceTreeNode,
                                                                  TreeNode* destTreeNode, TreeNode* sourceParentDir, TreeNode* destParentDir);

// deletes WatchDescAndTreeNode from WdAndTreeNodesList
int deleteWatchDescAndTreeNodeFromWdAndTreeNodesList(WdAndTreeNodesList* wdAndTreeNodesList, int wd);

// adds a TreeNode to a Directory TreeNode and complies to the 1st main step of the coursework (synchronization)
TreeNode* addTreeNodeToDir(DirTree* tree, TreeNode* parentDir, char* name, char* pathName, INodesList* iNodesList, ino_t id, time_t lastModTime, off_t size,
                           Type type, TreeNode* sourceTreeNode /*this is NULL when populating tree for the first time*/);

// deletes a TreeNode from a Directory TreeNode
int deleteTreeNodeFromDir(DirTree* tree, TreeNode* parentDir, char* name, char* pathName, INodesList* iNodesList);

// finds a TreeNode in a Directory TreeNode
TreeNode* findTreeNodeInDir(TreeNode* parentDir, char* name, Type type);  // type = -1 when we don't care about the type of the tree node

// frees all the TreeNodes of a DirTree recursively
void freeDirTreeNodes(TreeNode* treeNode, INodesList* iNodesList);

// populates the structures (DirTree, INodesList, NamesLists) for a directory for the first time
void populateTree(const char* dirName, int indent, DirTree* dirTree, TreeNode* parentDir, INodesList* iNodesList);

// implements the functionality that is mentioned in the 1st main step of the coursework and synchronizes the source and destination directories and its structures
// it also populates the WdAndTreeNodesList but sets wd = -1 to all entries temporarily
void dfsFor2Trees(DirTree* sourceDirTree, DirTree* destDirTree, INodesList* sourceINodesList, INodesList* destINodesList, TreeNode* curSourceDir,
                  TreeNode* curDestDir, WdAndTreeNodesList* wdAndTreeNodesList, int iNotifyFd);

// frees DirTree and INodesList structures
void freeDirTreeAndINodesList(DirTree* dirTree, INodesList* iNodesList);

// adds watches to all the source directories that are already inserted into WdAndTreeNodesList and updates the wd variable of each entry
void addWatches(TreeNode* sourceRootDir, TreeNode* destRootDir, WdAndTreeNodesList* wdAndTreeNodesList, int iNotifyFd);

void handleCreateEvent(DirTree* sourceDirTree, DirTree* destDirTree, TreeNode* curSourceParentDir, TreeNode* curDestParentDir, char* curName,
                       char* pathName, INodesList* sourceINodesList, INodesList* destINodesList, struct stat curStat);

// starts reading the inotify's file descriptor iNotifyFd
void startWatchingDirectory(int iNotifyFd, WdAndTreeNodesList* wdAndTreeNodesList, DirTree* sourceDirTree, INodesList* sourceINodesList,
                            DirTree* destDirTree, INodesList* destINodesList, char** curName);

#endif